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

#define FLOAT_SPEED 32
#define TARGET_TOLERANCE 0x400000

#define CREATURE_SHOOT_TARGETING_SPEED 300
#define CREATURE_SHOOT_HIT_CHANCE 0x2000

void Creature_Float(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    item->hit_points = DONT_TARGET;
    item->rot.x = 0;

    const int32_t wh = Room_GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_num);
    if (item->pos.y > wh) {
        item->pos.y -= FLOAT_SPEED;
    } else if (item->pos.y < wh) {
        item->pos.y = wh;
    }

    Item_Animate(item);

    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    item->floor = Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
    if (room_num != item->room_num) {
        Item_NewRoom(item_num, room_num);
    }
}

void Creature_Underwater(ITEM *const item, const int32_t depth)
{
    const int32_t wh = Room_GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_num);
    if (item->pos.y >= wh + depth) {
        return;
    }

    item->pos.y = wh + depth;
    if (item->rot.x > 2 * DEG_1) {
        item->rot.x -= 2 * DEG_1;
    } else {
        CLAMPG(item->rot.x, 0);
    }
}

int32_t Creature_Vault(
    const int16_t item_num, const int16_t angle, int32_t vault,
    const int32_t shift)
{
    ITEM *const item = Item_Get(item_num);
    const int16_t room_num = item->room_num;
    const XYZ_32 old = item->pos;

    Creature_Animate(item_num, angle, 0);

    if (item->floor > old.y + STEP_L * 7 / 2) {
        vault = -4;
    } else if (item->pos.y > old.y - STEP_L * 3 / 2) {
        return 0;
    } else if (item->pos.y > old.y - STEP_L * 5 / 2) {
        vault = 2;
    } else if (item->pos.y > old.y - STEP_L * 7 / 2) {
        vault = 3;
    } else {
        vault = 4;
    }

    const int32_t old_x_sector = old.x >> WALL_SHIFT;
    const int32_t old_z_sector = old.z >> WALL_SHIFT;
    const int32_t x_sector = item->pos.x >> WALL_SHIFT;
    const int32_t z_sector = item->pos.z >> WALL_SHIFT;
    if (old_z_sector == z_sector) {
        if (old_x_sector == x_sector) {
            return 0;
        }

        if (old_x_sector >= x_sector) {
            item->rot.y = -DEG_90;
            item->pos.x = (old_x_sector << WALL_SHIFT) + shift;
        } else {
            item->rot.y = DEG_90;
            item->pos.x = (x_sector << WALL_SHIFT) - shift;
        }
    } else if (old_x_sector == x_sector) {
        if (old_z_sector >= z_sector) {
            item->rot.y = -DEG_180;
            item->pos.z = (old_z_sector << WALL_SHIFT) + shift;
        } else {
            item->rot.y = 0;
            item->pos.z = (z_sector << WALL_SHIFT) - shift;
        }
    }

    item->floor = old.y;
    item->pos.y = old.y;

    if (room_num != item->room_num) {
        Item_NewRoom(item_num, room_num);
    }
    return vault;
}

void Creature_Kill(
    ITEM *const item, const int32_t kill_anim, const int32_t kill_state,
    const int32_t lara_kill_state)
{
    Item_SwitchToAnim(item, kill_anim, 0);
    item->current_anim_state = kill_state;

    Item_SwitchToObjAnim(g_LaraItem, LA_EXTRA_BREATH, 0, O_LARA_EXTRA);
    g_LaraItem->current_anim_state = LA_EXTRA_BREATH;
    g_LaraItem->goal_anim_state = lara_kill_state;
    g_LaraItem->pos = item->pos;
    g_LaraItem->rot = item->rot;
    g_LaraItem->fall_speed = 0;
    g_LaraItem->gravity = 0;
    g_LaraItem->speed = 0;

    int16_t room_num = item->room_num;
    if (room_num != g_LaraItem->room_num) {
        Item_NewRoom(g_Lara.item_num, room_num);
    }

    Item_Animate(g_LaraItem);

    g_LaraItem->goal_anim_state = lara_kill_state;
    g_LaraItem->current_anim_state = lara_kill_state;
    g_Lara.extra_anim = 1;
    g_Lara.gun_status = LGS_HANDS_BUSY;
    g_Lara.gun_type = LGT_UNARMED;
    g_Lara.hit_direction = -1;
    g_Lara.air = -1;

    g_Camera.pos.room_num = g_LaraItem->room_num;
}

void Creature_GetBaddieTarget(const int16_t item_num, const bool goody)
{
    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->data;

    ITEM *best_item = nullptr;
    int32_t best_distance = INT32_MAX;
    for (int32_t i = 0; i < NUM_SLOTS; i++) {
        const int16_t target_item_num = g_BaddieSlots[i].item_num;
        if (target_item_num == NO_ITEM || target_item_num == item_num) {
            continue;
        }

        ITEM *const target = Item_Get(target_item_num);
        const GAME_OBJECT_ID obj_id = target->object_id;
        if (goody && obj_id != O_BANDIT_1 && obj_id != O_BANDIT_2) {
            continue;
        } else if (!goody && obj_id != O_MONK_1 && obj_id != O_MONK_2) {
            continue;
        }

        const int32_t dx = (target->pos.x - item->pos.x) >> 6;
        const int32_t dy = (target->pos.y - item->pos.y) >> 6;
        const int32_t dz = (target->pos.z - item->pos.z) >> 6;
        const int32_t distance = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
        if (distance < best_distance) {
            best_item = target;
            best_distance = distance;
        }
    }

    if (best_item == nullptr) {
        if (!goody || g_IsMonkAngry) {
            creature->enemy = g_LaraItem;
        } else {
            creature->enemy = nullptr;
        }
        return;
    }

    if (!goody || g_IsMonkAngry) {
        const int32_t dx = (g_LaraItem->pos.x - item->pos.x) >> 6;
        const int32_t dy = (g_LaraItem->pos.y - item->pos.y) >> 6;
        const int32_t dz = (g_LaraItem->pos.z - item->pos.z) >> 6;
        const int32_t distance = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
        if (distance < best_distance) {
            best_item = g_LaraItem;
            best_distance = distance;
        }
    }

    const ITEM *const target = creature->enemy;
    if (target == nullptr || target->status != IS_ACTIVE) {
        creature->enemy = best_item;
    } else {
        const int32_t dx = (target->pos.x - item->pos.x) >> 6;
        const int32_t dy = (target->pos.y - item->pos.y) >> 6;
        const int32_t dz = (target->pos.z - item->pos.z) >> 6;
        const int32_t distance = SQUARE(dz) + SQUARE(dy) + SQUARE(dx);
        if (distance < best_distance + TARGET_TOLERANCE) {
            creature->enemy = best_item;
        }
    }
}

bool Creature_IsHostile(const ITEM *const item)
{
    return Object_IsType(item->object_id, g_EnemyObjects)
        || (g_IsMonkAngry
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
            / CREATURE_SHOOT_TARGETING_SPEED;
        distance = info->distance + SQUARE(distance);
        if (distance > CREATURE_SHOOT_RANGE) {
            is_hit = false;
        } else {
            const int32_t chance = CREATURE_SHOOT_HIT_CHANCE
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
