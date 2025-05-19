#pragma once

#include "../../colors.h"
#include "../game_flow/types.h"

extern RGB_888 Level_GetWaterColor(void);
extern float Level_GetFogStart(void);
extern float Level_GetFogEnd(void);
const GF_AMBIENT_DATA *Level_GetAmbientData(void);
