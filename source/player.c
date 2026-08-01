/*
 * player.c - MVD H.264 hardware video decoder for 3DS
 * Downloads MP4, demuxes to Annex B H.264, decodes with MVD,
 * renders to a C2D texture.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <3ds.h>
#include <citro2d.h>
#include "player.h"
#include "network.h"
#include "mp4.h"

#define MVD_WORKBUF_SIZE MVD_DEFAULT_WORKBUF_SIZE
#define PLAY_WIDTH 400
#define PLAY_HEIGHT 240

static MVDSTD_Config mvd_config;
static u8 *workbuf = NULL;
static u8 *output_buf = NULL;
static bool mvd_initialized = false;

static player_info_t p_info;
static u8 *h264_data = NULL;
static int h264_size = 0;
static int h264_offset = 0;
static int decoded_frames = 0;

static C3D_Tex video_tex;
static C2D_Image video_image;
static bool tex_ready = false;

int player_debug_state = -1;
int player_debug_init = -1;
int player_debug_load = -1;
int player_debug_h264 = 0;

int player_init(void) {
    if (mvd_initialized) return 0;
    memset(&p_info, 0, sizeof(p_info));
    p_info.state = PLAYER_IDLE;
    p_info.width = PLAY_WIDTH;
    p_info.height = PLAY_HEIGHT;

    workbuf = (u8*)linearAlloc(MVD_WORKBUF_SIZE);
    output_buf = (u8*)linearAlloc(PLAY_WIDTH * PLAY_HEIGHT * 2);
    if (!workbuf || !output_buf) {
        if (workbuf) linearFree(workbuf);
        if (output_buf) linearFree(output_buf);
        return -2;
    }

    Result r = mvdstdInit(MVDMODE_VIDEOPROCESSING, MVD_INPUT_H264,
                   MVD_OUTPUT_RGB565, MVD_WORKBUF_SIZE, NULL);
    player_debug_init = (int)r;
    if (R_FAILED(r)) {
        linearFree(workbuf);
        linearFree(output_buf);
        return -3;
    }

    u32 out0_addr = (u32)(uintptr_t)output_buf;
    mvdstdGenerateDefaultConfig(&mvd_config, PLAY_WIDTH, PLAY_HEIGHT,
                                PLAY_WIDTH, PLAY_HEIGHT,
                                NULL, &out0_addr, NULL);
    mvd_config.physaddr_outdata0 = osConvertVirtToPhys(output_buf);
    mvd_config.outwidth = PLAY_WIDTH;
    mvd_config.outheight = PLAY_HEIGHT;

    MVDSTD_SetConfig(&mvd_config);
    mvd_initialized = true;

    /* C2D texture for video output */
    if (C3D_TexInit(&video_tex, PLAY_WIDTH, PLAY_HEIGHT, GPU_RGB565)) {
        C3D_TexSetFilter(&video_tex, GPU_LINEAR, GPU_LINEAR);
        video_image.tex = &video_tex;
        video_image.subtex = NULL;
        tex_ready = true;
    }
    return 0;
}

void player_exit(void) {
    player_stop();
    if (tex_ready) {
        C3D_TexDelete(&video_tex);
        tex_ready = false;
    }
    if (mvd_initialized) {
        mvdstdExit();
        if (workbuf) { linearFree(workbuf); workbuf = NULL; }
        if (output_buf) { linearFree(output_buf); output_buf = NULL; }
        mvd_initialized = false;
    }
}

int player_load(const char *url) {
    if (!url || !mvd_initialized) return -1;
    strncpy(p_info.url, url, sizeof(p_info.url) - 1);
    p_info.state = PLAYER_LOADING;
    decoded_frames = 0;
    h264_offset = 0;

    /* Download MP4 */
    http_response_t resp = {0};
    int ret = http_get(url, &resp);
    if (ret != 0 || !resp.buf || resp.data_len < 1024) {
        p_info.state = PLAYER_ERROR;
        if (resp.buf) http_response_free(&resp);
        return -2;
    }

    /* Extract H.264 Annex B stream */
    uint8_t *stream = NULL;
    size_t stream_size = 0;
    ret = mp4_extract_h264((const uint8_t*)resp.buf, resp.data_len,
                           &stream, &stream_size);
    http_response_free(&resp);
    player_debug_load = ret;
    player_debug_h264 = (int)stream_size;
    if (ret != 0 || !stream || stream_size < 64) {
        if (stream) free(stream);
        p_info.state = PLAYER_ERROR;
        return -3;
    }

    if (h264_data) { free(h264_data); h264_data = NULL; }
    h264_data = stream;
    h264_size = (int)stream_size;
    h264_offset = 0;

    p_info.state = PLAYER_PLAYING;
    p_info.total_frames = 0;
    p_info.current_frame = 0;
    return 0;
}

