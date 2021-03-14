#ifndef T1M_GAME_TRAPS_MIDAS_TOUCH_H
#define T1M_GAME_TRAPS_MIDAS_TOUCH_H

#include "game/types.h"

#include <stdint.h>

extern int16_t MidasBounds[12];

void SetupMidasTouch(OBJECT_INFO *obj);
void MidasCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);

#endif
