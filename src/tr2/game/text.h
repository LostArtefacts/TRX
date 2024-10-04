#pragma once

#include "global/types.h"

#define TEXT_HEIGHT 15
#define TEXT_MAX_STRING_SIZE 100

void __cdecl Text_Init(void);
void Text_Shutdown(void);

TEXTSTRING *__cdecl Text_Create(
    int32_t x, int32_t y, int32_t z, const char *text);
void __cdecl Text_ChangeText(TEXTSTRING *text, const char *content);
void __cdecl Text_SetPos(TEXTSTRING *text, int16_t x, int16_t y);
void __cdecl Text_SetScale(TEXTSTRING *text, int32_t scale_h, int32_t scale_v);
void __cdecl Text_Flash(TEXTSTRING *text, int16_t enable, int16_t rate);
void __cdecl Text_AddBackground(
    TEXTSTRING *text, int16_t x_size, int16_t y_size, int16_t x_off,
    int16_t y_off, int16_t z_off, INV_COLOR color, const uint16_t *gour_ptr,
    uint16_t flags);
void __cdecl Text_RemoveBackground(TEXTSTRING *text);
void __cdecl Text_AddOutline(
    TEXTSTRING *text, int16_t enable, INV_COLOR color, const uint16_t *gour_ptr,
    uint16_t flags);
void __cdecl Text_RemoveOutline(TEXTSTRING *text);
void __cdecl Text_CentreH(TEXTSTRING *text, int16_t enable);
void __cdecl Text_CentreV(TEXTSTRING *text, int16_t enable);
void __cdecl Text_AlignRight(TEXTSTRING *text, int16_t enable);
void __cdecl Text_AlignBottom(TEXTSTRING *text, int16_t enable);
void __cdecl Text_SetMultiline(TEXTSTRING *textstring, bool enable);
int32_t __cdecl Text_Remove(TEXTSTRING *text);
int32_t __cdecl Text_GetWidth(TEXTSTRING *text);
int32_t __cdecl Text_GetHeight(const TEXTSTRING *text);

void __cdecl Text_Draw(void);
void __cdecl Text_DrawBorder(
    int32_t x, int32_t y, int32_t z, int32_t width, int32_t height);
void __cdecl Text_DrawText(TEXTSTRING *text);
uint32_t __cdecl Text_GetScaleH(uint32_t value);
uint32_t __cdecl Text_GetScaleV(uint32_t value);
