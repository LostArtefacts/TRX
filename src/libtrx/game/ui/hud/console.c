#include "game/ui/hud/console.h"

#include "game/console.h"
#include "game/text.h"
#include "game/ui/elements/modal.h"
#include "game/ui/elements/pad.h"
#include "game/ui/elements/prompt.h"
#include "game/ui/elements/spacer.h"
#include "game/ui/elements/stack.h"
#include "game/ui/events.h"
#include "game/ui/helpers.h"
#include "game/ui/hud/console_logs.h"
#include "utils.h"

static void M_Draw(const UI_NODE *node);

static const UI_WIDGET_OPS m_Ops = {
    .measure = UI_MeasureWrapper,
    .layout = UI_LayoutWrapper,
    .draw = M_Draw,
};

static void M_MoveHistoryUp(UI_CONSOLE_STATE *s);
static void M_MoveHistoryDown(UI_CONSOLE_STATE *s);
static void M_HandleOpen(const EVENT *event, void *user_data);
static void M_HandleClose(const EVENT *event, void *user_data);
static void M_HandleCancel(const EVENT *event, void *user_data);
static void M_HandleConfirm(const EVENT *event, void *user_data);

static void M_MoveHistoryUp(UI_CONSOLE_STATE *const s)
{
    s->history_idx--;
    CLAMP(s->history_idx, 0, Console_History_GetLength());
    const char *const new_prompt = Console_History_Get(s->history_idx);
    UI_Prompt_ChangeText(&s->prompt, new_prompt == nullptr ? "" : new_prompt);
}

static void M_MoveHistoryDown(UI_CONSOLE_STATE *const s)
{
    s->history_idx++;
    CLAMP(s->history_idx, 0, Console_History_GetLength());
    const char *const new_prompt = Console_History_Get(s->history_idx);
    UI_Prompt_ChangeText(&s->prompt, new_prompt == nullptr ? "" : new_prompt);
}

static void M_HandleKeyDown(const EVENT *const event, void *const user_data)
{
    if (!Console_IsOpened()) {
        return;
    }

    UI_CONSOLE_STATE *const s = user_data;
    const UI_INPUT key = (UI_INPUT)(uintptr_t)event->data;

    // clang-format off
    switch (key) {
    case UI_KEY_UP:   M_MoveHistoryUp(s); break;
    case UI_KEY_DOWN: M_MoveHistoryDown(s); break;
    default:          break;
    }
    // clang-format on
}

static void M_HandleOpen(const EVENT *event, void *user_data)
{
    UI_CONSOLE_STATE *const s = user_data;
    UI_Prompt_SetFocus(&s->prompt, true);
    s->history_idx = Console_History_GetLength();
}

static void M_HandleClose(const EVENT *event, void *user_data)
{
    UI_CONSOLE_STATE *const s = user_data;
    UI_Prompt_SetFocus(&s->prompt, false);
    UI_Prompt_Clear(&s->prompt);
}

static void M_HandleCancel(const EVENT *const event, void *const data)
{
    Console_Close();
}

static void M_HandleConfirm(const EVENT *event, void *user_data)
{
    UI_CONSOLE_STATE *const s = user_data;
    const char *text = event->data;
    Console_History_Append(text);
    Console_Eval(text);
    Console_Close();
    s->history_idx = Console_History_GetLength();
}

static void M_Draw(const UI_NODE *node)
{
    UI_CONSOLE_STATE *const s = *(UI_CONSOLE_STATE **)node->data;
    if (Console_IsOpened() || s->logs.vis_lines > 0) {
        Console_DrawBackdrop();
    }
    UI_DrawWrapper(node);
}

void UI_Console_Init(UI_CONSOLE_STATE *const s)
{
    UI_Prompt_Init(&s->prompt);
    UI_ConsoleLogs_Init(&s->logs);

    struct {
        const char *event_name;
        const void *sender;
        EVENT_LISTENER handler;
    } listeners[] = {
        { "console_open", nullptr, M_HandleOpen },
        { "console_close", nullptr, M_HandleClose },
        { "cancel", &s->prompt, M_HandleCancel },
        { "confirm", &s->prompt, M_HandleConfirm },
        { "key_down", nullptr, M_HandleKeyDown },
        { 0 },
    };
    for (int32_t i = 0; listeners[i].event_name != nullptr; i++) {
        s->listeners[i] = UI_Subscribe(
            listeners[i].event_name, listeners[i].sender, listeners[i].handler,
            s);
    }

    s->history_idx = -1;
}

void UI_Console_Free(UI_CONSOLE_STATE *const s)
{
    UI_ConsoleLogs_Free(&s->logs);
    UI_Prompt_Free(&s->prompt);
    for (int32_t i = 0; i < 5; i++) {
        UI_Unsubscribe(s->listeners[i]);
    }
}

void UI_Console_Control(UI_CONSOLE_STATE *const s)
{
    UI_Prompt_Control(&s->prompt);
}

void UI_Console(UI_CONSOLE_STATE *const s)
{
    UI_Prompt_SetFocus(&s->prompt, Console_IsOpened());

    UI_NODE *const node = UI_AllocNode(&m_Ops, sizeof(UI_CONSOLE_STATE *));
    *(UI_CONSOLE_STATE **)node->data = s;
    UI_AddChild(node);
    UI_PushCurrent(node);

    UI_BeginModal(0.0f, 1.0f);
    UI_BeginPad(5.0f, 5.0f);
    UI_BeginStack(UI_STACK_VERTICAL);

    UI_ConsoleLogs(&s->logs);
    UI_Spacer(0.0f, 8.0f);
    if (Console_IsOpened()) {
        UI_Prompt(&s->prompt);
    } else {
        UI_Spacer(0.0f, TEXT_HEIGHT_FIXED);
    }

    UI_EndStack();
    UI_EndModal();
    UI_EndPad();

    UI_PopCurrent();
}
