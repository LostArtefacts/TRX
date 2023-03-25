#pragma once

#include <stdbool.h>
#include <stdint.h>

bool Screen_SetResIdx(int32_t idx);
bool Screen_SetPrevRes(void);
bool Screen_SetNextRes(void);

int32_t Screen_GetResIdx(void);
int32_t Screen_GetResWidth(void);
int32_t Screen_GetResHeight(void);
int32_t Screen_GetResWidthDownscaledText(void);
int32_t Screen_GetResHeightDownscaledText(void);
int32_t Screen_GetResWidthDownscaledBar(void);
int32_t Screen_GetResHeightDownscaledBar(void);

int32_t Screen_GetPendingResIdx(void);
int32_t Screen_GetPendingResWidth(void);
int32_t Screen_GetPendingResHeight(void);

int32_t Screen_GetRenderScaleText(int32_t unit);
int32_t Screen_GetRenderScaleBar(int32_t unit);
int32_t Screen_GetRenderScaleBase(
    int32_t unit, int32_t base_width, int32_t base_height, double factor);
int32_t Screen_GetRenderScaleGLRage(int32_t unit);

void Screen_ApplyResolution(void);
