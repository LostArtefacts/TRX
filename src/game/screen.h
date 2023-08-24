#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum RENDER_SCALE_REF {
    RSR_TEXT = 0,
    RSR_BAR = 1,
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
