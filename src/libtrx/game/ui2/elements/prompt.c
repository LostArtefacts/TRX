#include "game/ui2/elements/prompt.h"

#include "game/const.h"
#include "game/input.h"
#include "game/ui2/common.h"
#include "game/ui2/elements/flash.h"
#include "game/ui2/elements/label.h"
#include "game/ui2/events.h"
#include "game/ui2/helpers.h"
#include "log.h"
#include "memory.h"
#include "strings.h"
#include "utils.h"

#include <string.h>

typedef struct {
    UI2_PROMPT_STATE *state;
} M_DATA;

static const char m_ValidPromptChars[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-: ";

static void M_Layout(UI2_NODE *node, float x, float y, float w, float h);

static const UI2_WIDGET_OPS m_Ops = {
    .measure = UI2_MeasureWrapper,
    .layout = M_Layout,
    .draw = UI2_DrawWrapper,
};

static void M_MoveCaretLeft(UI2_PROMPT_STATE *s);
static void M_MoveCaretRight(UI2_PROMPT_STATE *s);
static void M_MoveCaretStart(UI2_PROMPT_STATE *s);
static void M_MoveCaretEnd(UI2_PROMPT_STATE *s);
static void M_DeleteCharBack(UI2_PROMPT_STATE *s);
static void M_Confirm(UI2_PROMPT_STATE *s);
static void M_Cancel(UI2_PROMPT_STATE *s);
static void M_Clear(UI2_PROMPT_STATE *s);

static void M_HandleKeyDown(const EVENT *event, void *user_data);
static void M_HandleTextEdit(const EVENT *event, void *user_data);

static void M_Layout(
    UI2_NODE *const node, const float x, const float y, const float w,
    const float h)
{
    UI2_LayoutBasic(node, x, y, w, h);
    const M_DATA *const data = node->data;
    const UI2_PROMPT_STATE *const s = data->state;
    UI2_NODE *const prompt = node->first_child;
    UI2_NODE *const caret = prompt->next_sibling;
    prompt->ops->layout(prompt, x, y, w, h);

    const char old = s->current_text[s->caret_pos];
    s->current_text[s->caret_pos] = '\0';
    float caret_pos;
    UI2_Label_Measure(s->current_text, &caret_pos, nullptr);
    s->current_text[s->caret_pos] = old;

    caret->ops->layout(caret, x + caret_pos, y, w, h);
}

static void M_MoveCaretLeft(UI2_PROMPT_STATE *const s)
{
    if (s->caret_pos > 0) {
        s->caret_pos--;
    }
}

static void M_MoveCaretRight(UI2_PROMPT_STATE *const s)
{
    if (s->caret_pos < (int32_t)strlen(s->current_text)) {
        s->caret_pos++;
    }
}

static void M_MoveCaretStart(UI2_PROMPT_STATE *const s)
{
    s->caret_pos = 0;
}

static void M_MoveCaretEnd(UI2_PROMPT_STATE *const s)
{
    s->caret_pos = strlen(s->current_text);
}

static void M_DeleteCharBack(UI2_PROMPT_STATE *const s)
{
    if (s->caret_pos <= 0) {
        return;
    }

    memmove(
        s->current_text + s->caret_pos - 1, s->current_text + s->caret_pos,
        strlen(s->current_text) + 1 - s->caret_pos);
    s->caret_pos--;
}

static void M_Confirm(UI2_PROMPT_STATE *const s)
{
    if (String_IsEmpty(s->current_text)) {
        M_Cancel(s);
        return;
    }
    UI2_FireEvent((EVENT) {
        .name = "confirm",
        .sender = s,
        .data = s->current_text,
    });
    M_Clear(s);
}

static void M_Cancel(UI2_PROMPT_STATE *const s)
{
    UI2_FireEvent((EVENT) {
        .name = "cancel",
        .sender = s,
        .data = s->current_text,
    });
    M_Clear(s);
}

static void M_Clear(UI2_PROMPT_STATE *const s)
{
    strcpy(s->current_text, "");
    s->caret_pos = 0;
}

static void M_HandleKeyDown(const EVENT *const event, void *const user_data)
{
    const UI2_INPUT key = (UI2_INPUT)(uintptr_t)event->data;
    UI2_PROMPT_STATE *const s = user_data;

    if (!s->is_focused) {
        return;
    }

    // clang-format off
    switch (key) {
    case UI_KEY_LEFT:   M_MoveCaretLeft(s); break;
    case UI_KEY_RIGHT:  M_MoveCaretRight(s); break;
    case UI_KEY_HOME:   M_MoveCaretStart(s); break;
    case UI_KEY_END:    M_MoveCaretEnd(s); break;
    case UI_KEY_BACK:   M_DeleteCharBack(s); break;
    case UI_KEY_RETURN: M_Confirm(s); break;
    case UI_KEY_ESCAPE: M_Cancel(s); break;
    default:            break;
    }
    // clang-format on
}

static void M_HandleTextEdit(const EVENT *const event, void *const user_data)
{
    const char *insert_string = event->data;
    const size_t insert_length = strlen(insert_string);
    UI2_PROMPT_STATE *const s = user_data;

    if (!s->is_focused) {
        return;
    }

    if (strlen(insert_string) != 1
        || strstr(m_ValidPromptChars, insert_string) == nullptr) {
        return;
    }

    const size_t available_space =
        s->current_text_capacity - strlen(s->current_text);
    if (insert_length >= available_space) {
        s->current_text_capacity *= 2;
        s->current_text =
            Memory_Realloc(s->current_text, s->current_text_capacity);
    }

    memmove(
        s->current_text + s->caret_pos + insert_length,
        s->current_text + s->caret_pos,
        strlen(s->current_text) + 1 - s->caret_pos);
    memcpy(s->current_text + s->caret_pos, insert_string, insert_length);

    s->caret_pos += insert_length;
}

void UI2_Prompt_Init(UI2_PROMPT_STATE *const s)
{
    s->is_focused = false;
    s->current_text_capacity = 30;
    s->current_text = Memory_Alloc(s->current_text_capacity);
    s->listener1 = UI2_Subscribe("key_down", nullptr, M_HandleKeyDown, s);
    s->listener2 = UI2_Subscribe("text_edit", nullptr, M_HandleTextEdit, s);
    UI2_Flash_Init(&s->flash, LOGIC_FPS * 2 / 3);
}

void UI2_Prompt_Free(UI2_PROMPT_STATE *const s)
{
    UI2_Unsubscribe(s->listener1);
    UI2_Unsubscribe(s->listener2);
    UI2_Flash_Free(&s->flash);
    Memory_FreePointer(&s->current_text);
}

void UI2_Prompt_Control(UI2_PROMPT_STATE *const s)
{
    UI2_Flash_Control(&s->flash);
}

void UI2_Prompt(UI2_PROMPT_STATE *const s)
{
    UI2_NODE *const node = UI2_AllocNode(&m_Ops, sizeof(M_DATA));
    M_DATA *const data = node->data;
    data->state = s;
    UI2_AddChild(node);
    UI2_PushCurrent(node);
    UI2_LabelEx(
        s->current_text != nullptr ? s->current_text : "",
        (UI2_LABEL_SETTINGS) { .scale = 1.0f, .z = 16 });
    if (s->is_focused) {
        UI2_BeginFlash(&s->flash);
    }
    UI2_LabelEx(
        "\\{button left}", (UI2_LABEL_SETTINGS) { .scale = 1.0f, .z = 8 });
    if (s->is_focused) {
        UI2_EndFlash();
    }
    UI2_PopCurrent();
}

void UI2_Prompt_SetFocus(UI2_PROMPT_STATE *const s, const bool is_focused)
{
    if (s->is_focused == is_focused) {
        return;
    }
    s->is_focused = is_focused;
    s->flash.count = 0;
    if (is_focused) {
        Input_EnterListenMode();
    } else {
        Input_ExitListenMode();
    }
}

void UI2_Prompt_Clear(UI2_PROMPT_STATE *const s)
{
    M_Clear(s);
}

void UI2_Prompt_ChangeText(
    UI2_PROMPT_STATE *const s, const char *const new_text)
{
    Memory_FreePointer(&s->current_text);
    s->current_text = Memory_DupStr(new_text);
    s->caret_pos = strlen(new_text);
}
