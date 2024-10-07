#include "game/ui/widgets/console.h"

#include "game/clock.h"
#include "game/console/common.h"
#include "game/console/history.h"
#include "game/text.h"
#include "game/ui/common.h"
#include "game/ui/events.h"
#include "game/ui/widgets/label.h"
#include "game/ui/widgets/prompt.h"
#include "game/ui/widgets/spacer.h"
#include "game/ui/widgets/stack.h"
#include "memory.h"
#include "strings.h"
#include "utils.h"

#include <string.h>

#define WINDOW_MARGIN 5
#define MAX_LOG_LINES 20
#define MAX_LISTENERS 4
#define LOG_MARGIN 10
#define LOG_SCALE 0.8
#define DELAY_PER_CHAR 0.2

typedef struct {
    UI_WIDGET_VTABLE vtable;
    UI_WIDGET *container;
    UI_WIDGET *prompt;
    UI_WIDGET *spacer;
    char *log_lines;
    int32_t logs_on_screen;
    int32_t history_idx;

    int32_t listeners[MAX_LISTENERS];
    struct {
        double expire_at;
        UI_WIDGET *label;
    } logs[MAX_LOG_LINES];
} UI_CONSOLE;

static void M_MoveHistoryUp(UI_CONSOLE *self);
static void M_MoveHistoryDown(UI_CONSOLE *self);
static void M_HandlePromptCancel(const EVENT *event, void *data);
static void M_HandlePromptConfirm(const EVENT *event, void *data);
static void M_HandleCanvasResize(const EVENT *event, void *data);
static void M_UpdateLogCount(UI_CONSOLE *self);

static int32_t M_GetWidth(const UI_CONSOLE *self);
static int32_t M_GetHeight(const UI_CONSOLE *self);
static void M_SetPosition(UI_CONSOLE *self, int32_t x, int32_t y);
static void M_Control(UI_CONSOLE *self);
static void M_Draw(UI_CONSOLE *self);
static void M_Free(UI_CONSOLE *self);

static void M_MoveHistoryUp(UI_CONSOLE *const self)
{
    self->history_idx--;
    CLAMP(self->history_idx, 0, Console_History_GetLength());
    const char *const new_prompt = Console_History_Get(self->history_idx);
    if (new_prompt == NULL) {
        UI_Prompt_ChangeText(self->prompt, "");
    } else {
        UI_Prompt_ChangeText(self->prompt, new_prompt);
    }
}

static void M_MoveHistoryDown(UI_CONSOLE *const self)
{
    self->history_idx++;
    CLAMP(self->history_idx, 0, Console_History_GetLength());
    const char *const new_prompt = Console_History_Get(self->history_idx);
    if (new_prompt == NULL) {
        UI_Prompt_ChangeText(self->prompt, "");
    } else {
        UI_Prompt_ChangeText(self->prompt, new_prompt);
    }
}

static void M_HandlePromptCancel(const EVENT *const event, void *const data)
{
    Console_Close();
}

static void M_HandlePromptConfirm(const EVENT *const event, void *const data)
{
    UI_CONSOLE *const self = (UI_CONSOLE *)data;
    const char *text = event->data;
    Console_History_Append(text);
    Console_Eval(text);
    Console_Close();

    self->history_idx = Console_History_GetLength();
}

static void M_HandleCanvasResize(const EVENT *event, void *data)
{
    UI_CONSOLE *const self = (UI_CONSOLE *)data;
    UI_Stack_SetSize(self->container, M_GetWidth(self), M_GetHeight(self));
}

static void M_HandleKeyDown(const EVENT *const event, void *const user_data)
{
    if (!Console_IsOpened()) {
        return;
    }

    UI_CONSOLE *const self = user_data;
    const UI_INPUT key = (UI_INPUT)(uintptr_t)event->data;

    // clang-format off
    switch (key) {
    case UI_KEY_UP:   M_MoveHistoryUp(self); break;
    case UI_KEY_DOWN: M_MoveHistoryDown(self); break;
    default:          break;
    }
    // clang-format on
}

static void M_UpdateLogCount(UI_CONSOLE *const self)
{
    self->logs_on_screen = 0;
    for (int32_t i = MAX_LOG_LINES - 1; i >= 0; i--) {
        if (self->logs[i].expire_at) {
            self->logs_on_screen = i + 1;
            break;
        }
    }
}

static int32_t M_GetWidth(const UI_CONSOLE *const self)
{
    return UI_GetCanvasWidth() - 2 * WINDOW_MARGIN;
}

static int32_t M_GetHeight(const UI_CONSOLE *const self)
{
    return UI_GetCanvasHeight() - 2 * WINDOW_MARGIN;
}

static void M_SetPosition(UI_CONSOLE *const self, int32_t x, int32_t y)
{
    return self->container->set_position(self->container, x, y);
}

static void M_Control(UI_CONSOLE *const self)
{
    if (self->container->control != NULL) {
        self->container->control(self->container);
    }
}

static void M_Draw(UI_CONSOLE *const self)
{
    if (self->container->draw != NULL) {
        self->container->draw(self->container);
    }
}

