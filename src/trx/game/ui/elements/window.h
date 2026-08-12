#pragma once

#include <trx/game/ui/scrollable.h>

typedef void UI_WINDOW_CALLBACK(void *user_data);

typedef struct {
    const char *title;
    const UI_SCROLLABLE *scrollable;
    float title_spacing;
    bool heavy;
    UI_WINDOW_CALLBACK *header_func;
    UI_WINDOW_CALLBACK *footer_func;
    void *user_data;
    bool reserve_scroll_space;
} UI_WINDOW_SETTINGS;

void UI_BeginWindow(UI_WINDOW_SETTINGS settings);
void UI_EndWindow(void);

// The horizontal space a window spends on its own frame and padding, in the
// units a caller sizes its content in.
float UI_Window_GetChromeWidth(void);

// The vertical space a window spends on its own frame, padding, title and
// scroll hints. What a header or footer function draws is the caller's, and is
// not counted here.
float UI_Window_GetChromeHeight(const UI_WINDOW_SETTINGS *settings);
