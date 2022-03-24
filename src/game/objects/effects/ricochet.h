#pragma once

#include "global/types.h"

#include <stdint.h>

void Ricochet_Setup(OBJECT_INFO *obj);
void Ricochet_Control(int16_t fx_num);

void Ricochet(GAME_VECTOR *pos);
