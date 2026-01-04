#pragma once

#include <trx/game/items/types.h>

#include <stdbool.h>

void FootprintFX_Init(void);

void FootprintFX_Add(const ITEM *lara_item, bool is_left_foot);

void FootprintFX_Update(void);
void FootprintFX_Draw(void);
