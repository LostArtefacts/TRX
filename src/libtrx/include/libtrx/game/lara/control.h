#pragma once

#include "../collision.h"
#include "../game_flow.h"

extern void Lara_InitialiseMeshes(const GF_LEVEL *level);
extern void Lara_HandleUnderwater(ITEM *item, COLL_INFO *coll);

void Lara_Control(void);
void Lara_BaddieCollision(ITEM *lara_item, COLL_INFO *coll);
void Lara_WaterCurrent(COLL_INFO *coll);
void Lara_DismountVehicle(void);
