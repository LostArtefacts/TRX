#pragma once

#include "global/types.h"

extern PHD_VECTOR g_LsVectorView;

bool Output_Init();
void Output_Shutdown();

void Output_SetViewport(int width, int height);
void Output_SetFullscreen(bool fullscreen);
void Output_ApplyResolution();
void Output_DownloadTextures(int page_count);

RGBA8888 Output_RGB2RGBA(const RGB888 color);
void Output_SetPalette(RGB888 palette[256]);
RGB888 Output_GetPaletteColor(uint8_t idx);

int32_t Output_GetNearZ();
int32_t Output_GetFarZ();
int32_t Output_GetDrawDistMin();
int32_t Output_GetDrawDistFade();
int32_t Output_GetDrawDistMax();
void Output_SetDrawDistFade(int32_t dist);
void Output_SetDrawDistMax(int32_t dist);
void Output_SetWaterColor(const RGBF *color);

void Output_FadeReset();
void Output_FadeResetToBlack();
void Output_FadeToBlack(bool allow_immediate);
void Output_FadeToSemiBlack(bool allow_immediate);
void Output_FadeToTransparent(bool allow_immediate);
bool Output_FadeIsAnimating();
void Output_DrawBackdropScreen();
void Output_DrawOverlayScreen();

void Output_ClearScreen();
void Output_DrawEmpty();
void Output_InitialisePolyList();
void Output_CopyPictureToScreen();
int32_t Output_DumpScreen();

void Output_CalculateLight(int32_t x, int32_t y, int32_t z, int16_t room_num);
void Output_CalculateStaticLight(int16_t adder);

void Output_DrawPolygons(const int16_t *obj_ptr, int clip);
void Output_DrawPolygons_I(const int16_t *obj_ptr, int32_t clip);

void Output_DrawRoom(const int16_t *obj_ptr);
void Output_DrawShadow(int16_t size, int16_t *bptr, ITEM_INFO *item);
void Output_DrawLightningSegment(
    int32_t x1, int32_t y1, int32_t z1, int32_t x2, int32_t y2, int32_t z2,
    int32_t width);

void Output_DrawScreenFlatQuad(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA8888 color);
void Output_DrawScreenTranslucentQuad(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA8888 color);
void Output_DrawScreenGradientQuad(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA8888 tl, RGBA8888 tr,
    RGBA8888 bl, RGBA8888 br);
void Output_DrawScreenLine(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA8888 col);
void Output_DrawScreenBox(int32_t sx, int32_t sy, int32_t w, int32_t h);
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

void Output_DisplayPicture(const char *filename);

void Output_SetupBelowWater(bool underwater);
void Output_SetupAboveWater(bool underwater);
void Output_AnimateTextures(int32_t ticks);

void Output_ApplyWaterEffect(float *r, float *g, float *b);

bool Output_MakeScreenshot(const char *path);
