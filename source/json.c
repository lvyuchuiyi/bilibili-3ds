#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "json.h"

static void skip_whitespace(const char **p) {
    while (**p && isspace((unsigned char)**p)) (*p)++;
}

static json_value_t *alloc_value(void) {
    json_value_t *v = calloc(1, sizeof(json_value_t));
    if (!v) return NULL;
    v->type = JSON_NULL;
    return v;
}

static json_value_t *parse_value(const char **p);

static char *parse_string_raw(const char **p) {
    skip_whitespace(p);
    if (**p != '"') return NULL;
    (*p)++;

    int cap = 64, len = 0;
    char *s = malloc(cap);
    if (!s) return NULL;

    while (**p && **p != '"') {
        if (len >= cap - 1) {
            cap *= 2;
            char *ns = realloc(s, cap);
            if (!ns) { free(s); return NULL; }
            s = ns;
        }
        if (**p == '\\') {
            (*p)++;
            switch (**p) {
                case '"':  s[len++] = '"'; break;
                case '\\': s[len++] = '\\'; break;
                case '/':  s[len++] = '/'; break;
                case 'b':  s[len++] = '\b'; break;
                case 'f':  s[len++] = '\f'; break;
                case 'n':  s[len++] = '\n'; break;
                case 'r':  s[len++] = '\r'; break;
                case 't':  s[len++] = '\t'; break;
                case 'u': {
                    (*p)++;
                    char hex[5] = {0};
                    strncpy(hex, *p, 4);
                    unsigned int cp = strtoul(hex, NULL, 16);
                    (*p) += 3;
                    if (cp < 0x80) s[len++] = cp;
                    else if (cp < 0x800) { s[len++] = 0xC0 | (cp>>6); s[len++] = 0x80 | (cp&0x3F); }
                    else { s[len++] = 0xE0 | (cp>>12); s[len++] = 0x80 | ((cp>>6)&0x3F); s[len++] = 0x80 | (cp&0x3F); }
                    break;
                }
                default: s[len++] = **p; break;
            }
        } else {
            s[len++] = **p;
        }
        (*p)++;
    }
    if (**p == '"') (*p)++;
    s[len] = '\0';
    return s;
}

static json_value_t *parse_string(const char **p) {
    json_value_t *v = alloc_value();
    if (!v) return NULL;
    v->type = JSON_STRING;
    v->str_val = parse_string_raw(p);
    if (!v->str_val) { free(v); return NULL; }
    return v;
}

static json_value_t *parse_number(const char **p) {
    json_value_t *v = alloc_value();
    if (!v) return NULL;
    v->type = JSON_NUMBER;
    char *end;
    v->num_val = strtod(*p, &end);
    *p = end;
    return v;
}

static json_value_t *parse_array(const char **p) {
    json_value_t *v = alloc_value();
    if (!v) return NULL;
    v->type = JSON_ARRAY;
    v->array.count = 0;
    v->array.capacity = 8;
    v->array.values = malloc(sizeof(json_value_t*) * v->array.capacity);
    if (!v->array.values) { free(v); return NULL; }

    (*p)++;
    skip_whitespace(p);
    if (**p == ']') { (*p)++; return v; }

    while (1) {
        json_value_t *elem = parse_value(p);
        if (elem) {
            if (v->array.count >= v->array.capacity) {
                v->array.capacity *= 2;
                v->array.values = realloc(v->array.values, sizeof(json_value_t*) * v->array.capacity);
            }
            v->array.values[v->array.count++] = elem;
        }
        skip_whitespace(p);
        if (**p == ']') { (*p)++; break; }
        if (**p == ',') { (*p)++; skip_whitespace(p); }
    }
    return v;
}

static json_value_t *parse_object(const char **p) {
    json_value_t *v = alloc_value();
    if (!v) return NULL;
    v->type = JSON_OBJECT;
    v->object.count = 0;
    v->object.capacity = 8;
    v->object.keys = malloc(sizeof(char*) * v->object.capacity);
    v->object.values = malloc(sizeof(json_value_t*) * v->object.capacity);
    if (!v->object.keys || !v->object.values) { free(v); return NULL; }

    (*p)++;
    skip_whitespace(p);
    if (**p == '}') { (*p)++; return v; }

    while (1) {
        skip_whitespace(p);
        char *key = parse_string_raw(p);
        if (!key) break;
        skip_whitespace(p);
        if (**p == ':') (*p)++;
        skip_whitespace(p);
        json_value_t *val = parse_value(p);
        if (!val) { free(key); break; }

        if (v->object.count >= v->object.capacity) {
            v->object.capacity *= 2;
            v->object.keys = realloc(v->object.keys, sizeof(char*) * v->object.capacity);
            v->object.values = realloc(v->object.values, sizeof(json_value_t*) * v->object.capacity);
        }
        v->object.keys[v->object.count] = key;
        v->object.values[v->object.count] = val;
        v->object.count++;

        skip_whitespace(p);
        if (**p == '}') { (*p)++; break; }
        if (**p == ',') { (*p)++; }
    }
    return v;
}

static json_value_t *parse_value(const char **p) {
    skip_whitespace(p);
    switch (**p) {
        case '"': return parse_string(p);
        case '{': return parse_object(p);
        case '[': return parse_array(p);
        case 't': (*p) += 4; { json_value_t *v = alloc_value(); v->type = JSON_BOOL; v->bool_val = 1; return v; }
        case 'f': (*p) += 5; { json_value_t *v = alloc_value(); v->type = JSON_BOOL; v->bool_val = 0; return v; }
        case 'n': (*p) += 4; return alloc_value();
        default:
            if (**p == '-' || (**p >= '0' && **p <= '9')) return parse_number(p);
            return NULL;
    }
}

json_value_t *json_parse(const char *text) {
    if (!text) return NULL;
    return parse_value(&text);
}

json_value_t *json_get(json_value_t *root, const char *key) {
    if (!root || root->type != JSON_OBJECT) return NULL;
    for (int i = 0; i < root->object.count; i++) {
        if (strcmp(root->object.keys[i], key) == 0)
            return root->object.values[i];
    }
    return NULL;
}

json_value_t *json_index(json_value_t *arr, int idx) {
    if (!arr || arr->type != JSON_ARRAY) return NULL;
    if (idx < 0 || idx >= arr->array.count) return NULL;
    return arr->array.values[idx];
}

const char *json_string(json_value_t *val) {
    if (!val || val->type != JSON_STRING) return NULL;
    return val->str_val;
}

double json_number(json_value_t *val) {
    if (!val || val->type != JSON_NUMBER) return 0;
    return val->num_val;
}

int json_bool(json_value_t *val) {
    if (!val || val->type != JSON_BOOL) return 0;
    return val->bool_val;
}

int json_length(json_value_t *val) {
    if (!val) return 0;
    if (val->type == JSON_ARRAY) return val->array.count;
    if (val->type == JSON_OBJECT) return val->object.count;
    return 0;
}

static void json_free_internal(json_value_t *val) {
    if (!val) return;
    if (val->type == JSON_STRING) {
        free(val->str_val);
    } else if (val->type == JSON_ARRAY) {
        for (int i = 0; i < val->array.count; i++)
            json_free_internal(val->array.values[i]);
        free(val->array.values);
    } else if (val->type == JSON_OBJECT) {
        for (int i = 0; i < val->object.count; i++) {
            free(val->object.keys[i]);
            json_free_internal(val->object.values[i]);
        }
        free(val->object.keys);
        free(val->object.values);
    }
    free(val);
}

void json_free(json_value_t *val) {
    json_free_internal(val);
}
