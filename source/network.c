#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <3ds.h>
#include "network.h"

int net_debug_status = 0;  /* 0=OK, negative=init error */
int net_debug_http_ret = 0; /* last http_get return code */
int net_debug_http_status = 0; /* HTTP status code */
int net_debug_stage = 0;       /* 0=start 1=context 2=request 3=status 4=download 5=done */
char net_debug_raw[64] = "";         /* first bytes of response */
int net_debug_sslc_ret = 0;          /* last download result */

static bool httpc_initialized = false;

int net_init(void) {
    if (httpc_initialized) return 0;
    net_debug_status = 0;

    /* http:C service handles HTTPS internally - no SOC needed */
    if (R_SUCCEEDED(httpcInit(0))) httpc_initialized = true;
    else {
        net_debug_status = -1;
        return -1;
    }
    return 0;
}

void net_exit(void) {
    if (httpc_initialized) {
        httpcExit();
        httpc_initialized = false;
    }
}

int http_get(const char *url, http_response_t *resp) {
    if (!url || !resp) return -1;

    net_debug_stage = 0;
    httpcContext context;

    Result r = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 0);
    if (R_FAILED(r)) {
        net_debug_http_ret = -1;
        return -1;
    }
    net_debug_stage = 1;

    /* Disable SSL cert verification - homebrew has no CA bundle */
    r = httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);
    if (R_FAILED(r)) {
        httpcCloseContext(&context);
        net_debug_http_ret = -2;
        return -2;
    }

    /* Bilibili-friendly headers */
    httpcAddRequestHeaderField(&context, "User-Agent",
        "Mozilla/5.0 (Linux; U; Android 4.4; 3DS) AppleWebKit/537.36 BiliApp/1.0");
    httpcAddRequestHeaderField(&context, "Referer",
        "https://www.bilibili.com/client");
    httpcAddRequestHeaderField(&context, "Origin",
        "https://www.bilibili.com");
    httpcAddRequestHeaderField(&context, "Accept",
        "application/json, text/plain, */*");
    httpcAddRequestHeaderField(&context, "Connection", "close");

    r = httpcBeginRequest(&context);
    if (R_FAILED(r)) {
        httpcCloseContext(&context);
        net_debug_http_ret = -3;
        return -3;
    }
    net_debug_stage = 2;

    r = httpcGetResponseStatusCode(&context, (u32*)&net_debug_http_status);
    if (R_FAILED(r)) {
        httpcCloseContext(&context);
        net_debug_http_ret = -4;
        return -4;
    }
    net_debug_stage = 3;

    /* Read response */
    resp->buf = malloc(NET_BUF_SIZE);
    if (!resp->buf) {
        httpcCloseContext(&context);
        net_debug_http_ret = -5;
        return -5;
    }
    resp->buf_size = NET_BUF_SIZE;
    resp->data_len = 0;
    resp->parse_pos = 0;

    net_debug_stage = 4;
    while (1) {
        int remaining = resp->buf_size - resp->data_len;
        if (remaining <= 1) {
            resp->buf_size *= 2;
            char *nb = realloc(resp->buf, resp->buf_size);
            if (!nb) { net_debug_http_ret = -6; break; }
            resp->buf = nb;
            remaining = resp->buf_size - resp->data_len;
        }

        u32 readsize = 0;
        r = httpcDownloadData(&context, (u8*)resp->buf + resp->data_len,
                              remaining - 1, &readsize);
        net_debug_sslc_ret = (int)r;
        resp->data_len += (int)readsize;

        if (r == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING) continue;
        break;
    }
    resp->buf[resp->data_len] = '\0';

    /* Save first bytes for debug */
    strncpy(net_debug_raw, resp->buf, sizeof(net_debug_raw) - 1);
    net_debug_raw[sizeof(net_debug_raw) - 1] = 0;

    httpcCloseContext(&context);

    net_debug_stage = 5;
    net_debug_http_ret = 0;
    return 0;
}

void http_response_free(http_response_t *resp) {
    if (resp && resp->buf) {
        free(resp->buf);
        resp->buf = NULL;
        resp->data_len = 0;
        resp->buf_size = 0;
    }
}
