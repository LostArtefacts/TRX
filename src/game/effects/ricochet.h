#pragma once

#include "global/types.h"

#include <stdint.h>

void SetupRicochet(OBJECT_INFO *obj);
void Ricochet(GAME_VECTOR *pos);
void ControlRicochet1(int16_t fx_num);
