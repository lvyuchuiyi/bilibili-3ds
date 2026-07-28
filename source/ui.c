/*
 * ui.c - citro3d-based UI for 3DS
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <3ds.h>
#include <citro2d.h>
#include "ui.h"
#include "player.h"
#include <stdarg.h>

/* Pre-allocated render resources */
static C3D_RenderTarget *top_target = NULL;
static C3D_RenderTarget *bot_target = NULL;
static C2D_Font font = NULL;
static C2D_TextBuf textbuf = NULL;

/* Text pool - parse BEFORE C3D_FrameBegin, draw DURING frame */
#define MAX_TEXTS 64
static C2D_Text text_pool[MAX_TEXTS];
static int text_n = 0;
static int text_draw_idx = 0;
static int text_prep_pass = 0;

/* Colors */
#define CLR_BG        C2D_Color32(0xF5, 0xF5, 0xF5, 0xFF)
#define CLR_PRIMARY   C2D_Color32(0x00, 0x96, 0xED, 0xFF)
#define CLR_PRIMARY_DK C2D_Color32(0x00, 0x78, 0xC8, 0xFF)
#define CLR_TEXT      C2D_Color32(0x22, 0x22, 0x22, 0xFF)
#define CLR_TEXT_LT   C2D_Color32(0x88, 0x88, 0x88, 0xFF)
#define CLR_WHITE     C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF)
#define CLR_CARD      C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF)
#define CLR_SEL       C2D_Color32(0xE3, 0xF2, 0xFD, 0xFF)
#define CLR_RED       C2D_Color32(0xFB, 0x72, 0x99, 0xFF)

/* Layout constants */
#define TOP_W  400
#define TOP_H  240
#define BOT_W  320
#define BOT_H  240
#define MARGIN  8
#define LIST_ITEM_H 26
#define BTN_H 28
#define KB_KEY_W 24
#define KB_KEY_H 20
#define KB_GAP 2

/*------------------- Helper functions -------------------*/

static void draw_rect(int x, int y, int w, int h, u32 color) {
    if (!text_prep_pass)
        C2D_DrawRectSolid(x, y, 0.5f, w, h, color);
}

static void draw_text(int x, int y, float scale, u32 color, const char *fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (text_prep_pass) {
        /* Phase 1: store and parse text (before C3D_FrameBegin) */
        if (text_n < MAX_TEXTS) {
            C2D_TextFontParse(&text_pool[text_n], font, textbuf, buf);
            C2D_TextOptimize(&text_pool[text_n]);
            text_n++;
        }
    } else {
        /* Phase 2: draw prepared text (during C3D_FrameBegin/End) */
        if (text_draw_idx < MAX_TEXTS) {
            C2D_DrawText(&text_pool[text_draw_idx], C2D_WithColor, x, y, 0.5f, scale, scale, color);
            text_draw_idx++;
        }
    }
}

/*------------------- Screen renderers -------------------*/

static void render_main_menu(void) {
    draw_rect(0, 0, TOP_W, 40, CLR_PRIMARY);
    draw_text(16, 10, 0.65f, CLR_WHITE, "BiliBili for 3DS");

    int btn_w = 200, btn_h = 48;
    int bx = (TOP_W - btn_w) / 2;
    int by = 70;

    draw_rect(bx, by, btn_w, btn_h, CLR_PRIMARY);
    draw_text(bx + 16, by + 14, 0.55f, CLR_WHITE, "Popular");

    draw_rect(bx, by + 70, btn_w, btn_h, CLR_PRIMARY);
    draw_text(bx + 16, by + 84, 0.55f, CLR_WHITE, "Search");
}

