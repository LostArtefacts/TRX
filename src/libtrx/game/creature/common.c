#include "config.h"
#include "game/camera/vars.h"
#include "game/carrier.h"
#include "game/creature.h"
#include "game/game_flow.h"
#include "game/lara/common.h"
#include "game/los.h"
#include "game/objects/vars.h"
#include "game/pathing.h"
#include "game/random.h"
#include "game/rooms.h"
#include "log.h"
#include "utils.h"

#define M_FLOAT_SPEED 32
#define M_MAX_DISTANCE (WALL_L * 30)
#define M_ATTACK_RANGE SQUARE(WALL_L * 3) // = 0x900000 = 9437184
#define M_ESCAPE_CHANCE 2048
#define M_RECOVER_CHANCE 256
#define M_TARGET_TOLERANCE 0x400000
#define M_MAX_TILT (3 * DEG_1) // = 546
#define M_MAX_HEAD_CHANGE (5 * DEG_1) // = 910
#if TR_VERSION == 1
    #define M_HEAD_ARC FRONT_ARC
#elif TR_VERSION >= 2
    #define M_HEAD_ARC 0x3000 // = 12288
#endif
#define M_MAX_X_ROT (20 * DEG_1) // = 3640

static bool m_AlliesHostile = false;

static bool M_TestSwitchOrKill(
    const int16_t item_num, const OBJECT_ID target_id)
{
    if (Object_Get(target_id)->loaded) {
        return true;
    }

    LOG_WARNING(
        "Object %d is not loaded; item %d cannot be converted.", target_id,
        item_num);
    Item_Kill(item_num);
    return false;
}

static void M_GetBaddieTarget(const int16_t item_num, const bool goody)
{
    ITEM *const lara_item = Lara_GetItem();
    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->data;

    ITEM *best_item = nullptr;
    int32_t best_distance = INT32_MAX;
    for (int32_t i = 0; i < LOT_SLOT_COUNT; i++) {
        const int16_t target_item_num = LOT_GetBaddieSlot(i)->item_num;
        if (target_item_num == NO_ITEM || target_item_num == item_num) {
            continue;
        }

        ITEM *const target = Item_Get(target_item_num);
        const OBJECT_ID obj_id = target->object_id;
#if TR_VERSION == 2
        if (goody && !Creature_IsAllyTargetingEnemy(target)) {
            continue;
        } else if (!goody && !Creature_IsAlly(target)) {
            continue;
        }
#endif

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
        if (!goody || Creature_AreAlliesHostile()) {
            creature->enemy = lara_item;
        } else {
            creature->enemy = nullptr;
        }
        return;
    }

    if (!goody || Creature_AreAlliesHostile()) {
        const int32_t dx = (lara_item->pos.x - item->pos.x) >> 6;
        const int32_t dy = (lara_item->pos.y - item->pos.y) >> 6;
        const int32_t dz = (lara_item->pos.z - item->pos.z) >> 6;
        const int32_t distance = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
        if (distance < best_distance) {
            best_item = lara_item;
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
        if (distance < best_distance + M_TARGET_TOLERANCE) {
            creature->enemy = best_item;
        }
    }
}

static ITEM *M_ChooseEnemy(const ITEM *const item)
{
    CREATURE *const creature = item->data;
    if (Creature_IsAlly(item)) {
        M_GetBaddieTarget(creature->item_num, true);
    } else if (Creature_IsAllyTargetingEnemy(item)) {
        M_GetBaddieTarget(creature->item_num, false);
    } else {
        creature->enemy = Lara_GetItem();
    }

    if (creature->enemy != nullptr) {
        return creature->enemy;
    }
    return Lara_GetItem();
}

static bool M_SwitchToWater(
    const int16_t item_num, const int32_t wh, const HYBRID_INFO *const info)
{
    if (wh == NO_HEIGHT) {
        return false;
    }

    ITEM *const item = Item_Get(item_num);

    if (item->hit_points <= 0
        && item->current_anim_state == info->land.death_state) {
        // Dead land creatures should remain in their pose permanently.
        return false;
    }

    // Switch to the water creature, but only switch animations if the creature
    // is alive to avoid savegame reload issues.
    if (!M_TestSwitchOrKill(item_num, info->water.id)) {
        return false;
    }

    item->object_id = info->water.id;
    if (item->hit_points > 0) {
        Item_SwitchToAnim(item, info->water.active_anim, 0);
        item->current_anim_state = Item_GetAnim(item)->current_anim_state;
        item->goal_anim_state = item->current_anim_state;
    }
    item->pos.y = wh;

    return true;
}

static bool M_SwitchToLand(
    const int16_t item_num, const int32_t wh, const HYBRID_INFO *const info)
{
    if (wh != NO_HEIGHT) {
        return false;
    }

    if (!M_TestSwitchOrKill(item_num, info->land.id)) {
        return false;
    }

    ITEM *const item = Item_Get(item_num);

    // Switch to the land creature regardless of death state.
    item->object_id = info->land.id;
    item->rot.x = 0;

    if (item->hit_points > 0) {
        Item_SwitchToAnim(item, info->land.active_anim, 0);
        item->current_anim_state = Item_GetAnim(item)->current_anim_state;
        item->goal_anim_state = item->current_anim_state;

    } else {
        Item_SwitchToAnim(item, info->land.death_anim, -1);
        item->current_anim_state = info->land.death_state;
        item->goal_anim_state = item->current_anim_state;

        int16_t room_num = item->room_num;
        const SECTOR *const sector =
            Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
        item->floor =
            Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
        item->pos.y = item->floor;
        Item_UpdateRoom(item_num, room_num);
    }

    return true;
}

void Creature_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    // TODO: remove GF check once demo config reset is run before level load
    if (g_Config.gameplay.enable_enemy_rotation
        || GF_GetCurrentLevel()->type == GFL_DEMO) {
        item->rot.y += (Random_GetControl() - DEG_90) >> 1;
    }
    item->collidable = true;
    item->data = nullptr;
}

