#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <stdint.h>

#define NET_BUF_SIZE 32768

typedef struct {
    int sock;
    char *buf;
    int buf_size;
    int data_len;
    int parse_pos;
} http_response_t;

int net_init(void);
void net_exit(void);
int http_get(const char *url, http_response_t *resp);
void http_response_free(http_response_t *resp);

#endif
