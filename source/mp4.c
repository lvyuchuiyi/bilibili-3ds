#include <stdlib.h>
#include <string.h>
#include "mp4.h"

static uint32_t be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static uint32_t be24(const uint8_t *p) {
    return ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | p[2];
}

/* Find a box with the given 4-char type in the buffer.
 * Returns pointer to box payload start, or NULL. */
static const uint8_t *find_box(const uint8_t *buf, size_t size,
                               const char *type, size_t *box_payload_size) {
    size_t pos = 0;
    while (pos + 8 <= size) {
        uint32_t box_size = be32(buf + pos);
        if (box_size < 8) box_size = size - pos;
        if (memcmp(buf + pos + 4, type, 4) == 0) {
            if (box_payload_size) *box_payload_size = box_size - 8;
            return buf + pos + 8;
        }
        pos += box_size;
    }
    return NULL;
}

/* Find a box recursively inside a container box payload. */
static const uint8_t *find_box_in(const uint8_t *payload, size_t payload_size,
                                  const char *type, size_t *box_payload_size) {
    size_t pos = 0;
    while (pos + 8 <= payload_size) {
        uint32_t box_size = be32(payload + pos);
        if (box_size < 8) box_size = payload_size - pos;
        if (memcmp(payload + pos + 4, type, 4) == 0) {
            if (box_payload_size) *box_payload_size = box_size - 8;
            return payload + pos + 8;
        }
        pos += box_size;
    }
    return NULL;
}

/* Grow output stream */
static int stream_grow(mp4_stream_t *s, size_t extra) {
    if (s->size + extra > s->capacity) {
        size_t new_cap = s->capacity ? s->capacity * 2 : 65536;
        while (new_cap < s->size + extra) new_cap *= 2;
        uint8_t *p = realloc(s->data, new_cap);
        if (!p) return -1;
        s->data = p;
        s->capacity = new_cap;
    }
    return 0;
}

static int stream_append(mp4_stream_t *s, const uint8_t *data, size_t len) {
    if (stream_grow(s, len) != 0) return -1;
    memcpy(s->data + s->size, data, len);
    s->size += len;
    return 0;
}

static int append_start_code(mp4_stream_t *s) {
    static const uint8_t sc[4] = {0, 0, 0, 1};
    return stream_append(s, sc, 4);
}

/* Extract SPS/PPS from avcC payload */
static int extract_avcc_sps_pps(mp4_stream_t *s, const uint8_t *avcc, size_t avcc_size) {
    if (!avcc || avcc_size < 7) return -1;

    uint8_t profile = avcc[1];
    uint8_t compat  = avcc[2];
    uint8_t level   = avcc[3];
    (void)profile; (void)compat; (void)level;

    int num_sps = avcc[5] & 0x1F;
    size_t pos = 6;
    for (int i = 0; i < num_sps && pos + 2 <= avcc_size; i++) {
        uint16_t sps_len = (uint16_t)((avcc[pos] << 8) | avcc[pos+1]);
        pos += 2;
        if (pos + sps_len > avcc_size) return -1;
        if (append_start_code(s) != 0) return -1;
        if (stream_append(s, avcc + pos, sps_len) != 0) return -1;
        pos += sps_len;
    }

    if (pos + 1 > avcc_size) return 0;
    int num_pps = avcc[pos++];
    for (int i = 0; i < num_pps && pos + 2 <= avcc_size; i++) {
        uint16_t pps_len = (uint16_t)((avcc[pos] << 8) | avcc[pos+1]);
        pos += 2;
        if (pos + pps_len > avcc_size) return -1;
        if (append_start_code(s) != 0) return -1;
        if (stream_append(s, avcc + pos, pps_len) != 0) return -1;
        pos += pps_len;
    }
    return 0;
}

/* Find avcC inside stsd box */
static int extract_avcc(mp4_stream_t *s, const uint8_t *stsd, size_t stsd_size) {
    if (stsd_size < 8) return -1;
    /* Skip version/flags + entry count */
    size_t pos = 8;
    while (pos + 8 <= stsd_size) {
        uint32_t entry_size = be32(stsd + pos);
        if (entry_size < 8) entry_size = stsd_size - pos;
        /* Check if this is avc1/avc3 */
        if (memcmp(stsd + pos + 4, "avc1", 4) == 0 ||
            memcmp(stsd + pos + 4, "avc3", 4) == 0) {
            /* Skip avc1 header (78 bytes) to reach child boxes */
            size_t child_start = pos + 8 + 78;
            if (child_start + 8 <= pos + entry_size) {
                size_t child_size = pos + entry_size - child_start;
                size_t avcc_size = 0;
                const uint8_t *avcc = find_box_in(stsd + child_start, child_size, "avcC", &avcc_size);
                if (avcc) return extract_avcc_sps_pps(s, avcc, avcc_size);
            }
        }
        pos += entry_size;
    }
    return -1;
}