bool Creature_Activate(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->status != IS_INVISIBLE) {
        return true;
    }

    if (!LOT_EnableBaddieAI(item_num, false)) {
        return false;
    }

    item->status = IS_ACTIVE;
    return true;
}

void Creature_AIInfo(ITEM *const item, AI_INFO *const info)
{
    CREATURE *const creature = item->data;
    if (creature == nullptr) {
        return;
    }

    ITEM *const enemy = M_ChooseEnemy(item);
    const int16_t *const zone = Box_GetLotZone(&creature->lot);

    {
        const ROOM *const room = Room_Get(item->room_num);
        item->box_num =
            Room_GetWorldSector(room, item->pos.x, item->pos.z)->box;
        info->zone_num = zone[item->box_num];
    }

    {
        const ROOM *const room = Room_Get(enemy->room_num);
        enemy->box_num =
            Room_GetWorldSector(room, enemy->pos.x, enemy->pos.z)->box;
        info->enemy_zone_num = zone[enemy->box_num];
    }

    const BOX_INFO *const enemy_box = Box_GetBox(enemy->box_num);
    if (((enemy_box->overlap_index & creature->lot.setup.block_mask) != 0)
        || (creature->lot.node[item->box_num].search_num
            == (creature->lot.search_num | BOX_BLOCKED_SEARCH))) {
        info->enemy_zone_num |= BOX_BLOCKED;
    }

    const OBJECT *const obj = Object_Get(item->object_id);
    const int32_t z = enemy->pos.z
        - ((obj->pivot_length * Math_Cos(item->rot.y)) >> W2V_SHIFT)
        - item->pos.z;
    const int32_t x = enemy->pos.x
        - ((obj->pivot_length * Math_Sin(item->rot.y)) >> W2V_SHIFT)
        - item->pos.x;
    const int16_t angle = Math_Atan(z, x);

    if (creature->enemy == nullptr) {
        info->distance = 0x7FFFFFFF;
    } else if (ABS(x) > M_MAX_DISTANCE || ABS(z) > M_MAX_DISTANCE) {
        info->distance = SQUARE(M_MAX_DISTANCE);
    } else {
        info->distance = SQUARE(x) + SQUARE(z);
    }

    info->angle = angle - item->rot.y;
    info->enemy_facing = angle - enemy->rot.y + DEG_180;
    info->ahead = info->angle > -FRONT_ARC && info->angle < FRONT_ARC;
    info->bite = info->ahead && ABS(enemy->pos.y - item->pos.y) <= STEP_L
        && (TR_VERSION == 1 || enemy->hit_points > 0);
}

