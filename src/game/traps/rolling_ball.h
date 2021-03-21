#ifndef T1M_GAME_TRAPS_ROLLING_BALL_H
#define T1M_GAME_TRAPS_ROLLING_BALL_H

#include "global/types.h"

#define ROLLINGBALL_DAMAGE_AIR 100

void SetupRollingBall(OBJECT_INFO *obj);
void InitialiseRollingBall(int16_t item_num);
void RollingBallControl(int16_t item_num);
void RollingBallCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);

#endif
