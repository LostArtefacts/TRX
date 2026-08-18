#pragma once

#include <trx/game/ui/common.h>
#include <trx/game/ui/text.h>

// Basic text widget.

typedef UI_TEXT_SETTINGS UI_LABEL_SETTINGS;

void UI_Label(const char *text);
void UI_LabelFmt(const char *fmt, ...);
void UI_LabelEx(const char *text, UI_LABEL_SETTINGS settings);

// The text a node carries where it is a label, and nullptr where it is
// anything else.
const char *UI_Label_GetText(const UI_NODE *node);

void UI_Label_Measure(const char *text, float *out_w, float *out_h);
float UI_Label_MeasureW(const char *text);
void UI_Label_MeasureEx(
    const char *text, float *out_w, float *out_h, UI_LABEL_SETTINGS settings);