bool Creature_EnsureHabitat(
    const int16_t item_num, int32_t *const wh, const HYBRID_INFO *const info)
{
    // Test the environment for a hybrid creature. Record the water height and
    // return whether or not a type conversion has taken place.
    const ITEM *const item = Item_Get(item_num);
    const int32_t water_height = Room_GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_num);
    if (wh != nullptr) {
        *wh = water_height;
    }
    if (item->status == IS_INACTIVE) {
        return false;
    }

    return item->object_id == info->land.id
        ? M_SwitchToWater(item_num, water_height, info)
        : M_SwitchToLand(item_num, water_height, info);
}

void Creature_Mood(
    const ITEM *const item, const AI_INFO *const info, const bool violent)
{
    CREATURE *const creature = item->data;
    if (creature == nullptr) {
        return;
    }

    LOT_INFO *const lot = &creature->lot;
    const ITEM *enemy = TR_VERSION >= 2 ? creature->enemy : Lara_GetItem();
    if (lot->node[item->box_num].search_num
        == (lot->search_num | BOX_BLOCKED_SEARCH)) {
        lot->required_box = NO_BOX;
    }

    if (creature->mood != MOOD_ATTACK && lot->required_box != NO_BOX
        && !Box_ValidBox(item, info->zone_num, lot->target_box)) {
        if (info->zone_num == info->enemy_zone_num) {
            creature->mood = MOOD_BORED;
        }
        lot->required_box = NO_BOX;
    }

    const MOOD_TYPE mood = creature->mood;
    if (enemy == nullptr) {
        creature->mood = MOOD_BORED;
        enemy = Lara_GetItem();
    } else if (enemy->hit_points <= 0) {
        creature->mood = MOOD_BORED;
    } else if (violent) {
        switch (mood) {
        case MOOD_BORED:
        case MOOD_STALK:
            if (info->zone_num == info->enemy_zone_num) {
                creature->mood = MOOD_ATTACK;
            } else if (item->hit_status) {
                creature->mood = MOOD_ESCAPE;
            }
            break;

        case MOOD_ATTACK:
            if (info->zone_num != info->enemy_zone_num) {
                creature->mood = MOOD_BORED;
            }
            break;

        case MOOD_ESCAPE:
            if (info->zone_num == info->enemy_zone_num) {
                creature->mood = MOOD_ATTACK;
            }
            break;
        }
    } else {
        switch (mood) {
        case MOOD_BORED:
        case MOOD_STALK:
            if (item->hit_status
                && (Random_GetControl() < M_ESCAPE_CHANCE
                    || info->zone_num != info->enemy_zone_num)) {
                creature->mood = MOOD_ESCAPE;
            } else if (info->zone_num == info->enemy_zone_num) {
                if (info->distance < M_ATTACK_RANGE
                    || (creature->mood == MOOD_STALK
                        && lot->required_box == NO_BOX)) {
                    creature->mood = MOOD_ATTACK;
                } else {
                    creature->mood = MOOD_STALK;
                }
            }
            break;

        case MOOD_ATTACK:
            if (item->hit_status
                && (Random_GetControl() < M_ESCAPE_CHANCE
                    || info->zone_num != info->enemy_zone_num)) {
                creature->mood = MOOD_ESCAPE;
            } else if (info->zone_num != info->enemy_zone_num) {
                creature->mood = MOOD_BORED;
            }
            break;

        case MOOD_ESCAPE:
            if (info->zone_num == info->enemy_zone_num
                && Random_GetControl() < M_RECOVER_CHANCE) {
                creature->mood = MOOD_STALK;
            }
            break;
        }
    }

    if (mood != creature->mood) {
        if (mood == MOOD_ATTACK) {
            Box_TargetBox(lot, lot->target_box);
        }
        lot->required_box = NO_BOX;
    }

    switch (creature->mood) {
    case MOOD_BORED: {
        const int16_t box_num =
            lot->node[lot->zone_count * Random_GetControl() / 0x7FFF].box_num;
        if (Box_ValidBox(item, info->zone_num, box_num)) {
            if (Box_StalkBox(item, enemy, box_num)
                && (TR_VERSION == 1
                    || (creature->enemy != nullptr && enemy->hit_points > 0))) {
                Box_TargetBox(lot, box_num);
                creature->mood = MOOD_STALK;
            } else if (lot->required_box == NO_BOX) {
                Box_TargetBox(lot, box_num);
            }
        }
        break;
    }

    case MOOD_ATTACK:
        if (TR_VERSION >= 2
            || Random_GetControl() < Object_Get(item->object_id)->smartness) {
            lot->target = enemy->pos;
            lot->required_box = enemy->box_num;
            if (lot->setup.fly != 0
                && Lara_GetLaraInfo()->water_status == LWS_ABOVE_WATER) {
                lot->target.y += Item_GetBestFrame(enemy)->bounds.min.y;
            }
        }
        break;

    case MOOD_ESCAPE: {
        const int16_t box_num =
            lot->node[lot->zone_count * Random_GetControl() / 0x7FFF].box_num;
        if (Box_ValidBox(item, info->zone_num, box_num)
            && lot->required_box == NO_BOX) {
            if (Box_EscapeBox(item, enemy, box_num)) {
                Box_TargetBox(lot, box_num);
            } else if (
                info->zone_num == info->enemy_zone_num
                && Box_StalkBox(item, enemy, box_num)) {
                Box_TargetBox(lot, box_num);
                creature->mood = MOOD_STALK;
            }
        }
        break;
    }

    case MOOD_STALK: {
        if (lot->required_box == NO_BOX
            || !Box_StalkBox(item, enemy, lot->required_box)) {
            const int16_t box_num =
                lot->node[lot->zone_count * Random_GetControl() / 0x7FFF]
                    .box_num;
            if (Box_ValidBox(item, info->zone_num, box_num)) {
                if (Box_StalkBox(item, enemy, box_num)) {
                    Box_TargetBox(lot, box_num);
                } else if (lot->required_box == NO_BOX) {
                    Box_TargetBox(lot, box_num);
                    if (info->zone_num != info->enemy_zone_num) {
                        creature->mood = MOOD_BORED;
                    }
                }
            }
        }
        break;
    }
    }

    if (lot->target_box == NO_BOX) {
        Box_TargetBox(lot, item->box_num);
    }
    Box_CalculateTarget(&creature->target, item, lot);
}