static void render_video_list(const char *title, bili_video_list_t *list,
                              int selected, int scroll_off) {
    draw_rect(0, 0, TOP_W, 32, CLR_PRIMARY);
    draw_text(12, 8, 0.55f, CLR_WHITE, "%s", title);
    draw_text(TOP_W - 80, 8, 0.45f, CLR_WHITE, "B-Back");

    if (!list || list->count == 0) {
        draw_text(60, 100, 0.55f, CLR_TEXT_LT, "No videos");
        return;
    }

    int y = 36;
    int visible = (TOP_H - 36 - MARGIN) / LIST_ITEM_H;
    if (visible > MAX_VISIBLE_ITEMS) visible = MAX_VISIBLE_ITEMS;

    for (int i = scroll_off; i < list->count && i < scroll_off + visible; i++) {
        bili_video_t *v = &list->videos[i];
        int item_y = y + (i - scroll_off) * LIST_ITEM_H;
        int item_h = LIST_ITEM_H;

        if (i == selected) {
            draw_rect(MARGIN, item_y, TOP_W - MARGIN * 2, item_h, CLR_SEL);
        }

        draw_rect(MARGIN, item_y, 22, item_h, CLR_RED);

        char disp_title[60];
        int tl = strlen(v->title);
        if (tl > 52) {
            strncpy(disp_title, v->title, 49);
            disp_title[49] = '.';
            disp_title[50] = '.';
            disp_title[51] = '.';
            disp_title[52] = '\0';
        } else {
            strcpy(disp_title, v->title);
        }
        draw_text(MARGIN + 28, item_y + 4, 0.50f, CLR_TEXT, "%s", disp_title);
        draw_text(MARGIN + 28, item_y + 16, 0.35f, CLR_TEXT_LT, "%s", v->author);

        if (i < list->count - 1) {
            draw_rect(MARGIN, item_y + item_h - 1, TOP_W - MARGIN * 2, 1,
                      C2D_Color32(0xE0, 0xE0, 0xE0, 0xFF));
        }
    }

    if (scroll_off > 0)
        draw_text(TOP_W / 2 - 10, TOP_H - 14, 0.4f, CLR_TEXT_LT, "^");
    if (scroll_off + visible < list->count)
        draw_text(TOP_W / 2 - 10, TOP_H - 14, 0.4f, CLR_TEXT_LT, "v");
}

static void render_search_input(const char *search_text) {
    draw_rect(0, 0, TOP_W, 40, CLR_PRIMARY);
    draw_text(16, 12, 0.55f, CLR_WHITE, "Search");

    draw_text(MARGIN, 60, 0.5f, CLR_TEXT, "Input: %s", search_text);
    draw_text(MARGIN, 90, 0.4f, CLR_TEXT_LT, "Type on bottom screen");
    draw_text(MARGIN, 110, 0.4f, CLR_TEXT_LT, "B to go back");
    draw_text(MARGIN, 130, 0.4f, CLR_TEXT_LT, "Enter to search");
}

static void render_video_detail(bili_video_t *video) {
    draw_rect(0, 0, TOP_W, 40, CLR_PRIMARY);
    draw_text(12, 12, 0.55f, CLR_WHITE, "Detail");
    draw_text(TOP_W - 80, 12, 0.45f, CLR_WHITE, "B-Back");

    int y = 50;
    draw_rect(MARGIN, y, TOP_W - MARGIN * 2, 90, CLR_CARD);

    draw_text(MARGIN + 8, y + 6, 0.55f, CLR_TEXT, "%s", video->title);
    draw_text(MARGIN + 8, y + 32, 0.45f, CLR_TEXT_LT, "By: %s", video->author);

    int mins = video->duration / 60;
    int secs = video->duration % 60;
    draw_text(MARGIN + 8, y + 54, 0.40f, CLR_TEXT_LT,
              "%d:%d | %d plays", mins, secs, video->play_count);

    int btn_w = TOP_W - MARGIN * 2;
    int btn_y = 155;
    draw_rect(MARGIN, btn_y, btn_w, 40, CLR_PRIMARY);
    draw_text(TOP_W / 2 - 30, btn_y + 12, 0.55f, CLR_WHITE, "Play");
}

