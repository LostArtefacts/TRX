#ifndef T1M_GAME_AI_STATUE_H
#define T1M_GAME_AI_STATUE_H

#include "global/types.h"

#include <stdint.h>

#define STATUE_EXPLODE_DIST (WALL_L * 7 / 2) // = 3584

void SetupStatue(OBJECT_INFO *obj);
void InitialiseStatue(int16_t item_num);
void StatueControl(int16_t item_num);

#endif
