#pragma once

#include <stdint.h>

typedef enum {
    RSR_TEXT,
    RSR_BAR,
    RSR_GENERIC,
} RENDER_SCALE_REF;

void Screen_Init(void);

int32_t Screen_GetResWidth(void);
int32_t Screen_GetResHeight(void);
int32_t Screen_GetResWidthDownscaled(RENDER_SCALE_REF ref);
int32_t Screen_GetResHeightDownscaled(RENDER_SCALE_REF ref);
int32_t Screen_GetRenderScale(int32_t unit, RENDER_SCALE_REF ref);
int32_t Screen_GetRenderScaleGLRage(int32_t unit);

bool Screen_CanSetPrevRes(void);
bool Screen_CanSetNextRes(void);
bool Screen_SetPrevRes(void);
bool Screen_SetNextRes(void);