static void render_playing(player_info_t *pinfo) {
    draw_rect(0, 0, TOP_W, TOP_H, C2D_Color32(0x00, 0x00, 0x00, 0xFF));

    if (pinfo->state == PLAYER_LOADING) {
        draw_text(TOP_W / 2 - 60, TOP_H / 2 - 10, 0.55f, CLR_WHITE, "Loading...");
    } else if (pinfo->state == PLAYER_PLAYING) {
        draw_text(TOP_W / 2 - 60, TOP_H / 2 - 10, 0.55f, CLR_WHITE, "Playing");
        draw_text(TOP_W / 2 - 60, TOP_H / 2 + 16, 0.40f, CLR_TEXT_LT,
                  "Frame %d/%d", pinfo->current_frame, pinfo->total_frames);
    } else if (pinfo->state == PLAYER_ERROR) {
        draw_text(TOP_W / 2 - 60, TOP_H / 2 - 10, 0.55f, CLR_RED, "Error");
    } else if (pinfo->state == PLAYER_DONE) {
        draw_text(TOP_W / 2 - 60, TOP_H / 2 - 10, 0.55f, CLR_WHITE, "Done");
    }
}

/* Touch keyboard on bottom screen */
static void render_keyboard(const char *search_text, int shift, int page) {
    draw_rect(0, 0, BOT_W, BOT_H, C2D_Color32(0xCC, 0xCC, 0xCC, 0xFF));
    draw_rect(0, 0, BOT_W, 24, CLR_WHITE);
    draw_text(4, 4, 0.45f, CLR_TEXT, "%s", search_text);

    int kx = 4, ky = 28;
    int kw = KB_KEY_W, kh = KB_KEY_H, gap = KB_GAP;

    const char *rows[4];
    if (shift) {
        rows[0] = "QWERTYUIOP";
        rows[1] = "ASDFGHJKL";
        rows[2] = "ZXCVBNM";
    } else {
        rows[0] = "qwertyuiop";
        rows[1] = "asdfghjkl";
        rows[2] = "zxcvbnm";
    }
    rows[3] = ".,?!@-_:;()";

    for (int r = 0; r < 4; r++) {
        int start_x = kx + (BOT_W - (int)strlen(rows[r]) * (kw + gap)) / 2;
        for (int c = 0; rows[r][c]; c++) {
            int bx = start_x + c * (kw + gap);
            int by = ky + r * (kh + gap);
            draw_rect(bx, by, kw, kh, CLR_WHITE);
            char ch[2] = {rows[r][c], '\0'};
            draw_text(bx + (kw - 8) / 2, by + 4, 0.40f, CLR_TEXT, ch);
        }
    }

    int ctrl_y = ky + 4 * (kh + gap) + 4;
    int gap2 = 4;
    int key_w = (BOT_W - 5 * gap2) / 6;

    draw_rect(gap2, ctrl_y, key_w, 24, CLR_PRIMARY);
    draw_text(gap2 + 2, ctrl_y + 4, 0.35f, CLR_WHITE, shift ? "ABC" : "abc");

    draw_rect(gap2 * 2 + key_w, ctrl_y, key_w * 2, 24, CLR_WHITE);
    draw_text(gap2 * 2 + key_w + 10, ctrl_y + 4, 0.35f, CLR_TEXT, "Space");

    int bx3 = gap2 * 3 + key_w * 3;
    draw_rect(bx3, ctrl_y, key_w, 24, C2D_Color32(0xFF, 0x99, 0x66, 0xFF));
    draw_text(bx3 + 4, ctrl_y + 4, 0.35f, CLR_WHITE, "DEL");

    int bx4 = gap2 * 4 + key_w * 4;
    draw_rect(bx4, ctrl_y, key_w, 24, C2D_Color32(0xFF, 0x66, 0x66, 0xFF));
    draw_text(bx4 + 4, ctrl_y + 4, 0.35f, CLR_WHITE, "Clr");

    int bx5 = gap2 * 5 + key_w * 5;
    draw_rect(bx5, ctrl_y, key_w, 24, C2D_Color32(0x00, 0xCC, 0x66, 0xFF));
    draw_text(bx5 + 4, ctrl_y + 4, 0.35f, CLR_WHITE, "Go");
}

