#pragma once

#include "global/types.h"

#include <stdint.h>

void SetupTwinkle(OBJECT_INFO *obj);
void Twinkle(GAME_VECTOR *pos);
void ItemSparkle(ITEM_INFO *item, int meshmask);
void ControlTwinkle(int16_t fx_num);
