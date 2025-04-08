#include "game/creature.h"

#include "game/box.h"
#include "game/camera.h"
#include "game/effects.h"
#include "game/gun/gun_misc.h"
#include "game/items.h"
#include "game/lara/misc.h"
#include "game/los.h"
#include "game/lot.h"
#include "game/objects/common.h"
#include "game/objects/vars.h"
#include "game/random.h"
#include "game/room.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/game/collision.h>
#include <libtrx/game/lara/common.h>
#include <libtrx/game/math.h>
#include <libtrx/utils.h>

#define M_SHOOT_TARGETING_SPEED 300
#define M_SHOOT_HIT_CHANCE 0x2000

bool Creature_IsHostile(const ITEM *const item)
{
    return Object_IsType(item->object_id, g_EnemyObjects)
        || (Creature_AreAlliesHostile()
            && (item->object_id == O_MONK_1 || item->object_id == O_MONK_2));
}

bool Creature_IsAlly(const ITEM *const item)
{
    return Object_IsType(item->object_id, g_AllyObjects);
}

int32_t Creature_ShootAtLara(
    ITEM *const item, const AI_INFO *const info, const BITE *const gun,
    const int16_t extra_rotation, const int32_t damage)
{
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
    if (target_item == g_LaraItem) {
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

    GAME_VECTOR start = {
        .pos = {
            .x = item->pos.x,
            .y = item->pos.y - STEP_L * 3,
            .z = item->pos.z,
        },
        .room_num = item->room_num,
    };

    GAME_VECTOR target = {
        .pos = {
            .x = target_item->pos.x,
            .y = target_item->pos.y - STEP_L * 3,
            .z = target_item->pos.z,
        },
        .room_num = target_item->room_num,
    };

    const int16_t item_to_smash = LOS_CheckSmashable(&start, &target);
    if (item_to_smash != NO_ITEM) {
        Gun_SmashItem(item_to_smash, LGT_UNARMED);
    }

    return is_targetable;
}
