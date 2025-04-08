#include "game/creature.h"

#include "game/box.h"
#include "game/carrier.h"
#include "game/effects.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/los.h"
#include "game/lot.h"
#include "game/objects/common.h"
#include "game/objects/vars.h"
#include "game/random.h"
#include "game/room.h"
#include "game/spawn.h"
#include "global/vars.h"

#include <libtrx/game/collision.h>
#include <libtrx/game/math.h>
#include <libtrx/log.h>

bool Creature_ShootAtLara(
    ITEM *item, int32_t distance, BITE *gun, int16_t extra_rotation,
    int16_t damage)
{
    bool hit;
    if (distance > CREATURE_SHOOT_RANGE) {
        hit = false;
    } else {
        hit = Random_GetControl()
            < ((CREATURE_SHOOT_RANGE - distance)
                   / (CREATURE_SHOOT_RANGE / 0x7FFF)
               - CREATURE_MISS_CHANCE);
    }

    int16_t effect_num;
    if (hit) {
        effect_num = Creature_Effect(item, gun, Spawn_GunShotHit);
    } else {
        effect_num = Creature_Effect(item, gun, Spawn_GunShotMiss);
    }

    if (effect_num != NO_EFFECT) {
        Effect_Get(effect_num)->rot.y += extra_rotation;
    }

    if (hit) {
        Lara_TakeDamage(damage, true);
    }

    return hit;
}

bool Creature_IsBoss(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    return Object_IsType(item->object_id, g_BossObjects);
}
