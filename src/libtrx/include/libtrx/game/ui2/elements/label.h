#pragma once

#include "../common.h"

#include <stdint.h>

// Basic text widget.

typedef struct {
    float scale;
    int32_t z;
} UI2_LABEL_SETTINGS;

void UI2_Label(const char *text);
void UI2_LabelEx(const char *text, UI2_LABEL_SETTINGS settings);

void UI2_Label_Measure(const char *text, float *out_w, float *out_h);
void UI2_Label_MeasureEx(
    const char *text, float *out_w, float *out_h, UI2_LABEL_SETTINGS settings);
