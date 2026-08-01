#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <3ds.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "network.h"

int net_debug_status = 0;  /* 0=OK, negative=init error */
int net_debug_http_ret = 0; /* last http_get return code */
int net_debug_http_status = 0; /* HTTP status code */
int net_debug_stage = 0;       /* 0=start 1=socket 2=connect 3=ssl 4=send 5=read 6=done */
char net_debug_raw[64] = "";         /* first bytes of response */
int net_debug_sslc_ret = 0;                /* last sslcRead result */

static u32 *soc_mem = NULL;
static bool soc_initialized = false;
static bool ac_initialized = false;
static bool sslc_initialized = false;

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
        net_debug_status = (int)r;
        free(soc_mem);
        soc_mem = NULL;
        return -2;
    }
    soc_initialized = true;

    if (R_SUCCEEDED(sslcInit(0))) sslc_initialized = true;
    return 0;
}

void net_exit(void) {
    if (sslc_initialized) {
        sslcExit();
        sslc_initialized = false;
    }
    if (soc_initialized) {
        socExit();
        soc_initialized = false;
    }
    if (soc_mem) {
        free(soc_mem);
        soc_mem = NULL;
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

    net_debug_stage = 0;

    /* Create socket */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        net_debug_http_ret = -1;
        return -1;
    }
    net_debug_stage = 1;

    /* Resolve hostname */
    struct addrinfo hints, *resaddr = NULL, *cur;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &resaddr) != 0) {
        closesocket(sockfd);
        net_debug_http_ret = -2;
        return -2;
    }

    /* Connect */
    int connected = 0;
    for (cur = resaddr; cur; cur = cur->ai_next) {
        if (connect(sockfd, cur->ai_addr, (socklen_t)cur->ai_addrlen) == 0) {
            connected = 1;
            break;
        }
    }
    freeaddrinfo(resaddr);
    if (!connected) {
        closesocket(sockfd);
        net_debug_http_ret = -3;
        return -3;
    }
    net_debug_stage = 2;

    /* TLS via native sslc service */
    sslcContext sslc_ctx;
    Result r = sslcCreateContext(&sslc_ctx, sockfd, SSLCOPT_DisableVerify, host);
    if (R_FAILED(r)) {
        closesocket(sockfd);
        net_debug_http_ret = -4;
        return -4;
    }

    r = sslcStartConnection(&sslc_ctx, NULL, NULL);
    if (R_FAILED(r)) {
        sslcDestroyContext(&sslc_ctx);
        closesocket(sockfd);
        net_debug_http_ret = -5;
        return -5;
    }
    net_debug_stage = 3;

    /* Build and send HTTP request */
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

    r = sslcWrite(&sslc_ctx, request, strlen(request));
    if (R_FAILED(r) || (int)r <= 0) {
        sslcDestroyContext(&sslc_ctx);
        closesocket(sockfd);
        net_debug_http_ret = -6;
        return -6;
    }
    net_debug_stage = 4;

    /* Read response */
    resp->buf = malloc(NET_BUF_SIZE);
    if (!resp->buf) {
        sslcDestroyContext(&sslc_ctx);
        closesocket(sockfd);
        net_debug_http_ret = -7;
        return -7;
    }
    resp->buf_size = NET_BUF_SIZE;
    resp->data_len = 0;
    resp->parse_pos = 0;

    net_debug_stage = 5;
    while (1) {
        int remaining = resp->buf_size - resp->data_len;
        if (remaining <= 1) {
            resp->buf_size *= 2;
            char *nb = realloc(resp->buf, resp->buf_size);
            if (!nb) { net_debug_http_ret = -8; break; }
            resp->buf = nb;
            remaining = resp->buf_size - resp->data_len;
        }

        r = sslcRead(&sslc_ctx, resp->buf + resp->data_len, remaining - 1, false);
        net_debug_sslc_ret = (int)r;
        if (R_FAILED(r)) break;  /* EOF or error: response complete */
        if ((int)r == 0) break;
        resp->data_len += (int)r;
    }
    resp->buf[resp->data_len] = '\0';

    /* Parse HTTP status line */
    if (sscanf(resp->buf, "HTTP/%*s %d", &net_debug_http_status) != 1)
        net_debug_http_status = 0;

    strncpy(net_debug_raw, resp->buf, sizeof(net_debug_raw) - 1);
    net_debug_raw[sizeof(net_debug_raw) - 1] = 0;

    sslcDestroyContext(&sslc_ctx);
    closesocket(sockfd);

    net_debug_stage = 6;
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


