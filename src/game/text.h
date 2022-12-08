#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

#define TEXT_HEIGHT 11
#define TEXT_OUTLINE_THICKNESS 2

RGBA8888 Text_GetMenuColor(MENU_COLOR color);
void Text_Init(void);
TEXTSTRING *Text_Create(int16_t x, int16_t y, const char *string);
void Text_ChangeText(TEXTSTRING *textstring, const char *string);
void Text_SetPos(TEXTSTRING *textstring, int16_t x, int16_t y);
void Text_SetScale(TEXTSTRING *textstring, int32_t scale_h, int32_t scale_v);
void Text_Flash(TEXTSTRING *textstring, bool enable, int16_t rate);
void Text_Hide(TEXTSTRING *textstring, bool enable);
void Text_AddBackground(
    TEXTSTRING *textstring, int16_t w, int16_t h, int16_t x, int16_t y,
    TEXT_STYLE style);
void Text_RemoveBackground(TEXTSTRING *textstring);
void Text_AddOutline(TEXTSTRING *textstring, bool enable, TEXT_STYLE style);
void Text_RemoveOutline(TEXTSTRING *textstring);
void Text_AddProgressBar(
    TEXTSTRING *textstring, int16_t w, int16_t h, int16_t x, int16_t y,
    int32_t value, UI_STYLE style);
void Text_CentreH(TEXTSTRING *textstring, bool enable);
void Text_CentreV(TEXTSTRING *textstring, bool enable);
void Text_AlignRight(TEXTSTRING *textstring, bool enable);
void Text_AlignBottom(TEXTSTRING *textstring, bool enable);
int32_t Text_GetWidth(TEXTSTRING *textstring);
void Text_Remove(TEXTSTRING *textstring);
void Text_RemoveAll(void);
void Text_Draw(void);
