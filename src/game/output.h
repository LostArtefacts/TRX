#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

bool Output_Init(void);
void Output_Shutdown(void);

void Output_SetWindowSize(int width, int height);
void Output_ApplyRenderSettings(void);
void Output_DownloadTextures(int page_count);

RGBA_8888 Output_RGB2RGBA(const RGB_888 color);
void Output_SetPalette(RGB_888 palette[256]);
RGB_888 Output_GetPaletteColor(uint8_t idx);

int32_t Output_GetNearZ(void);
int32_t Output_GetFarZ(void);
int32_t Output_GetDrawDistMin(void);
int32_t Output_GetDrawDistFade(void);
int32_t Output_GetDrawDistMax(void);
void Output_SetDrawDistFade(int32_t dist);
void Output_SetDrawDistMax(int32_t dist);
void Output_SetWaterColor(const RGB_F *color);

void Output_FadeReset(void);
void Output_FadeResetToBlack(void);
void Output_FadeToBlack(bool allow_immediate);
void Output_FadeToSemiBlack(bool allow_immediate);
void Output_FadeToTransparent(bool allow_immediate);
bool Output_FadeIsAnimating(void);
void Output_DrawBackdropScreen(void);
void Output_DrawOverlayScreen(void);

void Output_DrawBlack(void);
void Output_DrawBackdropImage(void);
void Output_DumpScreen(void);
void Output_ClearDepthBuffer(void);

void Output_CalculateLight(int32_t x, int32_t y, int32_t z, int16_t room_num);
void Output_CalculateStaticLight(int16_t adder);
void Output_CalculateObjectLighting(ITEM_INFO *item, int16_t *frame);

void Output_DrawPolygons(const int16_t *obj_ptr, int clip);
void Output_DrawPolygons_I(const int16_t *obj_ptr, int32_t clip);

void Output_DrawRoom(const int16_t *obj_ptr);
void Output_DrawShadow(int16_t size, int16_t *bptr, ITEM_INFO *item);
void Output_DrawLightningSegment(
    int32_t x1, int32_t y1, int32_t z1, int32_t x2, int32_t y2, int32_t z2,
    int32_t width);

void Output_DrawScreenFlatQuad(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 color);
void Output_DrawScreenTranslucentQuad(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 color);
void Output_DrawScreenGradientQuad(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 tl, RGBA_8888 tr,
    RGBA_8888 bl, RGBA_8888 br);
void Output_DrawScreenLine(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 col);
void Output_DrawScreenBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 colDark,
    RGBA_8888 colLight, int32_t thickness);
void Output_DrawGradientScreenLine(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 col1,
    RGBA_8888 col2);
void Output_DrawGradientScreenBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 tl, RGBA_8888 tr,
    RGBA_8888 bl, RGBA_8888 br, int32_t thickness);
void Output_DrawCentreGradientScreenBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 edge,
    RGBA_8888 center, int32_t thickness);
void Output_DrawScreenFBox(int32_t sx, int32_t sy, int32_t w, int32_t h);

void Output_DrawSprite(
    int32_t x, int32_t y, int32_t z, int16_t sprnum, int16_t shade);
void Output_DrawScreenSprite(
    int32_t sx, int32_t sy, int32_t z, int32_t scale_h, int32_t scale_v,
    int16_t sprnum, int16_t shade, uint16_t flags);
void Output_DrawScreenSprite2D(
    int32_t sx, int32_t sy, int32_t z, int32_t scale_h, int32_t scale_v,
    int32_t sprnum, int16_t shade, uint16_t flags, int32_t page);
void Output_DrawSpriteRel(
    int32_t x, int32_t y, int32_t z, int16_t sprnum, int16_t shade);
void Output_DrawUISprite(
    int32_t x, int32_t y, int32_t scale, int16_t sprnum, int16_t shade);

void Output_LoadBackdropImage(const char *filename);

void Output_SetupBelowWater(bool underwater);
void Output_SetupAboveWater(bool underwater);
void Output_AnimateTextures(void);
void Output_AnimateFades(void);
void Output_RotateLight(int16_t pitch, int16_t yaw);

void Output_ApplyWaterEffect(float *r, float *g, float *b);

bool Output_MakeScreenshot(const char *path);

int Output_GetObjectBounds(int16_t *bptr);
