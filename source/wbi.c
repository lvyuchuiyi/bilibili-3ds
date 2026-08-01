#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <3ds.h>
#include <mbedtls/md5.h>
#include "network.h"
#include "wbi.h"

static const unsigned char MIXIN_KEY_ENC_TAB[] = {
    46,47,18,2,53,8,23,32,15,50,10,31,58,3,45,35,27,43,5,49,
    33,9,42,19,29,28,14,39,12,38,41,13,37,48,7,16,24,55,40,
    61,26,17,0,1,60,51,30,4,22,25,54,21,56,59,6,63,57,62,11,
    36,20,34,44,52
};
#define TAB_LEN 64

static char mixin_key[33] = {0};
static int wbi_initialized = 0;

static void hex_md5(const char *in, char *out) {
    unsigned char hash[16];
    mbedtls_md5_ret((const unsigned char*)in, strlen(in), hash);
    for (int i = 0; i < 16; i++) sprintf(out + i*2, "%02x", hash[i]);
    out[32] = 0;
}

static char *extract_key(const char *haystack, const char *prefix, char *out, int max) {
    char *p = strstr(haystack, prefix);
    if (!p) return NULL;
    p += strlen(prefix);
    /* Find filename after last / and before .png */
    char *last_slash = p;
    while (*p && *p != '"' && *p != '\\') { if (*p == '/') last_slash = p + 1; p++; }
    if (*p != '"') return NULL;
    int n = 0;
    while (last_slash < p && *last_slash != '.' && n < max - 1) out[n++] = *last_slash++;
    out[n] = 0;
    return out;
}

int wbi_init(void) {
    http_response_t resp = {0};
    int ret = http_get("https://api.bilibili.com/x/web-interface/nav", &resp);
    if (ret != 0 || !resp.buf) return -1;

    char img_key[64] = {0}, sub_key[64] = {0};
    if (!extract_key(resp.buf, "\"img_url\":\"", img_key, 64) ||
        !extract_key(resp.buf, "\"sub_url\":\"", sub_key, 64)) {
        http_response_free(&resp); return -2;
    }
    http_response_free(&resp);

    /* Build raw key = img_key + sub_key */
    char raw[128]; snprintf(raw, sizeof(raw), "%s%s", img_key, sub_key);
    int rlen = strlen(raw);
    if (rlen < TAB_LEN) return -3;

    /* Shuffle using MIXIN_KEY_ENC_TAB */
    char mixed[65] = {0};
    for (int i = 0; i < TAB_LEN; i++) {
        int idx = MIXIN_KEY_ENC_TAB[i];
        mixed[i] = (idx < rlen) ? raw[idx] : '0';
    }
    strncpy(mixin_key, mixed, 32);
    mixin_key[32] = 0;
    wbi_initialized = 1;
    return 0;
}

/* Parse query string into key/value pairs, filter special chars,
 * sort alphabetically, then sign query + mixin_key (wiliwili pattern). */
int wbi_sign(const char *base, const char *params, char *out, int out_size) {
    if (!wbi_initialized) return -1;
    if (!base || !out || out_size <= 0) return -1;

    long ts = time(NULL);
    char query[512];
    if (params && params[0])
        snprintf(query, sizeof(query), "%s&wts=%ld", params, ts);
    else
        snprintf(query, sizeof(query), "wts=%ld", ts);

    /* Split into pairs */
    char *pairs[32];
    int pair_count = 0;
    char tmp[512];
    strncpy(tmp, query, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = 0;

    char *save = NULL;
    char *tok = strtok_r(tmp, "&", &save);
    while (tok && pair_count < 32) {
        pairs[pair_count++] = tok;
        tok = strtok_r(NULL, "&", &save);
    }

    /* Sort pairs lexicographically */
    for (int i = 0; i < pair_count - 1; i++) {
        for (int j = i + 1; j < pair_count; j++) {
            if (strcmp(pairs[j], pairs[i]) < 0) {
                char *t = pairs[i]; pairs[i] = pairs[j]; pairs[j] = t;
            }
        }
    }

    /* Build sorted query, filtering !'()* from values */
    char sorted[600] = {0};
    for (int i = 0; i < pair_count; i++) {
        char *eq = strchr(pairs[i], '=');
        char key[128] = {0}, val[256] = {0};
        if (eq) {
            int klen = eq - pairs[i];
            if (klen > 0 && klen < 128) strncpy(key, pairs[i], klen);
            strncpy(val, eq + 1, 255);
        } else {
            strncpy(key, pairs[i], 127);
        }
        /* Filter !'()* from value */
        int n = 0;
        for (int c = 0; val[c] && n < 255; c++) {
            if (val[c] != '!' && val[c] != '\'' && val[c] != '(' &&
                val[c] != ')' && val[c] != '*') {
                val[n++] = val[c];
            }
        }
        val[n] = 0;

        if (i > 0) strncat(sorted, "&", sizeof(sorted) - strlen(sorted) - 1);
        strncat(sorted, key, sizeof(sorted) - strlen(sorted) - 1);
        strncat(sorted, "=", sizeof(sorted) - strlen(sorted) - 1);
        strncat(sorted, val, sizeof(sorted) - strlen(sorted) - 1);
    }

    /* w_rid = md5(sorted_query + mixin_key)  (no '&') */
    char to_sign[700];
    snprintf(to_sign, sizeof(to_sign), "%s%s", sorted, mixin_key);
    char w_rid[33]; hex_md5(to_sign, w_rid);

    snprintf(out, out_size, "%s?%s&w_rid=%s", base, sorted, w_rid);
    return 0;
}
