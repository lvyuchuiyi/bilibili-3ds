/*
 * main.c - BiliBili 3DS: async network thread, UI always renders
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <3ds.h>
#include <citro2d.h>
#include "network.h"
#include "bilibili.h"
#include "ui.h"
#include "player.h"

#define TOUCH_HOLD_FRAMES 5

int main_debug_load_count = -1;
int app_loading = 0;
int app_load_state = 0;
int app_load_timed_out = 0;
int app_load_stage = 0;
char app_debug_playurl[128] = "";

/* mbedtls TLS needs more stack than the 32KB libctru default */
u32 __stacksize__ = 64U * 1024U;

static app_state_t state;
static volatile int load_requested = 0;
static volatile int load_state = 0;  /* 0=idle 1=loading 2=done */
static Thread load_thread = NULL;
static u64 load_start_tick = 0;

static void load_thread_func(void *arg) {
    (void)arg;
    if (load_requested == 1) {
        app_load_stage = 2;
        bili_popular(&state.popular_list);
        main_debug_load_count = state.popular_list.count;
    } else if (load_requested == 2) {
        if (strlen(state.search_text) > 0) {
            app_load_stage = 2;
            bili_search(state.search_text, &state.search_list);
        }
    } else if (load_requested == 3) {
        /* Fetch play URL and start playback */
        app_load_stage = 2;
        bili_video_t info;
        int r = bili_video_info(state.current_video.aid, &info);
        if (r == 0) {
            char *pu = bili_get_playurl(info.aid, info.cid);
            if (pu) {
                strncpy(app_debug_playurl, pu, sizeof(app_debug_playurl) - 1);
                app_debug_playurl[sizeof(app_debug_playurl) - 1] = 0;
                if (player_init() == 0 && player_load(pu) == 0) {
                    state.current_screen = SCREEN_PLAYING;
                }
                free(pu);
            } else {
                strcpy(app_debug_playurl, "PLAYURL FAIL");
            }
        } else {
            strcpy(app_debug_playurl, "INFO FAIL");
        }
    }
    app_load_stage = 3;
    load_state = 2;
}

static void request_load(int kind) {
    if (load_state != 0) return;
    load_requested = kind;
    load_state = 1;
    app_loading = 1;
    app_load_state = 1;
    app_load_timed_out = 0;
    app_load_stage = 1;
    load_start_tick = osGetTime();
    load_thread = threadCreate(load_thread_func, NULL, 64 * 1024, 0x3F, -1, false);
    if (!load_thread) {
        load_state = 0;
        app_loading = 0;
        app_load_state = 0;
        app_load_stage = 0;
    }
}

static void finish_load(void) {
    if (load_state == 2 && load_thread) {
        threadJoin(load_thread, U64_MAX);
        threadFree(load_thread);
        load_thread = NULL;
        load_state = 0;
        load_requested = 0;
        app_loading = 0;
        app_load_state = 0;
    }
}

int main(void) {
    if (ui_init() != 0) return 1;

    int net_ok = (net_init() == 0);

    memset(&state, 0, sizeof(state));
    state.current_screen = SCREEN_MAIN_MENU;
    state.prev_screen = SCREEN_MAIN_MENU;

    touchPosition last_touch;
    memset(&last_touch, 0, sizeof(last_touch));
    int touch_held = 0;
    int touch_triggered = 0;

    static int prev_screen = SCREEN_SPLASH;

    while (aptMainLoop()) {
        finish_load();

        /* Watchdog: if loading takes >15s, stop showing Loading */
        if (app_load_state == 1 && osGetTime() - load_start_tick > 15000) {
            app_load_timed_out = 1;
            app_loading = 0;
        }

        hidScanInput();
        u32 keys_down = hidKeysDown();
        touchPosition touch;
        hidTouchRead(&touch);

        /* B always works during loading */
        if ((keys_down & KEY_B) && app_loading) {
            state.current_screen = SCREEN_MAIN_MENU;
            state.prev_screen = SCREEN_MAIN_MENU;
        }
        int handled = ui_handle_keys(&state, keys_down);
        if (handled == 2 && state.current_screen == SCREEN_VIDEO_DETAIL && net_ok) {
            request_load(3);
        }

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
            ui_handle_touch(&state, &touch);
        }

        memcpy(&last_touch, &touch, sizeof(touch));

        /* Detect screen transitions and start async load */
        if (state.current_screen == SCREEN_POPULAR &&
            prev_screen != SCREEN_POPULAR) {
            if (net_ok) request_load(1);
        }
        if (state.current_screen == SCREEN_SEARCH_RESULTS &&
            prev_screen == SCREEN_SEARCH_INPUT) {
            if (net_ok) request_load(2);
        }
        prev_screen = state.current_screen;

        if (state.current_screen == SCREEN_PLAYING) {
            player_update();
        }

        /* Render every frame; Loading text shown while app_loading */
        ui_render(&state);
    }

    if (load_state == 1) {
        threadJoin(load_thread, U64_MAX);
        threadFree(load_thread);
    }
    player_exit();
    net_exit();
    ui_exit();
    return 0;
}




