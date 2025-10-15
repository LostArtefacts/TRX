#include "game/creature.h"

#include "game/effects.h"
#include "game/lara.h"

#include <libtrx/game/objects/vars.h>
#include <libtrx/game/random.h>
#include <libtrx/game/spawn.h>

bool Creature_ShootAtLara(
    ITEM *const item, const AI_INFO *const info, const BITE *const gun,
    const int16_t extra_rotation, const int32_t damage)
{
    bool is_hit;
    if (info->distance > CREATURE_SHOOT_RANGE) {
        is_hit = false;
    } else {
        is_hit = Random_GetControl()
            < ((CREATURE_SHOOT_RANGE - info->distance)
                   / (CREATURE_SHOOT_RANGE / 0x7FFF)
               - CREATURE_MISS_CHANCE);
    }

    int16_t effect_num;
    if (is_hit) {
        effect_num = Creature_Effect(item, gun, Spawn_GunHit);
    } else {
        effect_num = Creature_Effect(item, gun, Spawn_GunMiss);
    }

    if (effect_num != NO_EFFECT) {
        Effect_Get(effect_num)->rot.y += extra_rotation;
    }

    if (is_hit) {
        Lara_TakeDamage(damage, true);
    }

    return is_hit;
}
