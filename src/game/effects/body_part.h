#pragma once

#include "global/types.h"

#include <stdint.h>

void BodyPart_Setup(OBJECT_INFO *obj);
void BodyPart_Control(int16_t fx_num);

int32_t ExplodingDeath(int16_t item_num, int32_t mesh_bits, int16_t damage);
