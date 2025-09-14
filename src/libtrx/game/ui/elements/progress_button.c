#include "game/ui/elements/progress_button.h"

#include "game/const.h"
#include "game/ui/elements/bar.h"
#include "game/ui/elements/label.h"
#include "game/ui/elements/pad.h"
#include "game/ui/elements/span.h"
#include "memory.h"
#include "strings.h"

#define M_HOLD_TIMER_DEBUFF (LOGIC_FPS / 3)
#define M_HOLD_TIMER_MAX LOGIC_FPS

struct UI_PROGRESS_BUTTON_STATE {
    GAME_STRING_ID text;
    INPUT_BACKEND backend;
    INPUT_ROLE role;
    UI_PROGRESS_BUTTON_CALLBACK func;
    void *func_arg;
    int32_t hold_timer;
};

UI_PROGRESS_BUTTON_STATE *UI_ProgressButton_Init(
    INPUT_BACKEND backend, INPUT_ROLE role, GAME_STRING_ID text,
    UI_PROGRESS_BUTTON_CALLBACK func, void *func_arg)
{
    UI_PROGRESS_BUTTON_STATE *const s =
        Memory_Alloc(sizeof(UI_PROGRESS_BUTTON_STATE));
    s->backend = backend;
    s->role = role;
    s->text = text;
    s->func = func;
    s->func_arg = func_arg;
    s->hold_timer = 0;
    return s;
}

void UI_ProgressButton_Control(UI_PROGRESS_BUTTON_STATE *const s)
{
    if (!Input_IsPressedEx(s->backend, INPUT_LAYOUT_DEFAULT, s->role)) {
        s->hold_timer = 0;
        return;
    }
    if (s->hold_timer != -1) {
        s->hold_timer++;
        if (s->hold_timer - M_HOLD_TIMER_DEBUFF > M_HOLD_TIMER_MAX) {
            s->func(s->func_arg);
            s->hold_timer = -1; // Debounce the key
        }
    }
}

void UI_ProgressButton_Free(UI_PROGRESS_BUTTON_STATE *s)
{
    Memory_Free(s);
}

void UI_ProgressButton(UI_PROGRESS_BUTTON_STATE *const s)
{
    const char *const key_name =
        Input_GetKeyName(s->backend, INPUT_LAYOUT_DEFAULT, s->role);
    if (key_name == nullptr) {
        return;
    }
    const char *const value_label =
        String_FormatStatic(GS(MISC_HOLD_FMT), key_name);

    const float pad[2] = { 6.0f, 3.0f };

    UI_BeginPad(0.0f, -pad[1]);
    UI_BeginSpan();
    if (s->hold_timer >= M_HOLD_TIMER_DEBUFF) {
        UI_Bar((UI_BAR_SETTINGS) {
            .type = UI_BAR_PROGRESS,
            .value = s->hold_timer - M_HOLD_TIMER_DEBUFF,
            .max_value = M_HOLD_TIMER_MAX,
            .w = 0.0, // Span will make it expand anyway!
            .h = 0.0,
        });
    }
    UI_BeginPad(pad[0], pad[1]);
    UI_LabelFmt("%s: %s", GameString_Get(s->text), value_label);
    UI_EndPad();
    UI_EndSpan();
    UI_EndPad();
}
