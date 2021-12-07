#pragma once

#include "global/types.h"

#include <stdint.h>

int32_t TestCollision(ITEM_INFO *item, ITEM_INFO *lara_item);
int32_t GetSpheres(ITEM_INFO *item, SPHERE *slist, int32_t world_space);
void GetJointAbsPosition(ITEM_INFO *item, PHD_VECTOR *vec, int32_t joint);
