#pragma once

#include "global/types.h"

#define BGND_MAX_TEXTURE_PAGES 5

bool __cdecl BGND_Init(void);
void __cdecl BGND_Free(void);

void __cdecl BGND_Make640x480(uint8_t *bitmap, RGB_888 *palette);
int32_t __cdecl BGND_AddTexture(
    int32_t tile_idx, uint8_t *bitmap, int32_t pal_index,
    const RGB_888 *bmp_pal);
void __cdecl BGND_GetPageHandles(void);
void __cdecl BGND_DrawInGameBlack(void);
void __cdecl DrawQuad(
    float sx, float sy, float width, float height, D3DCOLOR color);
void __cdecl BGND_DrawInGameBackground(void);
void __cdecl DrawTextureTile(
    int32_t sx, int32_t sy, int32_t width, int32_t height,
    HWR_TEXTURE_HANDLE tex_source, int32_t tu, int32_t tv, int32_t t_width,
    int32_t t_height, D3DCOLOR color0, D3DCOLOR color1, D3DCOLOR color2,
    D3DCOLOR color3);
D3DCOLOR __cdecl BGND_CenterLighting(
    int32_t x, int32_t y, int32_t width, int32_t height);