static void render_bottom_default(app_screen_t screen) {
    draw_rect(0, 0, BOT_W, BOT_H, C2D_Color32(0xE8, 0xE8, 0xE8, 0xFF));
    switch (screen) {
        case SCREEN_MAIN_MENU:
            draw_text(16, 20, 0.45f, CLR_TEXT_LT, "A - Select");
            draw_text(16, 50, 0.40f, CLR_TEXT_LT, "START - Exit");
            break;
        case SCREEN_POPULAR:
        case SCREEN_SEARCH_RESULTS:
            draw_text(16, 20, 0.45f, CLR_TEXT_LT, "A - Open video");
            draw_text(16, 50, 0.40f, CLR_TEXT_LT, "B - Go back");
            draw_text(16, 80, 0.40f, CLR_TEXT_LT, "UP/DOWN - Navigate");
            break;
        case SCREEN_VIDEO_DETAIL:
            draw_text(16, 20, 0.45f, CLR_TEXT_LT, "A - Play");
            break;
        case SCREEN_PLAYING:
            draw_text(16, 20, 0.45f, CLR_TEXT_LT, "X - Pause");
            draw_text(16, 50, 0.40f, CLR_TEXT_LT, "B - Stop");
            break;
        default:
            break;
    }
}

/*------------------- Public API -------------------*/

int ui_init(void) {
    gfxInitDefault();
    C3D_Init(0x100000);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    font = C2D_FontLoadSystem(1);
    if (!font) return -1;

    textbuf = C2D_TextBufNew(8192);
    if (!textbuf) return -1;

    top_target = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bot_target = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    if (!top_target || !bot_target) return -1;

    return 0;
}

void ui_exit(void) {
    if (textbuf) C2D_TextBufDelete(textbuf);
    if (font) C2D_FontFree(font);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
}

void ui_render(app_state_t *state) {
    if (!state) return;

    player_info_t pinfo;
    player_get_info(&pinfo);

    /* Phase 1: Clear text buffer, parse ALL text (before C3D_FrameBegin) */
    text_prep_pass = 1;
    text_n = 0;
    C2D_TextBufClear(textbuf);

    switch (state->current_screen) {
        case SCREEN_MAIN_MENU:
            render_main_menu();
            break;
        case SCREEN_POPULAR:
            render_video_list("Popular", &state->popular_list,
                              state->selected_index, state->scroll_offset);
            break;
        case SCREEN_SEARCH_INPUT:
            render_search_input(state->search_text);
            break;
        case SCREEN_SEARCH_RESULTS:
            render_video_list("Results", &state->search_list,
                              state->selected_index, state->scroll_offset);
            break;
        case SCREEN_VIDEO_DETAIL:
            render_video_detail(&state->current_video);
            break;
        case SCREEN_PLAYING:
            render_playing(&pinfo);
            break;
        default:
            break;
    }

    /* Render top screen keywords */
    // Handle bottom screen text too
    if (state->current_screen == SCREEN_SEARCH_INPUT) {
        render_keyboard(state->search_text, state->kb_shift, state->kb_page);
    } else {
        render_bottom_default(state->current_screen);
    }

    /* Phase 2: Render everything (during C3D_FrameBegin/End) */
    text_prep_pass = 0;
    text_draw_idx = 0;

    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

    C2D_TargetClear(top_target, CLR_BG);
    C2D_SceneBegin(top_target);
    switch (state->current_screen) {
        case SCREEN_MAIN_MENU:
            render_main_menu();
            break;
        case SCREEN_POPULAR:
            render_video_list("Popular", &state->popular_list,
                              state->selected_index, state->scroll_offset);
            break;
        case SCREEN_SEARCH_INPUT:
            render_search_input(state->search_text);
            break;
        case SCREEN_SEARCH_RESULTS:
            render_video_list("Results", &state->search_list,
                              state->selected_index, state->scroll_offset);
            break;
        case SCREEN_VIDEO_DETAIL:
            render_video_detail(&state->current_video);
            break;
        case SCREEN_PLAYING:
            render_playing(&pinfo);
            break;
        default:
            break;
    }

    C2D_TargetClear(bot_target, CLR_BG);
    C2D_SceneBegin(bot_target);
    if (state->current_screen == SCREEN_SEARCH_INPUT) {
        render_keyboard(state->search_text, state->kb_shift, state->kb_page);
    } else {
        render_bottom_default(state->current_screen);
    }

    C3D_FrameEnd(0);
}

