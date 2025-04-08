#include "game/creature.h"

#include "game/effects.h"
#include "game/lara/common.h"
#include "game/objects/vars.h"
#include "game/random.h"
#include "game/spawn.h"

bool Creature_ShootAtLara(
    ITEM *item, int32_t distance, BITE *gun, int16_t extra_rotation,
    int16_t damage)
{
    bool is_hit;
    if (distance > CREATURE_SHOOT_RANGE) {
        is_hit = false;
    } else {
        is_hit = Random_GetControl()
            < ((CREATURE_SHOOT_RANGE - distance)
                   / (CREATURE_SHOOT_RANGE / 0x7FFF)
               - CREATURE_MISS_CHANCE);
    }

    int16_t effect_num;
    if (is_hit) {
        effect_num = Creature_Effect(item, gun, Spawn_GunShotHit);
    } else {
        effect_num = Creature_Effect(item, gun, Spawn_GunShotMiss);
    }

    if (effect_num != NO_EFFECT) {
        Effect_Get(effect_num)->rot.y += extra_rotation;
    }

    if (is_hit) {
        Lara_TakeDamage(damage, true);
    }

    return is_hit;
}

bool Creature_IsBoss(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    return Object_IsType(item->object_id, g_BossObjects);
}
