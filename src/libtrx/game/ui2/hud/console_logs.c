#include "game/ui2/hud/console_logs.h"

#include "debug.h"
#include "game/clock.h"
#include "game/text.h"
#include "game/ui2/elements/label.h"
#include "game/ui2/elements/stack.h"
#include "game/ui2/events.h"
#include "memory.h"
#include "strings.h"

#include <string.h>

#define M_LOG_SCALE 0.8f
#define M_MAX_LOG_LINES 20
#define M_DELAY_PER_CHAR 0.2

static void M_ScrollLogs(UI2_CONSOLE_LOGS *s);
static void M_UpdateLogCount(UI2_CONSOLE_LOGS *s);
static void M_HandleLog(const EVENT *event, void *user_data);

static void M_ScrollLogs(UI2_CONSOLE_LOGS *const s)
{
    int32_t i = s->max_lines - 1;
    while (i >= 0 && !s->logs[i].expire_at) {
        i--;
    }

    bool need_layout = false;
    while (i >= 0 && s->logs[i].expire_at
           && Clock_GetRealTime() >= s->logs[i].expire_at) {
        s->logs[i].expire_at = 0.0;
        Memory_FreePointer(&s->logs[i].text);
        need_layout = true;
        i--;
    }

    if (need_layout) {
        M_UpdateLogCount(s);
    }
}
static void M_UpdateLogCount(UI2_CONSOLE_LOGS *const s)
{
    s->vis_lines = 0;
    for (int32_t i = s->max_lines - 1; i >= 0; i--) {
        if (s->logs[i].expire_at != 0.0) {
            s->vis_lines = i + 1;
            break;
        }
    }
}

static void M_HandleLog(const EVENT *const event, void *const user_data)
{
    const char *text = event->data;
    UI2_CONSOLE_LOGS *const s = user_data;
    Memory_FreePointer(&s->logs[s->max_lines - 1].text);
    for (int32_t i = s->max_lines - 1; i > 0; i--) {
        s->logs[i] = s->logs[i - 1];
    }

    s->logs[0].expire_at =
        Clock_GetRealTime() + strlen(text) * M_DELAY_PER_CHAR;
    s->logs[0].text = String_WordWrap(text, Text_GetMaxLineLength());
    M_UpdateLogCount(s);
}

void UI2_ConsoleLogs_Init(UI2_CONSOLE_LOGS *const s)
{
    if (s->max_lines <= 0) {
        s->max_lines = M_MAX_LOG_LINES;
    }
    s->logs = Memory_Alloc(s->max_lines * sizeof(UI2_CONSOLE_LOG_LINE));
    s->vis_lines = 0;
    s->listener_id = UI2_Subscribe("console_log", nullptr, M_HandleLog, s);
}

void UI2_ConsoleLogs_Free(UI2_CONSOLE_LOGS *const s)
{
    Memory_FreePointer(&s->logs);
    UI2_Unsubscribe(s->listener_id);
}

void UI2_ConsoleLogs(UI2_CONSOLE_LOGS *const s)
{
    ASSERT(s != nullptr);
    M_ScrollLogs(s);
    UI2_BeginStackEx((UI2_STACK_SETTINGS) {
        .orientation = UI2_STACK_VERTICAL,
        .align = {
            .h = UI2_STACK_H_ALIGN_LEFT,
            .v = UI2_STACK_V_ALIGN_CENTER,
        },
    });
    for (int32_t i = s->vis_lines - 1; i >= 0; i--) {
        UI2_LabelEx(
            s->logs[i].text, (UI2_LABEL_SETTINGS) { .scale = M_LOG_SCALE });
    }
    UI2_EndStack();
}
