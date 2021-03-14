#ifndef T1M_GAME_EFFECTS_TWINKLE_H
#define T1M_GAME_EFFECTS_TWINKLE_H

#include "game/types.h"
#include <stdint.h>

void SetupTwinkle(OBJECT_INFO *obj);
void Twinkle(GAME_VECTOR *pos);
void ItemSparkle(ITEM_INFO *item, int meshmask);
void ControlTwinkle(int16_t fx_num);

#endif
