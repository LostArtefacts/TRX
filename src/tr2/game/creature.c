#include "game/creature.h"

#include "game/gun/gun_misc.h"
#include "game/los.h"

#include <libtrx/game/collision.h>
#include <libtrx/game/lara/common.h>
#include <libtrx/game/math.h>
#include <libtrx/game/random.h>
#include <libtrx/game/spawn.h>
#include <libtrx/utils.h>

#define M_SHOOT_TARGETING_SPEED 300
#define M_SHOOT_HIT_CHANCE 0x2000

static void M_CalcShootVectors(
    const ITEM *const item, const ITEM *const target_item, XYZ_32 *const start,
    XYZ_32 *const target)
{
    start->x = item->pos.x;
    start->y = item->pos.y - STEP_L * 3;
    start->z = item->pos.z;

    target->x = target_item->pos.x;
    target->y = target_item->pos.y - STEP_L * 3;
    target->z = target_item->pos.z;

    const int16_t angle = XYZ_32_GetYaw((XYZ_32) {
        .x = target->x - start->x,
        .y = target->y - start->y,
        .z = target->z - start->z,
    });

    const int32_t dist = WALL_L * 2;
    target->x += (dist * Math_Sin(angle)) >> W2V_SHIFT;
    target->z += (dist * Math_Cos(angle)) >> W2V_SHIFT;
}

bool Creature_ShootAtLara(
    ITEM *const item, const AI_INFO *const info, const BITE *const gun,
    const int16_t extra_rotation, const int32_t damage)
{
    const ITEM *const lara_item = Lara_GetItem();
    const CREATURE *const creature = item->data;
    ITEM *const target_item = creature->enemy;

    bool is_targetable;
    bool is_hit;
    if (info->distance > CREATURE_SHOOT_RANGE
        || !Creature_CanTargetEnemy(item, info)) {
        is_targetable = false;
        is_hit = false;
    } else {
        int32_t distance =
            (((target_item->speed * Math_Sin(info->enemy_facing)) >> W2V_SHIFT)
             * CREATURE_SHOOT_RANGE)
            / M_SHOOT_TARGETING_SPEED;
        distance = info->distance + SQUARE(distance);
        if (distance > CREATURE_SHOOT_RANGE) {
            is_hit = false;
        } else {
            const int32_t chance = M_SHOOT_HIT_CHANCE
                + (CREATURE_SHOOT_RANGE - info->distance)
                    / (CREATURE_SHOOT_RANGE / 0x5000);
            is_hit = Random_GetControl() < chance;
        }
        is_targetable = true;
    }

    int16_t effect_num = NO_EFFECT;
    if (target_item == lara_item) {
        if (is_hit) {
            effect_num = Creature_Effect(item, gun, Spawn_GunHit);
            Item_TakeDamage(target_item, damage, true);
        } else if (is_targetable) {
            effect_num = Creature_Effect(item, gun, Spawn_GunMiss);
        }
    } else {
        effect_num = Creature_Effect(item, gun, Spawn_GunShot);
        if (is_hit) {
            Item_TakeDamage(target_item, damage / 10, true);
        }
    }
    if (effect_num != NO_EFFECT) {
        Effect_Get(effect_num)->rot.y += extra_rotation;
    }

    XYZ_32 start, target;
    M_CalcShootVectors(item, target_item, &start, &target);
    Gun_SmashItems(start, target, nullptr);

    return is_targetable;
}
