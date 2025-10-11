#pragma once

#include "../../colors.h"
#include "../game_flow/types.h"

RGB_888 Level_GetWaterColor(void);
RGBA_8888 Level_GetFogColor(void);
float Level_GetFogStart(void);
float Level_GetFogEnd(void);
const GF_AMBIENT_DATA *Level_GetAmbientData(void);
bool Level_HasColdWater(void);
