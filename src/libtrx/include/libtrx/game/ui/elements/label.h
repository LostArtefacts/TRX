#pragma once

#include "../common.h"

#include <stdint.h>

// Basic text widget.

typedef struct {
    float scale;
    int32_t z;
} UI_LABEL_SETTINGS;

void UI_Label(const char *text);
void UI_LabelEx(const char *text, UI_LABEL_SETTINGS settings);

void UI_Label_Measure(const char *text, float *out_w, float *out_h);
void UI_Label_MeasureEx(
    const char *text, float *out_w, float *out_h, UI_LABEL_SETTINGS settings);
