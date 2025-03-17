#pragma once

#include "./types.h"

void Output_InitialiseTexturePages(int32_t num_pages, bool use_8bit);
void Output_InitialisePalettes(
    int32_t palette_size, const RGB_888 *palette_8, const RGB_888 *palette_16);
void Output_InitialiseObjectTextures(int32_t num_textures);
void Output_InitialiseSpriteTextures(int32_t num_textures);
void Output_InitialiseAnimatedTextures(int32_t num_ranges);

int32_t Output_GetTexturePageCount(void);
uint8_t *Output_GetTexturePage8(int32_t page_idx);
RGBA_8888 *Output_GetTexturePage32(int32_t page_idx);
int32_t Output_GetPaletteSize(void);
RGB_888 Output_GetPaletteColor8(uint16_t idx);
RGB_888 Output_GetPaletteColor16(uint16_t idx);
LIGHT_MAP *Output_GetLightMap(uint8_t idx);
SHADE_MAP *Output_GetShadeMap(uint8_t idx);
int32_t Output_GetObjectTextureCount(void);
int32_t Output_GetSpriteTextureCount(void);
OBJECT_TEXTURE *Output_GetObjectTexture(int32_t texture_idx);
SPRITE_TEXTURE *Output_GetSpriteTexture(int32_t texture_idx);
ANIMATED_TEXTURE_RANGE *Output_GetAnimatedTextureRange(int32_t range_idx);

RGBA_8888 Output_RGB2RGBA(RGB_888 color);
int16_t Output_FindColor8(RGB_888 color);
void Output_CycleAnimatedTextures(void);
