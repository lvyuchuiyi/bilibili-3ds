#ifndef _MP4_H_
#define _MP4_H_

#include <stdint.h>
#include <stddef.h>

/* Minimal MP4 H.264 demuxer.
 * Extracts an Annex B (start-code prefixed) H.264 stream from an MP4 file.
 */

typedef struct {
    uint8_t *data;
    size_t size;
    size_t capacity;
} mp4_stream_t;

/* Parse MP4 buffer and build Annex B stream.
 * Returns 0 on success, negative on error.
 * The returned stream must be freed with free(). */
int mp4_extract_h264(const uint8_t *mp4, size_t mp4_size,
                     uint8_t **out_stream, size_t *out_size);

#endif
