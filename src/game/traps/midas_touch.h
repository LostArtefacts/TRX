#pragma once

#include "global/types.h"

#include <stdint.h>

extern int16_t g_MidasBounds[12];

void SetupMidasTouch(OBJECT_INFO *obj);
void MidasCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
