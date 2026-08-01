/*
 * main.c - BiliBili 3DS Application
 *
 * Screen-based state machine with citro3d rendering,
 * mbedTLS networking, and MVD hardware video decoding.
 */

#include <string.h>
#include <stdio.h>
#include <3ds.h>
#include <citro2d.h>
#include "network.h"
#include "bilibili.h"
#include "ui.h"
#include "player.h"

#define TOUCH_HOLD_FRAMES 5

static app_state_t state;
static int load_requested = 0; /* 0=none, 1=popular, 2=search, 3=play */

static void load_popular(void) {
    int ret = bili_popular(&state.popular_list);
    (void)ret;
}

static void load_search(void) {
    if (strlen(state.search_text) > 0) {
        int ret = bili_search(state.search_text, &state.search_list);
        (void)ret;
    }
}

static void start_playback(void) {
    long long aid = state.current_video.aid;
    if (aid <= 0) return;

    bili_video_t info;
    int ret = bili_video_info(aid, &info);
    if (ret != 0) return;

    char *play_url = bili_get_playurl(aid, 0);
    if (!play_url) return;

    state.current_screen = SCREEN_PLAYING;
    ret = player_load(play_url);
    bili_free_playurl(play_url);

    if (ret != 0) {
        state.current_screen = SCREEN_VIDEO_DETAIL;
    }
}

int main(void) {
    if (ui_init() != 0) return 1;

    /* Network init failure is non-fatal: UI still works */
    int net_ok = (net_init() == 0);

    /* Player init failure is also non-fatal */
    int player_ok = (player_init() == 0);

    memset(&state, 0, sizeof(state));
    state.current_screen = SCREEN_MAIN_MENU;
    state.prev_screen = SCREEN_MAIN_MENU;

    touchPosition last_touch;
    memset(&last_touch, 0, sizeof(last_touch));
    int touch_held = 0;
    int touch_triggered = 0;

    while (aptMainLoop()) {
        if (load_requested == 1) {
            load_requested = 0;
            if (net_ok) load_popular();
        } else if (load_requested == 2) {
            load_requested = 0;
            if (net_ok) load_search();
        } else if (load_requested == 3) {
            load_requested = 0;
            if (player_ok) start_playback();
        }

        hidScanInput();
        u32 keys_down = hidKeysDown();
        touchPosition touch;
        hidTouchRead(&touch);

        int handled = ui_handle_keys(&state, keys_down);
        if (handled == 2) load_requested = 3;

        if (touch.px != last_touch.px || touch.py != last_touch.py) {
            touch_held = 0;
        }

        if (touch.px > 0 || touch.py > 0) {
            touch_held++;
        } else {
            touch_held = 0;
            touch_triggered = 0;
        }

        if (touch_held == TOUCH_HOLD_FRAMES && !touch_triggered) {
            touch_triggered = 1;
            int ret = ui_handle_touch(&state, &touch);
            if (ret == 2) load_requested = 3;
        }

        memcpy(&last_touch, &touch, sizeof(touch));

        static int prev_screen = SCREEN_SPLASH;
        if (state.current_screen == SCREEN_POPULAR &&
            prev_screen != SCREEN_POPULAR) {
            load_requested = 1;
        }
        if (state.current_screen == SCREEN_SEARCH_RESULTS &&
            prev_screen == SCREEN_SEARCH_INPUT) {
            load_requested = 2;
        }
        prev_screen = state.current_screen;

        if (state.current_screen == SCREEN_PLAYING && player_ok) {
            player_update();
        }

        ui_render(&state);
    }

    player_exit();
    net_exit();
    ui_exit();
    return 0;
}
