#pragma once

#include <trx/game/creature/types.h>
#include <trx/game/items/types.h>

bool FX_GunFlash_Spawn(const ITEM *owner_item, const CREATURE_GUN *gun);
void FX_GunFlash_Update(void);
void FX_GunFlash_Draw(void);
