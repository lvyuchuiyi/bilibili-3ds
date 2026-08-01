/*
 * player.c - MVD H.264 hardware video decoder for 3DS
 * Uses the new MVDSTD API: mvdstdInit, mvdstdGenerateDefaultConfig, 
 * mvdstdProcessVideoFrame, mvdstdRenderVideoFrame
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <3ds.h>
#include <citro2d.h>
#include "player.h"
#include "network.h"

#define MVD_WORKBUF_SIZE MVD_DEFAULT_WORKBUF_SIZE

static MVDSTD_Config mvd_config;
static u8 *workbuf = NULL;
static u8 *output_buf = NULL;
static bool mvd_initialized = false;

static player_info_t p_info;
static u8 *video_data = NULL;
static int video_data_size = 0;
static int nal_offset = 0;
static int decoded_frames = 0;

int player_init(void) {
    if (mvd_initialized) return 0;
    memset(&p_info, 0, sizeof(p_info));
    p_info.state = PLAYER_IDLE;
    p_info.width = 400;
    p_info.height = 240;

    workbuf = (u8*)linearAlloc(MVD_WORKBUF_SIZE);
    output_buf = (u8*)linearAlloc(400 * 240 * 2);
    if (!workbuf || !output_buf) {
        if (workbuf) linearFree(workbuf);
        if (output_buf) linearFree(output_buf);
        return -2;
    }

    Result r = mvdstdInit(MVDMODE_VIDEOPROCESSING, MVD_INPUT_H264,
                   MVD_OUTPUT_RGB565, MVD_WORKBUF_SIZE, NULL);
    if (R_FAILED(r)) {
        linearFree(workbuf);
        linearFree(output_buf);
        return -3;
    }

    u32 out0_addr = (u32)(uintptr_t)output_buf;
    mvdstdGenerateDefaultConfig(&mvd_config, 400, 240, 400, 240,
                                NULL, &out0_addr, NULL);
    mvd_config.physaddr_outdata0 = osConvertVirtToPhys(output_buf);
    mvd_config.outwidth = 400;
    mvd_config.outheight = 240;

    MVDSTD_SetConfig(&mvd_config);
    mvd_initialized = true;
    return 0;
}

void player_exit(void) {
    player_stop();
    if (mvd_initialized) {
        mvdstdExit();
        /* mvdExit not needed */
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
    nal_offset = 0;

    http_response_t resp = {0};
    int ret = http_get(url, &resp);
    if (ret != 0 || !resp.buf || resp.data_len < 1024) {
        p_info.state = PLAYER_ERROR;
        if (resp.buf) http_response_free(&resp);
        return -2;
    }

    /* Find body start */
    u8 *body = (u8*)resp.buf;
    int body_size = resp.data_len;
    char *hdr_end = strstr(resp.buf, "\r\n\r\n");
    if (hdr_end) {
        body = (u8*)(hdr_end + 4);
        body_size = resp.data_len - (int)((char*)body - resp.buf);
    }

    if (video_data) { linearFree(video_data); video_data = NULL; }
    video_data = linearAlloc(body_size);
    if (!video_data) { http_response_free(&resp); return -3; }
    memcpy(video_data, body, body_size);
    video_data_size = body_size;
    http_response_free(&resp);

    p_info.state = PLAYER_PLAYING;
    p_info.total_frames = 0;
    p_info.current_frame = 0;
    nal_offset = 0;
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
    if (video_data) { linearFree(video_data); video_data = NULL; }
    video_data_size = 0;
    nal_offset = 0;
    decoded_frames = 0;
    memset(&p_info, 0, sizeof(p_info));
    p_info.state = PLAYER_IDLE;
}

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
    if (p_info.state != PLAYER_PLAYING || !mvd_initialized)
        return p_info.state;
    if (!video_data || nal_offset >= video_data_size - 4) {
        p_info.state = PLAYER_DONE;
        return p_info.state;
    }

    /* Find next NAL unit */
    int start = find_start_code(video_data, video_data_size, nal_offset);
    if (start < 0) { p_info.state = PLAYER_DONE; return p_info.state; }

    int next = find_start_code(video_data, video_data_size, start + 4);
    int end = (next > 0) ? next : video_data_size;

    /* Skip start code */
    int nal_start = start;
    if (video_data[start+2] == 0 && video_data[start+3] == 1)
        nal_start = start + 4;
    else if (video_data[start+2] == 1)
        nal_start = start + 3;

    int nal_size = end - nal_start;
    if (nal_size <= 0) { nal_offset = end; return p_info.state; }

    /* Copy NAL to linear memory for MVD */
    if (nal_size > MVD_WORKBUF_SIZE - 4096) nal_size = MVD_WORKBUF_SIZE - 4096;
    memcpy(workbuf, video_data + nal_start, nal_size);

    /* Process NAL unit */
    u32 nal_type = video_data[nal_start] & 0x1F;
    u32 flag = (nal_type == 5 || nal_type == 7 || nal_type == 8) ? 1 : 0;

    MVDSTD_ProcessNALUnitOut out;
    Result r = mvdstdProcessVideoFrame(workbuf, nal_size, flag, &out);
    if (MVD_CHECKNALUPROC_SUCCESS(r)) {
        if (r == MVD_STATUS_FRAMEREADY) {
            /* Render the decoded frame */
            mvdstdRenderVideoFrame(NULL, true);
            decoded_frames++;
            p_info.current_frame = decoded_frames;
        }
    }

    nal_offset = end;
    p_info.total_frames = decoded_frames;
    p_info.progress = video_data_size > 0
        ? (float)nal_offset / video_data_size : 0.0f;

    if (nal_offset >= video_data_size)
        p_info.state = PLAYER_DONE;

    return p_info.state;
}

void player_get_info(player_info_t *info) {
    if (info) memcpy(info, &p_info, sizeof(player_info_t));
}

