#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <3ds.h>
#include <curl/curl.h>
#include "network.h"

#define SOC_BUFSIZE (1024U * 1024U)

static void *soc_buf = NULL;
static bool net_ready = false;

int net_init(void) {
    if (net_ready) return 0;

    /* ?? socInit????? curl */
    soc_buf = linearAlloc(SOC_BUFSIZE);
    if (!soc_buf) return -1;

    Result r = socInit(soc_buf, SOC_BUFSIZE);
    if (R_FAILED(r)) {
        linearFree(soc_buf);
        soc_buf = NULL;
        return -2;
    }

    net_ready = true;
    return 0;
}

void net_exit(void) {
    if (net_ready) {
        socExit();
        if (soc_buf) { linearFree(soc_buf); soc_buf = NULL; }
        net_ready = false;
    }
}

int http_get(const char *url, http_response_t *resp) {
    (void)url; (void)resp;
    return -99;
}

void http_response_free(http_response_t *resp) {
    (void)resp;
}
