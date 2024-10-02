#include "game/ui/widgets/prompt.h"

#include "game/input.h"
#include "game/ui/common.h"
#include "game/ui/events.h"
#include "game/ui/widgets/label.h"
#include "memory.h"
#include "strings.h"

#include <string.h>

static const char m_ValidPromptChars[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.- ";

typedef struct {
    UI_WIDGET_VTABLE vtable;
    UI_WIDGET *label;
    UI_WIDGET *caret;

    int32_t listener1;
    int32_t listener2;

    struct {
        int32_t x;
        int32_t y;
    } pos;
    bool is_focused;
    int32_t current_text_capacity;
    char *current_text;
    int32_t caret_pos;
} UI_PROMPT;

static void M_UpdatePromptLabel(UI_PROMPT *self);
static void M_UpdateCaretLabel(UI_PROMPT *self);
static void M_MoveCaretLeft(UI_PROMPT *self);
static void M_MoveCaretRight(UI_PROMPT *self);
static void M_MoveCaretStart(UI_PROMPT *self);
static void M_MoveCaretEnd(UI_PROMPT *self);
static void M_DeleteCharBack(UI_PROMPT *self);
static void M_Confirm(UI_PROMPT *self);
static void M_Cancel(UI_PROMPT *self);
static void M_Clear(UI_PROMPT *self);

static int32_t M_GetWidth(const UI_PROMPT *self);
static int32_t M_GetHeight(const UI_PROMPT *self);
static void M_SetPosition(UI_PROMPT *self, int32_t x, int32_t y);
static void M_Control(UI_PROMPT *self);
static void M_Draw(UI_PROMPT *self);
static void M_Free(UI_PROMPT *self);
static void M_HandleKeyDown(const EVENT *event, void *user_data);
static void M_HandleTextEdit(const EVENT *event, void *user_data);

static void M_UpdatePromptLabel(UI_PROMPT *const self)
{
    UI_Label_ChangeText(self->label, self->current_text);
}

static void M_UpdateCaretLabel(UI_PROMPT *const self)
{
    const char old = self->current_text[self->caret_pos];
    self->current_text[self->caret_pos] = '\0';
    UI_Label_ChangeText(self->label, self->current_text);
    const int32_t width = UI_Label_MeasureTextWidth(self->label);
    self->current_text[self->caret_pos] = old;
    UI_Label_ChangeText(self->label, self->current_text);

    self->caret->set_position(self->caret, self->pos.x + width, self->pos.y);
}

static int32_t M_GetWidth(const UI_PROMPT *const self)
{
    return self->label->get_width(self->label);
}

static int32_t M_GetHeight(const UI_PROMPT *const self)
{
    return self->label->get_height(self->label);
}

static void M_SetPosition(
    UI_PROMPT *const self, const int32_t x, const int32_t y)
{
    self->pos.x = x;
    self->pos.y = y;
    self->label->set_position(self->label, x, y);
    M_UpdateCaretLabel(self);
}

static void M_Control(UI_PROMPT *const self)
{
    if (self->label->control != NULL) {
        self->label->control(self->label);
    }
    if (self->caret->control != NULL) {
        self->caret->control(self->caret);
    }
}

static void M_Draw(UI_PROMPT *const self)
{
    if (self->label->draw != NULL) {
        self->label->draw(self->label);
    }
    if (self->caret->draw != NULL) {
        self->caret->draw(self->caret);
    }
}

static void M_Free(UI_PROMPT *const self)
{
    self->label->free(self->label);
    self->caret->free(self->caret);
    UI_Events_Unsubscribe(self->listener1);
    UI_Events_Unsubscribe(self->listener2);
    Memory_FreePointer(&self->current_text);
    Memory_Free(self);
}

static void M_MoveCaretLeft(UI_PROMPT *const self)
{
    if (self->caret_pos > 0) {
        self->caret_pos--;
        M_UpdateCaretLabel(self);
    }
}

static void M_MoveCaretRight(UI_PROMPT *const self)
{
    if (self->caret_pos < (int32_t)strlen(self->current_text)) {
        self->caret_pos++;
        M_UpdateCaretLabel(self);
    }
}

static void M_MoveCaretStart(UI_PROMPT *const self)
{
    self->caret_pos = 0;
    M_UpdateCaretLabel(self);
}

static void M_MoveCaretEnd(UI_PROMPT *const self)
{
    self->caret_pos = strlen(self->current_text);
    M_UpdateCaretLabel(self);
}

static void M_DeleteCharBack(UI_PROMPT *const self)
{
    if (self->caret_pos <= 0) {
        return;
    }

    memmove(
        self->current_text + self->caret_pos - 1,
        self->current_text + self->caret_pos,
        strlen(self->current_text) + 1 - self->caret_pos);

    self->caret_pos--;
    M_UpdatePromptLabel(self);
    M_UpdateCaretLabel(self);
}

static void M_Confirm(UI_PROMPT *const self)
{
    if (String_IsEmpty(self->current_text)) {
        M_Cancel(self);
        return;
    }
    const EVENT event = {
        .name = "confirm",
        .sender = self,
        .data = self->current_text,
    };
    UI_Events_Fire(&event);
    M_Clear(self);
    M_UpdateCaretLabel(self);
}

static void M_Cancel(UI_PROMPT *const self)
{
    const EVENT event = {
        .name = "cancel",
        .sender = self,
        .data = self->current_text,
    };
    UI_Events_Fire(&event);
    M_Clear(self);
}

static void M_Clear(UI_PROMPT *const self)
{
    strcpy(self->current_text, "");
    self->caret_pos = 0;
    M_UpdatePromptLabel(self);
    M_UpdateCaretLabel(self);
}

static void M_HandleKeyDown(const EVENT *const event, void *const user_data)
{
    const UI_INPUT key = (UI_INPUT)(uintptr_t)event->data;
    UI_PROMPT *const self = user_data;

    if (!self->is_focused) {
        return;
    }

    // clang-format off
    switch (key) {
    case UI_KEY_LEFT:   M_MoveCaretLeft(self); break;
    case UI_KEY_RIGHT:  M_MoveCaretRight(self); break;
    case UI_KEY_HOME:   M_MoveCaretStart(self); break;
    case UI_KEY_END:    M_MoveCaretEnd(self); break;
    case UI_KEY_BACK:   M_DeleteCharBack(self); break;
    case UI_KEY_RETURN: M_Confirm(self); break;
    case UI_KEY_ESCAPE: M_Cancel(self); break;
    }
    // clang-format on
}

static void M_HandleTextEdit(const EVENT *const event, void *const user_data)
{
    const char *insert_string = event->data;
    const size_t insert_length = strlen(insert_string);
    UI_PROMPT *const self = user_data;

    if (!self->is_focused) {
        return;
    }

    if (strlen(insert_string) != 1
        || !strstr(m_ValidPromptChars, insert_string)) {
        return;
    }

    const size_t available_space =
        self->current_text_capacity - strlen(self->current_text);
    if (insert_length >= available_space) {
        self->current_text_capacity *= 2;
        self->current_text =
            Memory_Realloc(self->current_text, self->current_text_capacity);
    }

    memmove(
        self->current_text + self->caret_pos + insert_length,
        self->current_text + self->caret_pos,
        strlen(self->current_text) + 1 - self->caret_pos);
    memcpy(self->current_text + self->caret_pos, insert_string, insert_length);

    self->caret_pos += insert_length;
    M_UpdatePromptLabel(self);
    M_UpdateCaretLabel(self);
}

UI_WIDGET *UI_Prompt_Create(const int32_t width, const int32_t height)
{
    UI_PROMPT *const self = Memory_Alloc(sizeof(UI_PROMPT));
    self->vtable = (UI_WIDGET_VTABLE) {
        .control = (UI_WIDGET_CONTROL)M_Control,
        .draw = (UI_WIDGET_DRAW)M_Draw,
        .get_width = (UI_WIDGET_GET_WIDTH)M_GetWidth,
        .get_height = (UI_WIDGET_GET_HEIGHT)M_GetHeight,
        .set_position = (UI_WIDGET_SET_POSITION)M_SetPosition,
        .free = (UI_WIDGET_FREE)M_Free,
    };

    self->current_text_capacity = 1;
    self->current_text = Memory_Alloc(self->current_text_capacity);
    self->label = UI_Label_Create(self->current_text, width, height);
    self->caret = UI_Label_Create("", width, height);
    UI_Label_SetZIndex(self->label, 16);
    UI_Label_SetZIndex(self->caret, 8);
    self->is_focused = false;

    self->listener1 =
        UI_Events_Subscribe("key_down", NULL, M_HandleKeyDown, self);
    self->listener2 =
        UI_Events_Subscribe("text_edit", NULL, M_HandleTextEdit, self);

    return (UI_WIDGET *)self;
}

void UI_Prompt_SetSize(
    UI_WIDGET *const widget, const int32_t width, const int32_t height)
{
    UI_PROMPT *const self = (UI_PROMPT *)widget;
    UI_Label_SetSize(self->label, width, height);
}

void UI_Prompt_SetFocus(UI_WIDGET *const widget, const bool is_focused)
{
    UI_PROMPT *const self = (UI_PROMPT *)widget;
    self->is_focused = is_focused;
    if (is_focused) {
        Input_EnterListenMode();
        UI_Label_ChangeText(self->caret, UI_Prompt_GetPromptChar());
        UI_Label_Flash(self->caret, 1, UI_Prompt_GetCaretFlashRate());
    } else {
        Input_ExitListenMode();
        UI_Label_ChangeText(self->caret, "");
    }
}

void UI_Prompt_Clear(UI_WIDGET *const widget)
{
    UI_PROMPT *const self = (UI_PROMPT *)widget;
    M_Clear(self);
}
