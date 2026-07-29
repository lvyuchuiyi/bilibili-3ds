#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <3ds.h>
#include <curl/curl.h>
#include "network.h"

#define SOC_ALIGN 0x1000U
#define SOC_BUFSIZE (1024U * 1024U)

static bool ac_ready = false;
static u32 *soc_mem = NULL;
static bool soc_ready = false;
static bool curl_ready = false;

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
    if (soc_ready) return 0;
    if (R_SUCCEEDED(acInit())) ac_ready = true;
    soc_mem = (u32*)linearAlloc(SOC_BUFSIZE);
    if (!soc_mem) return -1;
    if (R_FAILED(socInit(soc_mem, SOC_BUFSIZE))) {
        linearFree(soc_mem); soc_mem = NULL; return -2;
    }
    soc_ready = true;
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) return -3;
    curl_ready = true;
    return 0;
}

void net_exit(void) {
    if (curl_ready) { curl_global_cleanup(); curl_ready = false; }
    if (soc_ready) { socExit(); soc_ready = false; }
    if (soc_mem) { linearFree(soc_mem); soc_mem = NULL; }
    if (ac_ready) { acExit(); ac_ready = false; }
}

static struct curl_slist *add_headers(struct curl_slist *h, const char *v) {
    return curl_slist_append(h, v);
}

int http_get(const char *url, http_response_t *resp) {
    if (!url || !resp) return -1;
    CURL *curl = curl_easy_init();
    if (!curl) return -1;

    struct write_mem chunk = {0};
    long status = 0;
    int ret = -1;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Linux; Android 10; K) AppleWebKit/537.36 BiliApp/1.0");
    curl_easy_setopt(curl, CURLOPT_REFERER, "https://www.bilibili.com/");

    struct curl_slist *headers = NULL;
    headers = add_headers(headers, "Accept: application/json, text/plain, */*");
    headers = add_headers(headers, "Origin: https://www.bilibili.com");
    headers = add_headers(headers, "Accept-Language: zh-CN,zh;q=0.9");
    /* Generic buvid3 cookie - Bilibili might need this */
    headers = add_headers(headers, "Cookie: buvid3=test0123456789; fingerprint=test");
    if (headers) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

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
    if (headers) curl_slist_free_all(headers);
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
