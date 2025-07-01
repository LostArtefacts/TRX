#pragma once

#include "../game_flow.h"

extern void Lara_InitialiseMeshes(const GF_LEVEL *level);

void Lara_BaddieCollision(ITEM *lara_item, COLL_INFO *coll);
void Lara_DismountVehicle(void);