int16_t Creature_Turn(ITEM *const item, int16_t max_turn)
{
    const CREATURE *const creature = item->data;
    if (creature == nullptr || item->speed == 0 || max_turn == 0) {
        return 0;
    }

    const int32_t dx = creature->target.x - item->pos.x;
    const int32_t dz = creature->target.z - item->pos.z;

    int16_t angle = Math_Atan(dz, dx) - item->rot.y;
    if (angle > FRONT_ARC || angle < -FRONT_ARC) {
        const int32_t range = (item->speed << 14) / max_turn;
        if (SQUARE(dx) + SQUARE(dz) < SQUARE(range)) {
            max_turn /= 2;
        }
    }

    CLAMP(angle, -max_turn, max_turn);
    item->rot.y += angle;
    return angle;
}

void Creature_Tilt(ITEM *const item, int16_t angle)
{
    angle = angle * 4 - item->rot.z;
    CLAMP(angle, -M_MAX_TILT, M_MAX_TILT);
    item->rot.z += angle;
}

void Creature_Head(ITEM *const item, const int16_t required)
{
    CREATURE *const creature = item->data;
    if (creature == nullptr) {
        return;
    }

    int16_t change = required - creature->head_rotation;
    CLAMP(change, -M_MAX_HEAD_CHANGE, M_MAX_HEAD_CHANGE);

    creature->head_rotation += change;
    CLAMP(creature->head_rotation, -M_HEAD_ARC, M_HEAD_ARC);
}

