#pragma once

#include <stdbool.h>
#include <stdint.h>

struct UI_WIDGET;

typedef void (*UI_WIDGET_CONTROL)(struct UI_WIDGET *self);
typedef void (*UI_WIDGET_DRAW)(struct UI_WIDGET *self);
typedef int32_t (*UI_WIDGET_GET_WIDTH)(const struct UI_WIDGET *self);
typedef int32_t (*UI_WIDGET_GET_HEIGHT)(const struct UI_WIDGET *self);
typedef void (*UI_WIDGET_SET_POSITION)(
    struct UI_WIDGET *self, int32_t x, int32_t y);
typedef void (*UI_WIDGET_FREE)(struct UI_WIDGET *self);

typedef struct UI_WIDGET {
    UI_WIDGET_CONTROL control;
    UI_WIDGET_DRAW draw;
    UI_WIDGET_GET_WIDTH get_width;
    UI_WIDGET_GET_HEIGHT get_height;
    UI_WIDGET_SET_POSITION set_position;
    UI_WIDGET_FREE free;
} UI_WIDGET;

typedef UI_WIDGET UI_WIDGET_VTABLE;
