#pragma once

#include <stdint.h>

void ViewPort_Init(int32_t width, int32_t height);

int32_t ViewPort_GetMinX();
int32_t ViewPort_GetMinY();
int32_t ViewPort_GetCenterX();
int32_t ViewPort_GetCenterY();
int32_t ViewPort_GetMaxX();
int32_t ViewPort_GetMaxY();
int32_t ViewPort_GetWidth();
int32_t ViewPort_GetHeight();
