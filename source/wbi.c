#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <3ds.h>
#include "network.h"
#include "wbi.h"

/* Self-contained MD5 (RFC 1321) - avoids mbedtls dependency */

typedef struct {
    u32 state[4];
    u32 count[2];
    u8 buffer[64];
} md5_ctx;

#define F(x,y,z) (((x)&(y)) | (~(x)&(z)))
#define G(x,y,z) (((x)&(z)) | ((y)&(~(z))))
#define H(x,y,z) ((x)^(y)^(z))
#define I(x,y,z) ((y)^((x)|(~(z))))

#define ROTL(x,n) (((x)<<(n)) | ((x)>>(32-(n))))

#define STEP(f,a,b,c,d,x,t,s) \
    (a) += f((b),(c),(d)) + (x) + (t); \
    (a) = ROTL((a),(s)); \
    (a) += (b);

static const u32 K[64] = {
    0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,
    0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
    0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,
    0x6b901122,0xfd987193,0xa679438e,0x49b40821,
    0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,
    0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
    0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,
    0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
    0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,
    0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
    0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,
    0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
    0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,
    0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
    0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,
    0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391
};

static const u8 R[64] = {
    7,12,17,22, 7,12,17,22, 7,12,17,22, 7,12,17,22,
    5, 9,14,20, 5, 9,14,20, 5, 9,14,20, 5, 9,14,20,
    4,11,16,23, 4,11,16,23, 4,11,16,23, 4,11,16,23,
    6,10,15,21, 6,10,15,21, 6,10,15,21, 6,10,15,21
};

static void md5_init(md5_ctx *ctx) {
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xefcdab89;
    ctx->state[2] = 0x98badcfe;
    ctx->state[3] = 0x10325476;
    ctx->count[0] = 0;
    ctx->count[1] = 0;
}

static void md5_transform(u32 state[4], const u8 block[64]) {
    u32 a=state[0], b=state[1], c=state[2], d=state[3];
    u32 x[16];
    for (int i=0;i<16;i++) {
        x[i] = (u32)block[i*4] | ((u32)block[i*4+1]<<8) |
               ((u32)block[i*4+2]<<16) | ((u32)block[i*4+3]<<24);
    }
    int idx = 0;
    for (int i=0;i<16;i++) { STEP(F,a,b,c,d,x[idx],K[idx],R[idx]); idx++; STEP(F,d,a,b,c,x[idx],K[idx],R[idx]); idx++; STEP(F,c,d,a,b,x[idx],K[idx],R[idx]); idx++; STEP(F,b,c,d,a,x[idx],K[idx],R[idx]); idx++; }
    for (int i=0;i<16;i++) { STEP(G,a,b,c,d,x[(5*idx+1)&15],K[idx],R[idx]); idx++; STEP(G,d,a,b,c,x[(5*idx+1)&15],K[idx],R[idx]); idx++; STEP(G,c,d,a,b,x[(5*idx+1)&15],K[idx],R[idx]); idx++; STEP(G,b,c,d,a,x[(5*idx+1)&15],K[idx],R[idx]); idx++; }
    for (int i=0;i<16;i++) { STEP(H,a,b,c,d,x[(3*idx+5)&15],K[idx],R[idx]); idx++; STEP(H,d,a,b,c,x[(3*idx+5)&15],K[idx],R[idx]); idx++; STEP(H,c,d,a,b,x[(3*idx+5)&15],K[idx],R[idx]); idx++; STEP(H,b,c,d,a,x[(3*idx+5)&15],K[idx],R[idx]); idx++; }
    for (int i=0;i<16;i++) { STEP(I,a,b,c,d,x[(7*idx)&15],K[idx],R[idx]); idx++; STEP(I,d,a,b,c,x[(7*idx)&15],K[idx],R[idx]); idx++; STEP(I,c,d,a,b,x[(7*idx)&15],K[idx],R[idx]); idx++; STEP(I,b,c,d,a,x[(7*idx)&15],K[idx],R[idx]); idx++; }
    state[0]+=a; state[1]+=b; state[2]+=c; state[3]+=d;
}

