#pragma once

#include <trx/colors.h>

typedef struct {
    RGB_888 color;
    float w;
    float h;
} UI_COLOR_SWATCH_SETTINGS;

void UI_ColorSwatch(UI_COLOR_SWATCH_SETTINGS settings);
