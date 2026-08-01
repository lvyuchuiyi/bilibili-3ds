/*
 * main.c - BiliBili 3DS: UI + async network browsing, no player yet
 */

#include <string.h>
#include <stdio.h>
#include <3ds.h>
#include <citro2d.h>
#include "network.h"
#include "bilibili.h"
#include "ui.h"

#define TOUCH_HOLD_FRAMES 5

int main_debug_load_count = -1;  /* count right after bili_popular returns */
int app_loading = 0;                 /* 1 while network request is running */
int app_load_state = 0;                      /* 0=idle 1=loading 2=done */
static u64 load_start_tick = 0;
int app_load_timed_out = 0;

/* mbedtls TLS needs more stack than the 32KB libctru default */
u32 __stacksize__ = 64U * 1024U;

static app_state_t state;
static volatile int load_requested = 0; /* 0=none, 1=popular, 2=search */
static volatile int load_state = 0;     /* 0=idle, 1=loading, 2=done */
static Thread load_thread = NULL;

static void load_thread_func(void *arg) {
    (void)arg;
    if (load_requested == 1) {
        bili_popular(&state.popular_list);
        main_debug_load_count = state.popular_list.count;
    } else if (load_requested == 2) {
        if (strlen(state.search_text) > 0) {
            bili_search(state.search_text, &state.search_list);
        }
    }
    load_state = 2;
}

static void request_load(int kind) {
    if (load_state != 0) return;  /* already loading */
    load_requested = kind;
    load_state = 1;
    app_loading = 1;
    app_load_state = 1;
    app_load_timed_out = 0;
    load_start_tick = osGetTime();
    load_thread = threadCreate(load_thread_func, NULL, 64 * 1024, 0x30, -1, false);
    if (!load_thread) load_state = 0;
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

    /* Network init is non-fatal */
    int net_ok = (net_init() == 0);

    memset(&state, 0, sizeof(state));
    state.current_screen = SCREEN_MAIN_MENU;
    state.prev_screen = SCREEN_MAIN_MENU;

    touchPosition last_touch;
    memset(&last_touch, 0, sizeof(last_touch));
    int touch_held = 0;
    int touch_triggered = 0;

    while (aptMainLoop()) {
        finish_load();

        /* Watchdog: if loading takes >15s, stop showing Loading... */
        if (app_load_state == 1 && osGetTime() - load_start_tick > 15000) {
            app_load_timed_out = 1;
            app_loading = 0;
        }

        hidScanInput();
        u32 keys_down = hidKeysDown();
        touchPosition touch;
        hidTouchRead(&touch);

        if ((keys_down & KEY_B) && app_loading) {
            state.current_screen = SCREEN_MAIN_MENU;
            state.prev_screen = SCREEN_MAIN_MENU;
        }
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
            if (net_ok) request_load(1);
        }
        if (state.current_screen == SCREEN_SEARCH_RESULTS &&
            prev_screen == SCREEN_SEARCH_INPUT) {
            if (net_ok) request_load(2);
        }
        prev_screen = state.current_screen;

        ui_render(&state);
    }

    if (load_state == 1) {
        threadJoin(load_thread, U64_MAX);
        threadFree(load_thread);
    }
    net_exit();
    ui_exit();
    return 0;
}