/* Touch keyboard hit detection */
static int handle_kb_touch(app_state_t *state, int px, int py) {
    int kx = 4, ky = 28;
    int kw = KB_KEY_W, kh = KB_KEY_H, gap = KB_GAP;

    const char *rows[4];
    if (state->kb_shift) {
        rows[0] = "QWERTYUIOP";
        rows[1] = "ASDFGHJKL";
        rows[2] = "ZXCVBNM";
    } else {
        rows[0] = "qwertyuiop";
        rows[1] = "asdfghjkl";
        rows[2] = "zxcvbnm";
    }
    rows[3] = ".,?!@-_:;()";

    for (int r = 0; r < 4; r++) {
        int start_x = kx + (BOT_W - (int)strlen(rows[r]) * (kw + gap)) / 2;
        for (int c = 0; rows[r][c]; c++) {
            int bx = start_x + c * (kw + gap);
            int by = ky + r * (kh + gap);
            if (px >= bx && px < bx + kw && py >= by && py < by + kh) {
                int len = strlen(state->search_text);
                if (len < (int)sizeof(state->search_text) - 2) {
                    state->search_text[len] = rows[r][c];
                    state->search_text[len + 1] = '\0';
                    state->search_text_pos = len + 1;
                }
                return 1;
            }
        }
    }

    int ctrl_y = ky + 4 * (kh + gap) + 4;
    int gap2 = 4;
    int key_w = (BOT_W - 5 * gap2) / 6;

    if (px >= gap2 && px < gap2 + key_w && py >= ctrl_y && py < ctrl_y + 24) {
        state->kb_shift = !state->kb_shift;
        return 1;
    }
    if (px >= gap2 * 2 + key_w && px < gap2 * 2 + key_w + key_w * 2 &&
        py >= ctrl_y && py < ctrl_y + 24) {
        int len = strlen(state->search_text);
        if (len < (int)sizeof(state->search_text) - 2) {
            state->search_text[len] = ' ';
            state->search_text[len + 1] = '\0';
            state->search_text_pos = len + 1;
        }
        return 1;
    }
    if (px >= gap2 * 3 + key_w * 3 && px < gap2 * 3 + key_w * 3 + key_w &&
        py >= ctrl_y && py < ctrl_y + 24) {
        int len = strlen(state->search_text);
        if (len > 0) {
            state->search_text[len - 1] = '\0';
            state->search_text_pos = len - 1;
        }
        return 1;
    }
    if (px >= gap2 * 4 + key_w * 4 && px < gap2 * 4 + key_w * 4 + key_w &&
        py >= ctrl_y && py < ctrl_y + 24) {
        state->search_text[0] = '\0';
        state->search_text_pos = 0;
        return 1;
    }
    if (px >= gap2 * 5 + key_w * 5 && px < gap2 * 5 + key_w * 5 + key_w &&
        py >= ctrl_y && py < ctrl_y + 24) {
        if (strlen(state->search_text) > 0) {
            state->current_screen = SCREEN_SEARCH_RESULTS;
            state->selected_index = 0;
            state->scroll_offset = 0;
            return 2;
        }
        return 1;
    }

    return 0;
}

static int handle_main_menu_touch(app_state_t *state, int px, int py) {
    int btn_w = 200, btn_h = 48;
    int bx = (TOP_W - btn_w) / 2;
    int by = 70;

    if (px >= bx && px < bx + btn_w && py >= by && py < by + btn_h) {
        state->current_screen = SCREEN_POPULAR;
        state->selected_index = 0;
        state->scroll_offset = 0;
        return 1;
    }
    if (px >= bx && px < bx + btn_w && py >= by + 70 && py < by + 70 + btn_h) {
        state->current_screen = SCREEN_SEARCH_INPUT;
        state->search_text[0] = '\0';
        state->search_text_pos = 0;
        state->kb_shift = 0;
        return 1;
    }
    return 0;
}

