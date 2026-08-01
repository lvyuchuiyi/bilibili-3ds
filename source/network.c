#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <3ds.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/error.h>
#include "network.h"

int net_debug_status = 0;  /* 0=OK, negative=init error */
int net_debug_http_ret = 0; /* last http_get return code */
int net_debug_http_status = 0; /* HTTP status code */
int net_debug_stage = 0;               /* 0=start 1=seed 2=connect 3=config 4=handshake 5=send 6=read 7=done */

static u32 *soc_mem = NULL;
static bool soc_initialized = false;
static bool ac_initialized = false;

int net_init(void) {
    if (soc_initialized) return 0;
    net_debug_status = 0;

    if (R_SUCCEEDED(acInit())) ac_initialized = true;

    soc_mem = (u32*)memalign(0x1000, 0x100000);
    if (!soc_mem) {
        net_debug_status = -1;
        return -1;
    }

    Result r = socInit(soc_mem, 0x100000);
    if (R_FAILED(r)) {
        net_debug_status = (int)r;  /* show actual Result code */
        free(soc_mem);
        soc_mem = NULL;
        return -2;
    }
    soc_initialized = true;
    return 0;
}

void net_exit(void) {
    if (soc_initialized) {
        socExit();
        if (soc_mem) {
            free(soc_mem);
            soc_mem = NULL;
        }
        soc_initialized = false;
    }
    if (ac_initialized) {
        acExit();
        ac_initialized = false;
    }
}

static int parse_url(const char *url, char *host, int host_max,
                     char *path, int path_max, char *port, int port_max) {
    const char *p = url;

    if (strncmp(p, "https://", 8) == 0) p += 8;
    else if (strncmp(p, "http://", 7) == 0) p += 7;

    const char *host_start = p;
    while (*p && *p != '/' && *p != ':' && *p != '?') p++;

    int host_len = p - host_start;
    if (host_len >= host_max) return -1;
    memcpy(host, host_start, host_len);
    host[host_len] = '\0';

    if (*p == ':') {
        p++;
        const char *port_start = p;
        while (*p && *p != '/') p++;
        int port_len = p - port_start;
        if (port_len >= port_max) return -1;
        memcpy(port, port_start, port_len);
        port[port_len] = '\0';
    } else {
        strcpy(port, "443");
    }

    if (*p == '\0') {
        strcpy(path, "/");
    } else {
        int path_len = strlen(p);
        if (path_len >= path_max) return -1;
        memcpy(path, p, path_len);
        path[path_len] = '\0';
    }
    return 0;
}

int http_get(const char *url, http_response_t *resp) {
    if (!url || !resp) return -1;

    char host[256], path[1024], port[16];
    if (parse_url(url, host, sizeof(host), path, sizeof(path), port, sizeof(port)) != 0)
        return -1;

    mbedtls_net_context net_ctx;
    mbedtls_ssl_context ssl_ctx;
    mbedtls_ssl_config ssl_conf;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;

    mbedtls_net_init(&net_ctx);
    mbedtls_ssl_init(&ssl_ctx);
    mbedtls_ssl_config_init(&ssl_conf);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    int ret = -1;
    net_debug_stage = 0;

    do {
        ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func,
                                     &entropy, NULL, 0);
        if (ret != 0) break;
        net_debug_stage = 1;

        ret = mbedtls_net_connect(&net_ctx, host, port,
                                   MBEDTLS_NET_PROTO_TCP);
        if (ret != 0) break;
        net_debug_stage = 2;

        ret = mbedtls_ssl_config_defaults(&ssl_conf,
                    MBEDTLS_SSL_IS_CLIENT,
                    MBEDTLS_SSL_TRANSPORT_STREAM,
                    MBEDTLS_SSL_PRESET_DEFAULT);
        if (ret != 0) break;
        net_debug_stage = 3;

        mbedtls_ssl_conf_authmode(&ssl_conf, MBEDTLS_SSL_VERIFY_NONE);
        mbedtls_ssl_conf_rng(&ssl_conf, mbedtls_ctr_drbg_random, &ctr_drbg);
        mbedtls_ssl_conf_read_timeout(&ssl_conf, 5000);

        ret = mbedtls_ssl_setup(&ssl_ctx, &ssl_conf);
        if (ret != 0) break;

        ret = mbedtls_ssl_set_hostname(&ssl_ctx, host);
        if (ret != 0) break;

        mbedtls_ssl_set_bio(&ssl_ctx, &net_ctx,
                            mbedtls_net_send, NULL,
                            mbedtls_net_recv_timeout);

        ret = mbedtls_ssl_handshake(&ssl_ctx);
        if (ret != 0) break;
        net_debug_stage = 4;

        char request[2048];
        snprintf(request, sizeof(request),
            "GET %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "User-Agent: Mozilla/5.0 (Linux; U; Android 4.4; 3DS) "
            "AppleWebKit/537.36 BiliApp/1.0\r\n"
            "Referer: https://www.bilibili.com/client\r\n"
            "Origin: https://www.bilibili.com\r\n"
            "Accept: application/json, text/plain, */*\r\n"
            "Connection: close\r\n"
            "\r\n",
            path, host);

        ret = mbedtls_ssl_write(&ssl_ctx,
                                 (unsigned char*)request, strlen(request));
        if (ret <= 0) { ret = -1; break; }
        net_debug_stage = 5;

        resp->buf = malloc(NET_BUF_SIZE);
        if (!resp->buf) { ret = -1; break; }
        resp->buf_size = NET_BUF_SIZE;
        resp->data_len = 0;
        resp->parse_pos = 0;

        net_debug_stage = 6;
        while (1) {
            int remaining = resp->buf_size - resp->data_len;
            if (remaining <= 0) {
                resp->buf_size *= 2;
                char *nb = realloc(resp->buf, resp->buf_size);
                if (!nb) { ret = -1; break; }
                resp->buf = nb;
                remaining = resp->buf_size - resp->data_len;
            }

            ret = mbedtls_ssl_read(&ssl_ctx,
                (unsigned char*)resp->buf + resp->data_len,
                remaining - 1);
            if (ret == MBEDTLS_ERR_SSL_WANT_READ ||
                ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
                svcSleepThread(1000000);  /* 1ms yield to avoid busy loop */
                continue;
            }
            if (ret <= 0) {
                ret = 0;
                break;
            }
            resp->data_len += ret;
        }
        resp->buf[resp->data_len] = '\0';
        /* Parse HTTP status line */
        if (sscanf(resp->buf, "HTTP/%*s %d", &net_debug_http_status) != 1)
            net_debug_http_status = 0;

    } while (0);

    mbedtls_ssl_free(&ssl_ctx);
    mbedtls_ssl_config_free(&ssl_conf);
    mbedtls_net_free(&net_ctx);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    if (ret != 0 && resp->buf) {
        free(resp->buf);
        resp->buf = NULL;
        net_debug_http_ret = ret;
        return ret;
    }
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







