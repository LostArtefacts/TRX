#pragma once

#include "config.h"
#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

#define TEXT_HEIGHT 11
#define TEXT_Y_SPACING 3
#define TEXT_OUTLINE_THICKNESS 2
#define TEXT_MAX_STRING_SIZE 100

RGBA_8888 Text_GetMenuColor(MENU_COLOR color);
void Text_Init(void);
TEXTSTRING *Text_Create(int16_t x, int16_t y, const char *content);
void Text_ChangeText(TEXTSTRING *text, const char *content);
void Text_SetPos(TEXTSTRING *text, int16_t x, int16_t y);
void Text_SetScale(TEXTSTRING *text, int32_t scale_h, int32_t scale_v);
void Text_Flash(TEXTSTRING *text, bool enable, int16_t rate);
void Text_Hide(TEXTSTRING *text, bool enable);
void Text_AddBackground(
    TEXTSTRING *text, int16_t w, int16_t h, int16_t x, int16_t y,
    TEXT_STYLE style);
void Text_RemoveBackground(TEXTSTRING *text);
void Text_AddOutline(TEXTSTRING *text, bool enable, TEXT_STYLE style);
void Text_RemoveOutline(TEXTSTRING *text);
void Text_AddProgressBar(
    TEXTSTRING *text, int16_t w, int16_t h, int16_t x, int16_t y, int32_t value,
    UI_STYLE style);
void Text_CentreH(TEXTSTRING *text, bool enable);
void Text_CentreV(TEXTSTRING *text, bool enable);
void Text_AlignRight(TEXTSTRING *text, bool enable);
void Text_AlignBottom(TEXTSTRING *text, bool enable);
void Text_SetMultiline(TEXTSTRING *text, bool enable);
int32_t Text_GetWidth(const TEXTSTRING *text);
int32_t Text_GetHeight(const TEXTSTRING *text);
void Text_Remove(TEXTSTRING *text);
void Text_Draw(void);
void Text_DrawText(TEXTSTRING *text);