static int handle_list_touch(app_state_t *state, int px, int py, int count) {
    int y = 36;
    int visible = (TOP_H - 36 - MARGIN) / LIST_ITEM_H;
    if (visible > MAX_VISIBLE_ITEMS) visible = MAX_VISIBLE_ITEMS;

    for (int i = state->scroll_offset;
         i < count && i < state->scroll_offset + visible; i++) {
        int item_y = y + (i - state->scroll_offset) * LIST_ITEM_H;
        if (px >= MARGIN && px < TOP_W - MARGIN &&
            py >= item_y && py < item_y + LIST_ITEM_H) {
            state->selected_index = i;
            return 2;
        }
    }
    return 0;
}

int ui_handle_touch(app_state_t *state, touchPosition *touch) {
    if (!state || !touch) return 0;
    int px = touch->px, py = touch->py;

    if (py >= 240) {
        if (state->current_screen == SCREEN_SEARCH_INPUT) {
            return handle_kb_touch(state, px, py - 240);
        }
        return 0;
    }

    switch (state->current_screen) {
        case SCREEN_MAIN_MENU:
            return handle_main_menu_touch(state, px, py);
        case SCREEN_POPULAR:
            return handle_list_touch(state, px, py, state->popular_list.count);
        case SCREEN_SEARCH_RESULTS:
            return handle_list_touch(state, px, py, state->search_list.count);
        case SCREEN_VIDEO_DETAIL: {
            int btn_w = TOP_W - MARGIN * 2;
            int btn_y = 155;
            if (px >= MARGIN && px < MARGIN + btn_w &&
                py >= btn_y && py < btn_y + 40) {
                return 2;
            }
            return 0;
        }
        default:
            return 0;
    }
}

int ui_handle_keys(app_state_t *state, u32 keys_down) {
    if (!state) return 0;

    switch (state->current_screen) {
        case SCREEN_MAIN_MENU:
            if (keys_down & KEY_A) {
                state->current_screen = SCREEN_POPULAR;
                state->selected_index = 0;
                state->scroll_offset = 0;
                return 1;
            }
            if (keys_down & KEY_X) {
                state->current_screen = SCREEN_SEARCH_INPUT;
                state->search_text[0] = '\0';
                state->search_text_pos = 0;
                state->kb_shift = 0;
                return 1;
            }
            break;

        case SCREEN_POPULAR:
        case SCREEN_SEARCH_RESULTS: {
            int count = (state->current_screen == SCREEN_POPULAR)
                        ? state->popular_list.count
                        : state->search_list.count;
            int visible = (TOP_H - 36 - MARGIN) / LIST_ITEM_H;

            if (keys_down & KEY_DOWN) {
                if (state->selected_index < count - 1) {
                    state->selected_index++;
                    if (state->selected_index >= state->scroll_offset + visible)
                        state->scroll_offset++;
                    return 1;
                }
            }
            if (keys_down & KEY_UP) {
                if (state->selected_index > 0) {
                    state->selected_index--;
                    if (state->selected_index < state->scroll_offset)
                        state->scroll_offset--;
                    return 1;
                }
            }
            if (keys_down & KEY_A) {
                bili_video_t *video = (state->current_screen == SCREEN_POPULAR)
                    ? &state->popular_list.videos[state->selected_index]
                    : &state->search_list.videos[state->selected_index];
                state->current_video = *video;
                state->current_screen = SCREEN_VIDEO_DETAIL;
                state->prev_screen = state->current_screen;
                return 1;
            }
            if (keys_down & KEY_B) {
                state->current_screen = SCREEN_MAIN_MENU;
                return 1;
            }
            break;
        }

        case SCREEN_SEARCH_INPUT:
            if (keys_down & KEY_B) {
                state->current_screen = SCREEN_MAIN_MENU;
                return 1;
            }
            break;

        case SCREEN_VIDEO_DETAIL:
            if (keys_down & KEY_A) {
                return 2;
            }
            if (keys_down & KEY_B) {
                state->current_screen = SCREEN_POPULAR;
                return 1;
            }
            break;

        case SCREEN_PLAYING:
            if (keys_down & KEY_X) {
                player_pause();
                return 1;
            }
            if (keys_down & KEY_B) {
                player_stop();
                state->current_screen = SCREEN_VIDEO_DETAIL;
                return 1;
            }
            break;

        default:
            break;
    }
    return 0;
}