void Creature_Neck(ITEM *const item, const int16_t required)
{
    CREATURE *const creature = item->data;
    if (creature == nullptr) {
        return;
    }

    int16_t change = required - creature->neck_rotation;
    CLAMP(change, -M_MAX_HEAD_CHANGE, M_MAX_HEAD_CHANGE);

    creature->neck_rotation += change;
    CLAMP(creature->neck_rotation, -M_HEAD_ARC, M_HEAD_ARC);
}

void Creature_Float(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    item->hit_points = DONT_TARGET;
    item->rot.x = 0;

    const int32_t wh = Room_GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_num);
    if (item->pos.y > wh) {
        item->pos.y -= M_FLOAT_SPEED;
    } else if (item->pos.y < wh) {
        item->pos.y = wh;
    }

    Item_Animate(item);

    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    item->floor = Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
    Item_UpdateRoom(item_num, room_num);
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

bool Creature_CanTargetEnemy(const ITEM *const item, const AI_INFO *const info)
{
    const CREATURE *const creature = item->data;
    const ITEM *const enemy =
        creature->enemy != nullptr ? creature->enemy : Lara_GetItem();
    if (!info->ahead || info->distance >= CREATURE_SHOOT_RANGE) {
        return false;
    }

    GAME_VECTOR start;
    start.pos.x = item->pos.x;
    start.pos.y = item->pos.y - STEP_L * 3;
    start.pos.z = item->pos.z;
    start.room_num = item->room_num;

    GAME_VECTOR target;
    target.pos.x = enemy->pos.x;
    target.pos.y = enemy->pos.y - STEP_L * 3;
    target.pos.z = enemy->pos.z;
    target.room_num = enemy->room_num;

    return LOS_Check(&start, &target);
}

bool Creature_CheckBaddieOverlap(const int16_t item_num)
{
    const ITEM *item = Item_Get(item_num);

    const int32_t x = item->pos.x;
    const int32_t y = item->pos.y;
    const int32_t z = item->pos.z;
    const int32_t radius = SQUARE(Object_Get(item->object_id)->radius);

    int16_t link = Room_Get(item->room_num)->item_num;
    while (link != NO_ITEM && link != item_num) {
        item = Item_Get(link);
        if (item != Lara_GetItem() && item->status == IS_ACTIVE
            && item->speed != 0) {
            const XYZ_32 delta = {
                item->pos.x - x,
                item->pos.y - y,
                item->pos.z - z,
            };
            const int32_t distance =
                SQUARE(delta.x) + SQUARE(delta.y) + SQUARE(delta.z);
            if (distance < radius) {
                return true;
            }
        }

        link = item->next_item;
    }

    return false;
}

void Creature_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }
    if (!Collide_TestCollision(item, lara_item)) {
        return;
    }

    if (!coll->enable_baddie_push) {
        return;
    }

    if (TR_VERSION >= 2
        && (lara->water_status == LWS_UNDERWATER
            || lara->water_status == LWS_SURFACE)) {
        return;
    }

    Lara_Push(
        item, coll,
        (TR_VERSION >= 2 || item->hit_points > 0) ? coll->enable_hit : false,
        false);
}

