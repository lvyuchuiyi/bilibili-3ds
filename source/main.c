/*
 * main.c - minimal UI test: render main menu only, no net/player init
 */

#include <string.h>
#include <stdio.h>
#include <3ds.h>
#include <citro2d.h>
#include "ui.h"

static app_state_t state;

int main(void) {
    if (ui_init() != 0) return 1;

    memset(&state, 0, sizeof(state));
    state.current_screen = SCREEN_MAIN_MENU;
    state.prev_screen = SCREEN_MAIN_MENU;

    touchPosition last_touch;
    memset(&last_touch, 0, sizeof(last_touch));

    while (aptMainLoop()) {
        hidScanInput();
        u32 keys_down = hidKeysDown();
        touchPosition touch;
        hidTouchRead(&touch);

        ui_handle_keys(&state, keys_down);

        if (touch.px != last_touch.px || touch.py != last_touch.py) {
            ui_handle_touch(&state, &touch);
        }
        memcpy(&last_touch, &touch, sizeof(touch));

        ui_render(&state);
    }

    ui_exit();
    return 0;
}
