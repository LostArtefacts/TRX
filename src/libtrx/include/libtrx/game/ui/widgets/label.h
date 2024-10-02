#pragma once

#include "./base.h"

#define UI_LABEL_AUTO_SIZE (-1)

extern UI_WIDGET *UI_Label_Create(
    const char *text, int32_t width, int32_t height);

extern void UI_Label_ChangeText(UI_WIDGET *widget, const char *text);
extern const char *UI_Label_GetText(UI_WIDGET *widget);
extern void UI_Label_SetSize(UI_WIDGET *widget, int32_t width, int32_t height);

extern void UI_Label_AddFrame(UI_WIDGET *widget);
extern void UI_Label_RemoveFrame(UI_WIDGET *widget);
extern void UI_Label_Flash(UI_WIDGET *widget, bool enable, int32_t rate);
extern void UI_Label_SetScale(UI_WIDGET *widget, float scale);
extern void UI_Label_SetZIndex(UI_WIDGET *widget, int32_t z_index);
extern int32_t UI_Label_MeasureTextWidth(UI_WIDGET *widget);