bool Creature_Animate(
    const int16_t item_num, const int16_t angle, const int16_t tilt)
{
    ITEM *const item = Item_Get(item_num);
    const CREATURE *const creature = item->data;
    const OBJECT *const obj = Object_Get(item->object_id);
    if (creature == nullptr) {
        return false;
    }

    const LOT_INFO *const lot = &creature->lot;
    const XYZ_32 old = item->pos;

    const int16_t *const zone = Box_GetLotZone(lot);

    if (TR_VERSION >= 2 && !Object_IsType(item->object_id, g_WaterObjects)) {
        int16_t room_num = item->room_num;
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
        Item_UpdateRoom(item_num, room_num);
    }

    Item_Animate(item);
    if (item->status == IS_DEACTIVATED) {
        Creature_Die(item_num, false);
        return false;
    }

    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
    int32_t y = item->pos.y + bounds->min.y;

    int16_t room_num = item->room_num;
    Room_GetSector(old.x, y, old.z, &room_num);
    const SECTOR *sector =
        Room_GetSector(item->pos.x, y, item->pos.z, &room_num);
    int32_t height = Box_GetBox(sector->box)->height;
    int16_t next_box = lot->node[sector->box].exit_box;
    int32_t next_height =
        next_box != NO_BOX ? Box_GetBox(next_box)->height : height;

    const int32_t box_height = Box_GetBox(item->box_num)->height;
    if (sector->box == NO_BOX || zone[item->box_num] != zone[sector->box]
        || box_height - height > lot->setup.step
        || box_height - height < lot->setup.drop) {
        const int32_t pos_x = item->pos.x >> WALL_SHIFT;
        const int32_t shift_x = old.x >> WALL_SHIFT;
        const int32_t shift_z = old.z >> WALL_SHIFT;

        if (pos_x < shift_x) {
            item->pos.x = old.x & (~(WALL_L - 1));
        } else if (pos_x > shift_x) {
            item->pos.x = old.x | (WALL_L - 1);
        }

        if (pos_x < shift_z) {
            item->pos.z = old.z & (~(WALL_L - 1));
        } else if (pos_x > shift_z) {
            item->pos.z = old.z | (WALL_L - 1);
        }

        sector = Room_GetSector(item->pos.x, y, item->pos.z, &room_num);
        height = Box_GetBox(sector->box)->height;
        next_box = lot->node[sector->box].exit_box;
        next_height =
            next_box != NO_BOX ? Box_GetBox(next_box)->height : height;
    }

    const int32_t x = item->pos.x;
    const int32_t z = item->pos.z;
    const int32_t pos_x = x & (WALL_L - 1);
    const int32_t pos_z = z & (WALL_L - 1);
    int32_t shift_x = 0;
    int32_t shift_z = 0;
    const int32_t radius = obj->radius;

    if (pos_z < radius) {
        if (Box_BadFloor(
                x, y, z - radius, height, next_height, room_num, lot)) {
            shift_z = radius - pos_z;
        }

        if (pos_x < radius) {
            if (Box_BadFloor(
                    x - radius, y, z, height, next_height, room_num, lot)) {
                shift_x = radius - pos_x;
            } else if (
                !shift_z
                && Box_BadFloor(
                    x - radius, y, z - radius, height, next_height, room_num,
                    lot)) {
                if (item->rot.y > -DEG_135 && item->rot.y < DEG_45) {
                    shift_z = radius - pos_z;
                } else {
                    shift_x = radius - pos_x;
                }
            }
        } else if (pos_x > WALL_L - radius) {
            if (Box_BadFloor(
                    x + radius, y, z, height, next_height, room_num, lot)) {
                shift_x = WALL_L - radius - pos_x;
            } else if (
                !shift_z
                && Box_BadFloor(
                    x + radius, y, z - radius, height, next_height, room_num,
                    lot)) {
                if (item->rot.y > -DEG_45 && item->rot.y < DEG_135) {
                    shift_z = radius - pos_z;
                } else {
                    shift_x = WALL_L - radius - pos_x;
                }
            }
        }
    } else if (pos_z > WALL_L - radius) {
        if (Box_BadFloor(
                x, y, z + radius, height, next_height, room_num, lot)) {
            shift_z = WALL_L - radius - pos_z;
        }

        if (pos_x < radius) {
            if (Box_BadFloor(
                    x - radius, y, z, height, next_height, room_num, lot)) {
                shift_x = radius - pos_x;
            } else if (
                !shift_z
                && Box_BadFloor(
                    x - radius, y, z + radius, height, next_height, room_num,
                    lot)) {
                if (item->rot.y > -DEG_45 && item->rot.y < DEG_135) {
                    shift_x = radius - pos_x;
                } else {
                    shift_z = WALL_L - radius - pos_z;
                }
            }
        } else if (pos_x > WALL_L - radius) {
            if (Box_BadFloor(
                    x + radius, y, z, height, next_height, room_num, lot)) {
                shift_x = WALL_L - radius - pos_x;
            } else if (
                !shift_z
                && Box_BadFloor(
                    x + radius, y, z + radius, height, next_height, room_num,
                    lot)) {
                if (item->rot.y > -DEG_135 && item->rot.y < DEG_45) {
                    shift_x = WALL_L - radius - pos_x;
                } else {
                    shift_z = WALL_L - radius - pos_z;
                }
            }
        }
    } else if (pos_x < radius) {
        if (Box_BadFloor(
                x - radius, y, z, height, next_height, room_num, lot)) {
            shift_x = radius - pos_x;
        }
    } else if (pos_x > WALL_L - radius) {
        if (Box_BadFloor(
                x + radius, y, z, height, next_height, room_num, lot)) {
            shift_x = WALL_L - radius - pos_x;
        }
    }

    item->pos.x += shift_x;
    item->pos.z += shift_z;

    if (shift_x || shift_z) {
        sector = Room_GetSector(item->pos.x, y, item->pos.z, &room_num);
        item->rot.y += angle;
        Creature_Tilt(item, tilt * 2);
    }

    if (Creature_CheckBaddieOverlap(item_num)) {
        item->pos = old;
        return true;
    }

    if (lot->setup.fly != 0) {
        int32_t dy = creature->target.y - item->pos.y;
        CLAMP(dy, -lot->setup.fly, lot->setup.fly);

        height = Room_GetHeight(sector, item->pos.x, y, item->pos.z);
        if (item->pos.y + dy <= height) {
            const int32_t ceiling =
                Room_GetCeiling(sector, item->pos.x, y, item->pos.z);
            int32_t min_y = bounds->min.y;
            switch (item->object_id) {
            case O_ALLIGATOR:
                min_y = 0;
                break;
            case O_SHARK:
                min_y = 128;
                break;
            default:
                break;
            }
            if (item->pos.y + min_y + dy < ceiling) {
                if (item->pos.y + min_y < ceiling) {
                    item->pos.x = old.x;
                    item->pos.z = old.z;
                    dy = lot->setup.fly;
                } else {
                    dy = 0;
                }
            }
        } else if (item->pos.y <= height) {
            item->pos.y = height;
            dy = 0;
        } else {
            item->pos.x = old.x;
            item->pos.z = old.z;
            dy = -lot->setup.fly;
        }

        item->pos.y += dy;
        sector = Room_GetSector(item->pos.x, y, item->pos.z, &room_num);
        item->floor = Room_GetHeight(sector, item->pos.x, y, item->pos.z);

        int16_t angle = item->speed ? Math_Atan(item->speed, -dy) : 0;
        if (TR_VERSION >= 2) {
            CLAMP(angle, -M_MAX_X_ROT, M_MAX_X_ROT);
        }

        if (angle < item->rot.x - DEG_1) {
            item->rot.x -= DEG_1;
        } else if (angle > item->rot.x + DEG_1) {
            item->rot.x += DEG_1;
        } else {
            item->rot.x = angle;
        }
    } else {
        const SECTOR *const sector =
            Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
        item->floor =
            Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);

        if (item->pos.y > item->floor) {
            item->pos.y = item->floor;
        } else if (item->floor - item->pos.y > STEP_L / 4) {
            item->pos.y += STEP_L / 4;
        } else if (item->pos.y < item->floor) {
            item->pos.y = item->floor;
        }
        item->rot.x = 0;
    }

    if (!Object_IsType(item->object_id, g_WaterObjects)) {
        // Get the room just above the enemy so that if it is in one-click high
        // water, its effects behave still as though in a dry room.
        Room_GetSector(
            item->pos.x, item->pos.y - (STEP_L * 2), item->pos.z, &room_num);
        if (TR_VERSION >= 2
            && (Room_Get(room_num)->flags & RF_UNDERWATER) != 0) {
            item->hit_points = 0;
        }
    }

    Item_UpdateRoom(item_num, room_num);
    return true;
}

