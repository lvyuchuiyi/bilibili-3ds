#ifndef _BILIBILI_H_
#define _BILIBILI_H_

#include "json.h"

#define MAX_RESULTS 50
#define MAX_TITLE_LEN 256
#define MAX_AUTHOR_LEN 64
#define MAX_URL_LEN 512

typedef struct {
    long long aid;
    char title[MAX_TITLE_LEN];
    char author[MAX_AUTHOR_LEN];
    char pic_url[MAX_URL_LEN];
    int duration;
    int play_count;
    int video_review;
} bili_video_t;

typedef struct {
    int count;
    bili_video_t videos[MAX_RESULTS];
} bili_video_list_t;

int bili_popular(bili_video_list_t *list);
int bili_search(const char *keyword, bili_video_list_t *list);
int bili_video_info(long long aid, bili_video_t *video);
char *bili_get_playurl(long long aid, long long cid);
void bili_free_playurl(char *url);

#endif
