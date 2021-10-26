#ifndef T1M_GAME_OBJECTS_EARTH_QUAKE_H
#define T1M_GAME_OBJECTS_EARTH_QUAKE_H

#include "global/types.h"

#include <stdint.h>

void SetupEarthquake(OBJECT_INFO *obj);
void EarthQuake(ITEM_INFO *item);
void EarthQuakeControl(int16_t item_num);

#endif
