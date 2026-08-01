#ifndef _JSON_H_
#define _JSON_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    JSON_NULL,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} json_type_t;

typedef struct json_value {
    json_type_t type;
    union {
        int bool_val;
        double num_val;
        char *str_val;
        struct {
            struct json_value **values;
            int count;
            int capacity;
        } array;
        struct {
            char **keys;
            struct json_value **values;
            int count;
            int capacity;
        } object;
    };
} json_value_t;

json_value_t *json_parse(const char *text);
json_value_t *json_get(json_value_t *root, const char *key);
json_value_t *json_index(json_value_t *arr, int idx);
const char *json_string(json_value_t *val);
double json_number(json_value_t *val);
int json_bool(json_value_t *val);
int json_length(json_value_t *val);
void json_free(json_value_t *val);

#ifdef __cplusplus
}
#endif

#endif

