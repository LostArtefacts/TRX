#pragma once

#include "global/types.h"

#define ROLLINGBALL_DAMAGE_AIR 100

void SetupRollingBall(OBJECT_INFO *obj);
void InitialiseRollingBall(int16_t item_num);
void RollingBallControl(int16_t item_num);
void RollingBallCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
