#ifndef WBI_H
#define WBI_H
int wbi_init(void);
int wbi_sign(const char *base, const char *params, char *out, int out_size);
#endif