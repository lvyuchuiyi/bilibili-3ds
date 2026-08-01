#include "wbi.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "bilibili.h"
#include "network.h"

int bili_debug_wbi_ok = 0;
int bili_debug_last_ret = 0;

static char *find_json_body(char *buf) {
    char *p = strstr(buf, "\r\n\r\n");
    if (!p) return NULL;
    return p + 4;
}

static int http_get_json(const char *url, json_value_t **json) {
    http_response_t resp = {0};
    int ret = http_get(url, &resp);
    if (ret != 0 || !resp.buf) return -1;

    char *body = find_json_body(resp.buf);
    if (!body) { http_response_free(&resp); return -2; }

    *json = json_parse(body);
    http_response_free(&resp);

    return *json ? 0 : -3;
}

static int parse_video_list(json_value_t *arr, bili_video_list_t *list) {
    if (!arr || arr->type != JSON_ARRAY) return -1;

    list->count = 0;
    for (int i = 0; i < arr->array.count && list->count < MAX_RESULTS; i++) {
        json_value_t *item = arr->array.values[i];
        if (!item) continue;

        bili_video_t *v = &list->videos[list->count];
        memset(v, 0, sizeof(bili_video_t));

        json_value_t *aid_v = json_get(item, "aid");
        if (aid_v) v->aid = (long long)json_number(aid_v);

        json_value_t *title_v = json_get(item, "title");
        if (title_v && json_string(title_v)) {
            strncpy(v->title, json_string(title_v), MAX_TITLE_LEN - 1);
        }

        json_value_t *author_v = json_get(item, "author");
        if (author_v && json_string(author_v)) {
            strncpy(v->author, json_string(author_v), MAX_AUTHOR_LEN - 1);
        } else {
            json_value_t *owner = json_get(item, "owner");
            if (owner) {
                json_value_t *name = json_get(owner, "name");
                if (name && json_string(name))
                    strncpy(v->author, json_string(name), MAX_AUTHOR_LEN - 1);
            }
        }

        json_value_t *pic_v = json_get(item, "pic");
        if (pic_v && json_string(pic_v)) {
            strncpy(v->pic_url, json_string(pic_v), MAX_URL_LEN - 1);
        }

        json_value_t *dur_v = json_get(item, "duration");
        if (dur_v) v->duration = (int)json_number(dur_v);

        json_value_t *stat = json_get(item, "stat");
        if (stat) {
            json_value_t *view_v = json_get(stat, "view");
            if (view_v) v->play_count = (int)json_number(view_v);
        }

        json_value_t *play_v = json_get(item, "play");
        if (play_v) v->play_count = (int)json_number(play_v);

        list->count++;
    }
    return list->count;
}

static int wbi_done = 0;

int bili_popular(bili_video_list_t *list) {
    json_value_t *root = NULL;
    bili_debug_last_ret = 0;
    if (!wbi_done && wbi_init() == 0) {
        wbi_done = 1;
        bili_debug_wbi_ok = 1;
    }
    char wbi_url[512];
    const char *api = "https://api.bilibili.com/x/web-interface/popular";
    const char *final_url = api;
    if (wbi_done && wbi_sign(api, "ps=20&pn=1", wbi_url, 512) == 0)
        final_url = wbi_url;
    int ret = http_get_json(final_url, &root);
    bili_debug_last_ret = ret;

    if (ret != 0) return ret;

    json_value_t *data = json_get(root, "data");
    json_value_t *list_arr = data ? json_get(data, "list") : NULL;

    ret = list_arr ? parse_video_list(list_arr, list) : -4;
    bili_debug_last_ret = ret;
    json_free(root);
    return ret;
}

int bili_search(const char *keyword, bili_video_list_t *list) {
    char url[1024];
    char *encoded = NULL;

    int kw_len = strlen(keyword);
    encoded = malloc(kw_len * 3 + 1);
    if (!encoded) return -1;

    int pos = 0;
    for (int i = 0; keyword[i]; i++) {
        unsigned char c = keyword[i];
        if (c == ' ') {
            encoded[pos++] = '+';
        } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                   (c >= '0' && c <= '9') || c == '-' || c == '_') {
            encoded[pos++] = c;
        } else {
            pos += snprintf(encoded + pos, 4, "%%%02X", c);
        }
    }
    encoded[pos] = '\0';

    snprintf(url, sizeof(url),
        "https://api.bilibili.com/x/web-interface/search/type"
        "?search_type=video&keyword=%s", encoded);
    free(encoded);

    json_value_t *root = NULL;
    int ret = http_get_json(url, &root);
    bili_debug_last_ret = ret;
    if (ret != 0) return ret;

    json_value_t *data = json_get(root, "data");
    json_value_t *result_arr = data ? json_get(data, "result") : NULL;

    ret = result_arr ? parse_video_list(result_arr, list) : -4;
    bili_debug_last_ret = ret;
    json_free(root);
    return ret;
}

int bili_video_info(long long aid, bili_video_t *video) {
    char url[256];
    snprintf(url, sizeof(url),
        "https://api.bilibili.com/x/web-interface/view?aid=%lld", aid);

    json_value_t *root = NULL;
    int ret = http_get_json(url, &root);
    if (ret != 0) return ret;

    json_value_t *data = json_get(root, "data");
    if (!data) { json_free(root); return -4; }

    memset(video, 0, sizeof(bili_video_t));
    video->aid = aid;

    json_value_t *title_v = json_get(data, "title");
    if (title_v && json_string(title_v))
        strncpy(video->title, json_string(title_v), MAX_TITLE_LEN - 1);

    json_value_t *owner = json_get(data, "owner");
    if (owner) {
        json_value_t *name = json_get(owner, "name");
        if (name && json_string(name))
            strncpy(video->author, json_string(name), MAX_AUTHOR_LEN - 1);
    }

    json_value_t *pic_v = json_get(data, "pic");
    if (pic_v && json_string(pic_v))
        strncpy(video->pic_url, json_string(pic_v), MAX_URL_LEN - 1);

    json_value_t *dur_v = json_get(data, "duration");
    if (dur_v) video->duration = (int)json_number(dur_v);

    json_value_t *stat = json_get(data, "stat");
    if (stat) {
        json_value_t *view_v = json_get(stat, "view");
        if (view_v) video->play_count = (int)json_number(view_v);
    }

    json_free(root);
    return 0;
}

char *bili_get_playurl(long long aid, long long cid) {
    char url[512];
    snprintf(url, sizeof(url),
        "https://api.bilibili.com/x/player/playurl"
        "?avid=%lld&cid=%lld&qn=16&fnval=0&fnver=0&otype=json", aid, cid);

    json_value_t *root = NULL;
    int ret = http_get_json(url, &root);
    if (ret != 0) return NULL;

    json_value_t *data = json_get(root, "data");
    json_value_t *durl = data ? json_get(data, "durl") : NULL;

    char *result = NULL;
    if (durl && durl->type == JSON_ARRAY && durl->array.count > 0) {
        json_value_t *first = durl->array.values[0];
        json_value_t *url_v = json_get(first, "url");
        if (url_v && json_string(url_v)) {
            result = strdup(json_string(url_v));
        }
    }

    json_free(root);
    return result;
}

void bili_free_playurl(char *url) {
    free(url);
}
