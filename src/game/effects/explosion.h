#ifndef T1M_GAME_EFFECTS_EXPLOSION_H
#define T1M_GAME_EFFECTS_EXPLOSION_H

#include "global/types.h"

#include <stdint.h>

void SetupExplosion(OBJECT_INFO *obj);
void ControlExplosion1(int16_t fx_num);
void Explosion(ITEM_INFO *item);

#endif