static void M_Free(UI_CONSOLE *const self)
{
    self->spacer->free(self->spacer);
    self->prompt->free(self->prompt);
    self->container->free(self->container);
    for (int32_t i = 0; i < MAX_LISTENERS; i++) {
        UI_Events_Unsubscribe(self->listeners[i]);
    }
    Memory_Free(self);
}

UI_WIDGET *UI_Console_Create(void)
{
    UI_CONSOLE *const self = Memory_Alloc(sizeof(UI_CONSOLE));
    self->vtable = (UI_WIDGET_VTABLE) {
        .control = (UI_WIDGET_CONTROL)M_Control,
        .draw = (UI_WIDGET_DRAW)M_Draw,
        .get_width = (UI_WIDGET_GET_WIDTH)M_GetWidth,
        .get_height = (UI_WIDGET_GET_HEIGHT)M_GetHeight,
        .set_position = (UI_WIDGET_SET_POSITION)M_SetPosition,
        .free = (UI_WIDGET_FREE)M_Free,
    };

    self->container = UI_Stack_Create(
        UI_STACK_LAYOUT_VERTICAL, M_GetWidth(self), M_GetHeight(self));
    UI_Stack_SetVAlign(self->container, UI_STACK_V_ALIGN_BOTTOM);

    for (int32_t i = MAX_LOG_LINES - 1; i >= 0; i--) {
        self->logs[i].label =
            UI_Label_Create("", UI_LABEL_AUTO_SIZE, UI_LABEL_AUTO_SIZE);
        UI_Label_SetScale(self->logs[i].label, LOG_SCALE);
        UI_Stack_AddChild(self->container, self->logs[i].label);
    }

    self->spacer = UI_Spacer_Create(LOG_MARGIN, LOG_MARGIN);
    UI_Stack_AddChild(self->container, self->spacer);

    self->prompt = UI_Prompt_Create(UI_LABEL_AUTO_SIZE, UI_LABEL_AUTO_SIZE);
    UI_Stack_AddChild(self->container, self->prompt);

    M_SetPosition(self, WINDOW_MARGIN, WINDOW_MARGIN);

    int32_t i = 0;
    self->listeners[i++] = UI_Events_Subscribe(
        "confirm", self->prompt, M_HandlePromptConfirm, self);
    self->listeners[i++] =
        UI_Events_Subscribe("cancel", self->prompt, M_HandlePromptCancel, NULL);
    self->listeners[i++] =
        UI_Events_Subscribe("canvas_resize", NULL, M_HandleCanvasResize, self);
    self->listeners[i++] =
        UI_Events_Subscribe("key_down", NULL, M_HandleKeyDown, self);

    return (UI_WIDGET *)self;
}

void UI_Console_HandleOpen(UI_WIDGET *const widget)
{
    UI_CONSOLE *const self = (UI_CONSOLE *)widget;
    UI_Prompt_SetFocus(self->prompt, true);
    self->history_idx = Console_History_GetLength();
}

void UI_Console_HandleClose(UI_WIDGET *const widget)
{
    UI_CONSOLE *const self = (UI_CONSOLE *)widget;
    UI_Prompt_SetFocus(self->prompt, false);
    UI_Prompt_Clear(self->prompt);
}

void UI_Console_HandleLog(UI_WIDGET *const widget, const char *const text)
{
    UI_CONSOLE *const self = (UI_CONSOLE *)widget;

    int32_t dst_idx = -1;
    for (int32_t i = MAX_LOG_LINES - 1; i > 0; i--) {
        if (self->logs[i].label == NULL) {
            continue;
        }
        UI_Label_ChangeText(
            self->logs[i].label, UI_Label_GetText(self->logs[i - 1].label));
        self->logs[i].expire_at = self->logs[i - 1].expire_at;
    }

    if (self->logs[0].label == NULL) {
        return;
    }

    self->logs[0].expire_at =
        Clock_GetHighPrecisionCounter() + 1000 * strlen(text) * DELAY_PER_CHAR;

    char *wrapped = String_WordWrap(text, Text_GetMaxLineLength());
    UI_Label_ChangeText(self->logs[0].label, wrapped);
    Memory_FreePointer(&wrapped);

    UI_Stack_DoLayout(self->container);
    M_UpdateLogCount(self);
}

void UI_Console_ScrollLogs(UI_WIDGET *const widget)
{
    UI_CONSOLE *const self = (UI_CONSOLE *)widget;

    int32_t i = MAX_LOG_LINES - 1;
    while (i >= 0 && !self->logs[i].expire_at) {
        i--;
    }

    bool need_layout = false;
    while (i >= 0 && self->logs[i].expire_at
           && Clock_GetHighPrecisionCounter() >= self->logs[i].expire_at) {
        self->logs[i].expire_at = 0;
        UI_Label_ChangeText(self->logs[i].label, "");
        need_layout = true;
        i--;
    }

    if (need_layout) {
        M_UpdateLogCount(self);
        UI_Stack_DoLayout(self->container);
    }
}

int32_t UI_Console_GetVisibleLogCount(UI_WIDGET *const widget)
{
    UI_CONSOLE *const self = (UI_CONSOLE *)widget;
    return self->logs_on_screen;
}

int32_t UI_Console_GetMaxLogCount(UI_WIDGET *const widget)
{
    return MAX_LOG_LINES;
}
