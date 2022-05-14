#pragma once

#include "global/types.h"

#include <stdint.h>

void ViewPort_Init(int32_t width, int32_t height);

int32_t ViewPort_GetMinX(void);
int32_t ViewPort_GetMinY(void);
int32_t ViewPort_GetCenterX(void);
int32_t ViewPort_GetCenterY(void);
int32_t ViewPort_GetMaxX(void);
int32_t ViewPort_GetMaxY(void);
int32_t ViewPort_GetWidth(void);
int32_t ViewPort_GetHeight(void);

void ViewPort_AlterFOV(PHD_ANGLE fov);
