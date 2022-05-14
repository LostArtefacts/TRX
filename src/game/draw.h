#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

void DrawEffect(int16_t fxnum);
void DrawSpriteItem(ITEM_INFO *item);
void DrawDummyItem(ITEM_INFO *item);
void DrawPickupItem(ITEM_INFO *item);
void DrawAnimatingItem(ITEM_INFO *item);
void DrawUnclippedItem(ITEM_INFO *item);
