#ifndef _PLAYER_H_
#define _PLAYER_H_

#include <3ds.h>

#define PLAYER_BUF_SIZE (2 * 1024 * 1024)  /* 2MB download buffer */
#define PLAYER_NAL_SIZE (256 * 1024)        /* 256KB per NAL unit */

typedef enum {
    PLAYER_IDLE,
    PLAYER_LOADING,
    PLAYER_PLAYING,
    PLAYER_PAUSED,
    PLAYER_ERROR,
    PLAYER_DONE
} player_state_t;

typedef struct {
    player_state_t state;
    int width;
    int height;
    int total_frames;
    int current_frame;
    float progress;
    char url[512];
} player_info_t;

int player_init(void);
void player_exit(void);
int player_load(const char *url);
void player_play(void);
void player_pause(void);
void player_stop(void);
player_state_t player_update(void);
void player_get_info(player_info_t *info);

#endif
