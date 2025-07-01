#pragma once

#include "global/types.h"

#include <libtrx/game/viewport.h>

void Viewport_Init(int32_t x, int32_t y, int32_t width, int32_t height);

int32_t Viewport_GetMinX(void);
int32_t Viewport_GetMinY(void);
int32_t Viewport_GetCenterX(void);
int32_t Viewport_GetCenterY(void);