static void md5_update(md5_ctx *ctx, const u8 *data, size_t len) {
    u32 i, idx = (u32)((ctx->count[0] >> 3) & 63);
    if ((ctx->count[0] += (u32)len << 3) < (u32)len << 3) ctx->count[1]++;
    ctx->count[1] += (u32)len >> 29;
    for (i=0;i<len;i++) {
        ctx->buffer[idx++] = data[i];
        if (idx == 64) { md5_transform(ctx->state, ctx->buffer); idx = 0; }
    }
}

static void md5_final(md5_ctx *ctx, u8 digest[16]) {
    u8 bits[8];
    u32 idx, padlen;
    for (int i=0;i<8;i++) bits[i] = (u8)((ctx->count[i/4] >> ((i%4)*8)) & 0xff);
    idx = (u32)((ctx->count[0] >> 3) & 63);
    padlen = (idx < 56) ? (56 - idx) : (120 - idx);
    static const u8 padding[64] = { 0x80 };
    md5_update(ctx, padding, padlen);
    md5_update(ctx, bits, 8);
    for (int i=0;i<4;i++) {
        digest[i*4]   = (u8)(ctx->state[i] & 0xff);
        digest[i*4+1] = (u8)((ctx->state[i] >> 8) & 0xff);
        digest[i*4+2] = (u8)((ctx->state[i] >> 16) & 0xff);
        digest[i*4+3] = (u8)((ctx->state[i] >> 24) & 0xff);
    }
}

static void md5_hex(const char *in, char *out) {
    md5_ctx ctx;
    u8 digest[16];
    md5_init(&ctx);
    md5_update(&ctx, (const u8*)in, strlen(in));
    md5_final(&ctx, digest);
    for (int i=0;i<16;i++) sprintf(out + i*2, "%02x", digest[i]);
    out[32] = 0;
}

static const unsigned char MIXIN_KEY_ENC_TAB[] = {
    46,47,18,2,53,8,23,32,15,50,10,31,58,3,45,35,27,43,5,49,
    33,9,42,19,29,28,14,39,12,38,41,13,37,48,7,16,24,55,40,
    61,26,17,0,1,60,51,30,4,22,25,54,21,56,59,6,63,57,62,11,
    36,20,34,44,52
};
#define TAB_LEN 64

static char mixin_key[33] = {0};
static int wbi_initialized = 0;

static char *extract_key(const char *haystack, const char *prefix, char *out, int max) {
    char *p = strstr(haystack, prefix);
    if (!p) return NULL;
    p += strlen(prefix);
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

    char raw[128]; snprintf(raw, sizeof(raw), "%s%s", img_key, sub_key);
    int rlen = strlen(raw);
    if (rlen < TAB_LEN) return -3;

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

int wbi_sign(const char *base, const char *params, char *out, int out_size) {
    if (!wbi_initialized) return -1;
    if (!base || !out || out_size <= 0) return -1;

    long ts = time(NULL);
    char query[512];
    if (params && params[0])
        snprintf(query, sizeof(query), "%s&wts=%ld", params, ts);
    else
        snprintf(query, sizeof(query), "wts=%ld", ts);

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

    for (int i = 0; i < pair_count - 1; i++) {
        for (int j = i + 1; j < pair_count; j++) {
            if (strcmp(pairs[j], pairs[i]) < 0) {
                char *t = pairs[i]; pairs[i] = pairs[j]; pairs[j] = t;
            }
        }
    }

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

    char to_sign[700];
    snprintf(to_sign, sizeof(to_sign), "%s%s", sorted, mixin_key);
    char w_rid[33]; md5_hex(to_sign, w_rid);

    snprintf(out, out_size, "%s?%s&w_rid=%s", base, sorted, w_rid);
    return 0;
}