void Creature_SpecialKill(
    ITEM *const item, const int32_t kill_anim, const int32_t kill_state,
    const int32_t lara_kill_state)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    ITEM *const lara_item = Lara_GetItem();

    Item_SwitchToAnim(item, kill_anim, 0);
    item->current_anim_state = kill_state;

#if TR_VERSION == 2
    Item_SwitchToObjAnim(lara_item, LS_EXTRA_BREATH, 0, O_LARA_EXTRA);
    lara_item->current_anim_state = LS_EXTRA_BREATH;
#endif
    lara_item->goal_anim_state = lara_kill_state;
    lara_item->pos = item->pos;
    lara_item->rot = item->rot;
    lara_item->fall_speed = 0;
    lara_item->gravity = 0;
    lara_item->speed = 0;

    int16_t room_num = item->room_num;
    Item_UpdateRoom(lara->item_num, room_num);

    Item_Animate(lara_item);

    lara_item->goal_anim_state = lara_kill_state;
    lara_item->current_anim_state = lara_kill_state;
#if TR_VERSION == 2
    lara->extra_anim = true;
#endif
    lara->gun_status = LGS_HANDS_BUSY;
    lara->gun_type = LGT_UNARMED;
    lara->hit_direction = -1;
    lara->air = -1;

    g_Camera.pos.room_num = lara_item->room_num;
}

