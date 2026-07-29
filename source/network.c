#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <3ds.h>
#include <curl/curl.h>
#include "network.h"

/* libcurl for 3DS may have a static constructor (runs before main) that
 * calls socInit.  We handle both cases:
 *   - Constructor present: socInit already active when we start,
 *     only call curl_global_init to finish libcurl setup.
 *   - Constructor absent: call socInit ourselves, then curl_global_init.
 *
 * The weak globals __curl_soc_buffer / __curl_soc_buffer_size tell the
 * constructor (if it exists) how much linear memory to allocate for SOC. */
#define SOC_BUFSIZE (1024U * 1024U)

void *__curl_soc_buffer = NULL;
u32   __curl_soc_buffer_size = SOC_BUFSIZE;

static bool curl_ready = false;
static bool soc_ours = false;  /* did we allocate the buffer ourselves? */

struct write_mem {
    char *data;
    size_t size;
    size_t cap;
};

static size_t write_cb(char *ptr, size_t sz, size_t n, void *user) {
    struct write_mem *m = (struct write_mem *)user;
    size_t total = sz * n;
    if (m->size + total >= m->cap) {
        m->cap = m->cap ? m->cap * 2 : 65536;
        char *p = realloc(m->data, m->cap);
        if (!p) return 0;
        m->data = p;
    }
    memcpy(m->data + m->size, ptr, total);
    m->size += total;
    m->data[m->size] = 0;
    return total;
}

int net_init(void) {
    if (curl_ready) return 0;

    /* Try to init SOC ourselves. If constructor already did it
     * (newer devkitPro libcurl with constructor), socInit fails
     * harmlessly and SOC is already active. Either way we proceed. */
    void *buf = linearAlloc(SOC_BUFSIZE);
    if (buf) {
        Result r = socInit(buf, SOC_BUFSIZE);
        if (R_SUCCEEDED(r)) {
            __curl_soc_buffer = buf;
            soc_ours = true;
        } else {
            linearFree(buf);  /* constructor already init''ed SOC */
        }
    }

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
        return -3;
    curl_ready = true;
    return 0;
}

void net_exit(void) {
    if (curl_ready) { curl_global_cleanup(); curl_ready = false; }
    socExit();
    if (soc_ours && __curl_soc_buffer)
        linearFree(__curl_soc_buffer);
    __curl_soc_buffer = NULL;
}

int http_get(const char *url, http_response_t *resp) {
    if (!url || !resp) return -1;

    CURL *curl = curl_easy_init();
    if (!curl) return -1;

    struct write_mem chunk = {0};
    char errbuf[CURL_ERROR_SIZE] = {0};
    long status = 0;
    int ret = -1;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT,
        "Mozilla/5.0 (Linux; U; Android 4.4; 3DS) AppleWebKit/537.36 BiliApp/1.0");
    curl_easy_setopt(curl, CURLOPT_REFERER, "https://www.bilibili.com/");

    CURLcode code = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

    if (code == CURLE_OK && status >= 200 && status < 300 && chunk.data) {
        resp->buf = chunk.data;
        resp->buf_size = chunk.cap;
        resp->data_len = chunk.size;
        resp->parse_pos = 0;
        ret = 0;
    } else {
        if (chunk.data) free(chunk.data);
    }
    curl_easy_cleanup(curl);
    return ret;
}

void http_response_free(http_response_t *resp) {
    if (resp && resp->buf) {
        free(resp->buf);
        resp->buf = NULL;
        resp->data_len = 0;
        resp->buf_size = 0;
    }
}
