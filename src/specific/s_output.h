#ifndef T1M_SPECIFIC_S_OUTPUT_H
#define T1M_SPECIFIC_S_OUTPUT_H

#include "global/types.h"

// TODO: these do not belong to specific/ and are badly named

void S_NoFade();
void S_FadeInInventory(int32_t fade);
void S_FadeOutInventory(int32_t fade);

void S_CopyBufferToScreen();

void S_Wait(int32_t nframes);

RGB888 S_ColourFromPalette(int8_t idx);

void S_DrawScreenFlatQuad(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGB888 color);
void S_DrawScreenGradientQuad(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGB888 tl, RGB888 tr,
    RGB888 bl, RGB888 br);

void S_DrawScreenLine(int32_t sx, int32_t sy, int32_t w, int32_t h, RGB888 col);
void S_DrawScreenBox(int32_t sx, int32_t sy, int32_t w, int32_t h);
void S_DrawScreenFBox(int32_t sx, int32_t sy, int32_t w, int32_t h);
void S_DrawScreenSprite(
    int32_t sx, int32_t sy, int32_t z, int32_t scale_h, int32_t scale_v,
    int16_t sprnum, int16_t shade, uint16_t flags);
void S_DrawScreenSprite2d(
    int32_t sx, int32_t sy, int32_t z, int32_t scale_h, int32_t scale_v,
    int32_t sprnum, int16_t shade, uint16_t flags, int page);

void S_FadeToBlack();

void S_FinishInventory();

void S_InitialisePolyList();
int32_t S_DumpScreen();
void S_ClearScreen();
void S_OutputPolyList();
void S_CalculateLight(int32_t x, int32_t y, int32_t z, int16_t room_num);
void S_CalculateStaticLight(int16_t adder);
void S_SetupBelowWater(bool underwater);
void S_SetupAboveWater(bool underwater);
void S_AnimateTextures(int32_t ticks);
void S_DisplayPicture(const char *filename);
void S_DrawLightningSegment(
    int32_t x1, int32_t y1, int32_t z1, int32_t x2, int32_t y2, int32_t z2,
    int32_t width);
void S_PrintShadow(int16_t size, int16_t *bptr, ITEM_INFO *item);
int S_GetObjectBounds(int16_t *bptr);

void S_DrawSprite(
    int32_t x, int32_t y, int32_t z, int16_t sprnum, int16_t shade);
void S_DrawSpriteRel(
    int32_t x, int32_t y, int32_t z, int16_t sprnum, int16_t shade);
void S_DrawUISprite(
    int32_t x, int32_t y, int32_t scale, int16_t sprnum, int16_t shade);

const int16_t *S_DrawRoomSprites(const int16_t *obj_ptr, int32_t vertex_count);

#endif
