#pragma once

#include "./base.h"

#define UI_STACK_AUTO_SIZE (-1)

typedef enum {
    UI_STACK_LAYOUT_HORIZONTAL,
    UI_STACK_LAYOUT_VERTICAL,
} UI_STACK_LAYOUT;

typedef enum {
    UI_STACK_H_ALIGN_LEFT,
    UI_STACK_H_ALIGN_CENTER,
    UI_STACK_H_ALIGN_RIGHT,
} UI_STACK_H_ALIGN;

typedef enum {
    UI_STACK_V_ALIGN_TOP,
    UI_STACK_V_ALIGN_CENTER,
    UI_STACK_V_ALIGN_BOTTOM,
} UI_STACK_V_ALIGN;

UI_WIDGET *UI_Stack_Create(
    UI_STACK_LAYOUT layout, int32_t width, int32_t height);
void UI_Stack_SetHAlign(UI_WIDGET *self, UI_STACK_H_ALIGN align);
void UI_Stack_SetVAlign(UI_WIDGET *self, UI_STACK_V_ALIGN align);
void UI_Stack_AddChild(UI_WIDGET *self, UI_WIDGET *child);
void UI_Stack_SetSize(UI_WIDGET *widget, int32_t width, int32_t height);
void UI_Stack_DoLayout(UI_WIDGET *self);
