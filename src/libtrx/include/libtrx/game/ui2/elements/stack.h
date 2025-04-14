#pragma once

#include "../common.h"

#include <stdint.h>

// Stack several widgets vertically or horizontally.

typedef enum {
    UI2_STACK_VERTICAL,
    UI2_STACK_HORIZONTAL,
} UI2_STACK_ORIENTATION;

typedef enum {
    UI2_STACK_H_ALIGN_LEFT,
    UI2_STACK_H_ALIGN_CENTER,
    UI2_STACK_H_ALIGN_RIGHT,
    UI2_STACK_H_ALIGN_SPAN,
    UI2_STACK_H_ALIGN_DISTRIBUTE,
} UI2_STACK_H_ALIGN;

typedef enum {
    UI2_STACK_V_ALIGN_TOP,
    UI2_STACK_V_ALIGN_CENTER,
    UI2_STACK_V_ALIGN_BOTTOM,
    UI2_STACK_V_ALIGN_SPAN,
    UI2_STACK_V_ALIGN_DISTRIBUTE,
} UI2_STACK_V_ALIGN;

typedef struct {
    UI2_STACK_ORIENTATION orientation;
    struct {
        UI2_STACK_H_ALIGN h;
        UI2_STACK_V_ALIGN v;
    } align;
    struct {
        float h;
        float v;
    } spacing;
} UI2_STACK_SETTINGS;

void UI2_BeginStack(UI2_STACK_ORIENTATION orientation);
void UI2_BeginStackEx(UI2_STACK_SETTINGS settings);
void UI2_EndStack(void);