void Creature_Die(const int16_t item_num, const bool explode)
{
    ITEM *const item = Item_Get(item_num);

    switch (item->object_id) {
    case O_DRAGON_FRONT:
        item->hit_points = 0;
        return;

    case O_SKIDOO_DRIVER:
        if (explode) {
            Item_Explode(item_num, -1, 0);
        }
        item->hit_points = DONT_TARGET;
        const int16_t vehicle_item_num = (int16_t)(intptr_t)item->data;
        ITEM *const vehicle_item = Item_Get(vehicle_item_num);
        vehicle_item->hit_points = 0;
        return;

    default:
        break;
    }

    item->collidable = false;
    item->hit_points = DONT_TARGET;
    if (explode) {
        Item_Explode(item_num, -1, 0);
        Item_Kill(item_num);
    } else {
        Item_RemoveActive(item_num);
    }

    const OBJECT *const obj = Object_Get(item->object_id);
    if (obj->intelligent) {
        LOT_DisableBaddieAI(item_num);
    }
    item->flags |= IF_ONE_SHOT;

#if TR_VERSION >= 2
    if (item->killed) {
        item->next_active = Item_GetPrevActive();
        Item_SetPrevActive(item_num);
    }
#endif

    Carrier_TestItemDrops(item_num);
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

    Item_UpdateRoom(item_num, room_num);
    return vault;
}

bool Creature_AreAlliesHostile(void)
{
    return m_AlliesHostile;
}

void Creature_SetAlliesHostile(bool enable)
{
    m_AlliesHostile = enable;
}

bool Creature_IsAlive(const ITEM *const item)
{
    const OBJECT *const obj = Object_Get(item->object_id);
    if (obj->intelligent && Object_IsType(item->object_id, g_WaterObjects)) {
        return item->hit_points > 0;
    }

    return (item->hit_points > 0)
        || (item->hit_points == DONT_TARGET && item->active);
}

bool Creature_IsHostile(const ITEM *const item)
{
    return Object_IsType(item->object_id, g_EnemyObjects)
        || (Creature_AreAlliesHostile() && Creature_IsAlly(item));
}

bool Creature_IsAlly(const ITEM *const item)
{
    return Object_IsType(item->object_id, g_AllyObjects);
}

bool Creature_IsAllyTargetingEnemy(const ITEM *const item)
{
    return Object_IsType(item->object_id, g_AllyTargetingEnemies);
}

bool Creature_IsTargetable(const ITEM *const item)
{
#if TR_VERSION == 1
    return true;
#else
    return item->hit_points != DONT_TARGET
        && (!Object_Get(item->object_id)->intelligent
            || item->status == IS_ACTIVE)
        && (g_Config.gameplay.enable_ally_targeting || !Creature_IsAlly(item));
#endif
}

int16_t Creature_Effect(
    const ITEM *const item, const BITE *const bite,
    int16_t (*const spawn)(
        int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
        int16_t room_num))
{
    XYZ_32 pos = bite->pos;
    Collide_GetJointAbsPosition(item, &pos, bite->mesh_num);
    return spawn(pos.x, pos.y, pos.z, item->speed, item->rot.y, item->room_num);
}