void player_play(void) {
    if (mvd_initialized && p_info.state != PLAYER_ERROR)
        p_info.state = PLAYER_PLAYING;
}

void player_pause(void) {
    if (p_info.state == PLAYER_PLAYING)
        p_info.state = PLAYER_PAUSED;
    else if (p_info.state == PLAYER_PAUSED)
        p_info.state = PLAYER_PLAYING;
}

void player_stop(void) {
    if (h264_data) { free(h264_data); h264_data = NULL; }
    h264_size = 0;
    h264_offset = 0;
    decoded_frames = 0;
    memset(&p_info, 0, sizeof(p_info));
    p_info.state = PLAYER_IDLE;
}

/* Find start code in Annex B stream */
static int find_start_code(const u8 *data, int size, int offset) {
    for (int i = offset; i < size - 3; i++) {
        if (data[i] == 0 && data[i+1] == 0) {
            if (data[i+2] == 1) return i;
            if (i < size - 4 && data[i+2] == 0 && data[i+3] == 1) return i;
        }
    }
    return -1;
}

player_state_t player_update(void) {
    player_debug_state = (int)p_info.state;
    if (p_info.state != PLAYER_PLAYING || !mvd_initialized)
        return p_info.state;
    if (!h264_data || h264_offset >= h264_size - 4) {
        p_info.state = PLAYER_DONE;
        return p_info.state;
    }

    int start = find_start_code(h264_data, h264_size, h264_offset);
    if (start < 0) { p_info.state = PLAYER_DONE; return p_info.state; }

    int next = find_start_code(h264_data, h264_size, start + 4);
    int end = (next > 0) ? next : h264_size;

    int nal_start = start;
    if (h264_data[start+2] == 0 && h264_data[start+3] == 1)
        nal_start = start + 4;
    else if (h264_data[start+2] == 1)
        nal_start = start + 3;

    int nal_size = end - nal_start;
    if (nal_size <= 0) { h264_offset = end; return p_info.state; }
    if (nal_size > MVD_WORKBUF_SIZE - 4096) nal_size = MVD_WORKBUF_SIZE - 4096;

    memcpy(workbuf, h264_data + nal_start, nal_size);
    GSPGPU_FlushDataCache(workbuf, nal_size);

    u32 nal_type = h264_data[nal_start] & 0x1F;
    u32 flag = (nal_type == 5 || nal_type == 7 || nal_type == 8) ? 1 : 0;

    MVDSTD_ProcessNALUnitOut out;
    Result r = mvdstdProcessVideoFrame(workbuf, nal_size, flag, &out);
    if (MVD_CHECKNALUPROC_SUCCESS(r)) {
        if (r == MVD_STATUS_FRAMEREADY) {
            mvdstdRenderVideoFrame(NULL, false);
            decoded_frames++;
            p_info.current_frame = decoded_frames;
        }
    }

    h264_offset = end;
    p_info.total_frames = decoded_frames;
    p_info.progress = h264_size > 0
        ? (float)h264_offset / h264_size : 0.0f;

    if (h264_offset >= h264_size)
        p_info.state = PLAYER_DONE;

    return p_info.state;
}

/* Upload latest frame to C2D texture and draw on top screen */
void player_render(void) {
    if (!tex_ready || !mvd_initialized) return;
    if (p_info.state != PLAYER_PLAYING && p_info.state != PLAYER_PAUSED) return;

    memcpy(video_tex.data, output_buf, PLAY_WIDTH * PLAY_HEIGHT * 2);
    C3D_TexFlush(&video_tex);
    C2D_DrawImageAt(video_image, 0, 0, 0.5f, NULL, 1.0f, 1.0f);
}

void player_get_info(player_info_t *info) {
    if (info) memcpy(info, &p_info, sizeof(player_info_t));
}


