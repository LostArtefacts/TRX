#ifndef T1M_SPECIFIC_OUTPUT_H
#define T1M_SPECIFIC_OUTPUT_H

#include <stdbool.h>
#include <stdint.h>

#include "global/types.h"

void S_InitialisePolyList();
int32_t S_DumpScreen();
void S_ClearScreen();
void S_OutputPolyList();
void S_InitialiseScreen();
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

int32_t GetRenderScaleGLRage(int32_t unit);

int32_t GetRenderScale(int32_t unit);
int32_t GetRenderHeightDownscaled();
int32_t GetRenderWidthDownscaled();
int32_t GetRenderHeight();
int32_t GetRenderWidth();

#endif
