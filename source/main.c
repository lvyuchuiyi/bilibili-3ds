/*
 * main.c - BiliBili 3DS: UI + synchronous network loading
 */

#include <string.h>
#include <stdio.h>
#include <3ds.h>
#include <citro2d.h>
#include "network.h"
#include "bilibili.h"
#include "ui.h"

#define TOUCH_HOLD_FRAMES 5

int main_debug_load_count = -1;
int app_loading = 0;
int app_load_state = 0;
int app_load_timed_out = 0;
int app_load_stage = 0;  /* 0=idle 1=flag 2=entering bili_popular 3=returned */

/* mbedtls TLS needs more stack than the 32KB libctru default */
u32 __stacksize__ = 64U * 1024U;

static app_state_t state;

static void load_popular(void) {
    app_loading = 1;
    app_load_state = 1;
    app_load_timed_out = 0;
    app_load_stage = 1;
    ui_render(&state);  /* show Loading before blocking request */
    app_load_stage = 2;

    extern int net_debug_stage;
    net_debug_stage = -1;  /* mark before http_get */
    int ret = bili_popular(&state.popular_list);
    main_debug_load_count = state.popular_list.count;
    (void)ret;
    app_load_stage = 3;

    app_loading = 0;
    app_load_state = 0;
}

static void load_search(void) {
    if (strlen(state.search_text) == 0) return;
    app_loading = 1;
    app_load_state = 1;
    ui_render(&state);

    int ret = bili_search(state.search_text, &state.search_list);
    (void)ret;

    app_loading = 0;
    app_load_state = 0;
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

    while (aptMainLoop()) {
        hidScanInput();
        u32 keys_down = hidKeysDown();
        touchPosition touch;
        hidTouchRead(&touch);

        int handled = ui_handle_keys(&state, keys_down);
        (void)handled;

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

        static int prev_screen = SCREEN_SPLASH;
        if (state.current_screen == SCREEN_POPULAR &&
            prev_screen != SCREEN_POPULAR) {
            if (net_ok) load_popular();
        }
        if (state.current_screen == SCREEN_SEARCH_RESULTS &&
            prev_screen == SCREEN_SEARCH_INPUT) {
            if (net_ok) load_search();
        }
        prev_screen = state.current_screen;

        ui_render(&state);
    }

    net_exit();
    ui_exit();
    return 0;
}