/* Extract sample table from stbl */
static int extract_samples(mp4_stream_t *s,
                           const uint8_t *stbl, size_t stbl_size,
                           const uint8_t *mdat, size_t mdat_size) {
    size_t stsc_size = 0, stsz_size = 0, stco_size = 0;
    const uint8_t *stsc = find_box_in(stbl, stbl_size, "stsc", &stsc_size);
    const uint8_t *stsz = find_box_in(stbl, stbl_size, "stsz", &stsz_size);
    const uint8_t *stco = find_box_in(stbl, stbl_size, "stco", &stco_size);
    if (!stsc || !stsz || !stco || stsc_size < 8 || stsz_size < 12 || stco_size < 8)
        return -1;

    uint32_t stsc_count = be32(stsc + 4);
    uint32_t stsz_count = be32(stsz + 8);
    uint32_t stco_count = be32(stco + 4);

    if (stsc_count == 0 || stsz_count == 0 || stco_count == 0) return -1;

    /* Build sample sizes and offsets */
    uint32_t *sizes = calloc(stsz_count, sizeof(uint32_t));
    uint32_t *offsets = calloc(stsz_count, sizeof(uint32_t));
    if (!sizes || !offsets) { free(sizes); free(offsets); return -1; }

    for (uint32_t i = 0; i < stsz_count && i < (stsz_size - 12) / 4; i++)
        sizes[i] = be32(stsz + 12 + i * 4);
    for (uint32_t i = 0; i < stco_count && i < (stco_size - 8) / 4; i++)
        offsets[i] = be32(stco + 8 + i * 4);

    /* Map samples to chunks using stsc */
    uint32_t sample_index = 0;
    for (uint32_t c = 0; c < stsc_count && sample_index < stsz_count; c++) {
        uint32_t first_chunk = be32(stsc + 8 + c * 12);
        uint32_t samples_per_chunk = be32(stsc + 8 + c * 12 + 4);
        uint32_t next_first = (c + 1 < stsc_count) ? be32(stsc + 8 + (c+1) * 12) : stco_count + 1;

        for (uint32_t chunk = first_chunk; chunk < next_first && chunk <= stco_count && sample_index < stsz_count; chunk++) {
            uint32_t chunk_offset = offsets[chunk - 1];
            for (uint32_t k = 0; k < samples_per_chunk && sample_index < stsz_count; k++) {
                uint32_t sample_size = sizes[sample_index];
                if (chunk_offset + sample_size > mdat_size) break;

                const uint8_t *sample = mdat + chunk_offset;
                uint32_t pos = 0;

                /* Convert AVCC (length-prefixed NAL units) to Annex B */
                while (pos + 4 <= sample_size) {
                    uint32_t nal_len = be32(sample + pos);
                    pos += 4;
                    if (pos + nal_len > sample_size) break;
                    if (append_start_code(s) != 0) { free(sizes); free(offsets); return -1; }
                    if (stream_append(s, sample + pos, nal_len) != 0) { free(sizes); free(offsets); return -1; }
                    pos += nal_len;
                }

                chunk_offset += sample_size;
                sample_index++;
            }
        }
    }

    free(sizes);
    free(offsets);
    return 0;
}

int mp4_extract_h264(const uint8_t *mp4, size_t mp4_size,
                     uint8_t **out_stream, size_t *out_size) {
    if (!mp4 || !out_stream || !out_size || mp4_size < 8) return -1;

    size_t moov_size = 0, mdat_size = 0;
    const uint8_t *moov = find_box(mp4, mp4_size, "moov", &moov_size);
    const uint8_t *mdat = find_box(mp4, mp4_size, "mdat", &mdat_size);
    if (!moov || !mdat) return -2;

    /* Find stbl inside moov/trak/mdia/minf/stbl */
    size_t trak_size = 0;
    const uint8_t *trak = find_box_in(moov, moov_size, "trak", &trak_size);
    if (!trak) return -3;

    size_t mdia_size = 0;
    const uint8_t *mdia = find_box_in(trak, trak_size, "mdia", &mdia_size);
    if (!mdia) return -3;

    size_t minf_size = 0;
    const uint8_t *minf = find_box_in(mdia, mdia_size, "minf", &minf_size);
    if (!minf) return -3;

    size_t stbl_size = 0;
    const uint8_t *stbl = find_box_in(minf, minf_size, "stbl", &stbl_size);
    if (!stbl) return -3;

    /* Build output stream */
    mp4_stream_t out = {0};
    int ret = -1;

    /* SPS/PPS first */
    size_t stsd_size = 0;
    const uint8_t *stsd = find_box_in(stbl, stbl_size, "stsd", &stsd_size);
    if (!stsd) goto done;
    if (extract_avcc(&out, stsd, stsd_size) != 0) goto done;

    /* Samples */
    if (extract_samples(&out, stbl, stbl_size, mdat, mdat_size) != 0) goto done;

    *out_stream = out.data;
    *out_size = out.size;
    ret = 0;

done:
    if (ret != 0 && out.data) free(out.data);
    return ret;
}
