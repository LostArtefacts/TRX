#include "game/creature.h"

#include "game/box.h"
#include "game/carrier.h"
#include "game/collide.h"
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

#include <libtrx/game/math.h>
#include <libtrx/log.h>

#define MAX_CREATURE_DISTANCE (WALL_L * 30)

static bool M_SwitchToWater(
    int16_t item_num, const int32_t *wh, const HYBRID_INFO *info);
static bool M_SwitchToLand(
    int16_t item_num, const int32_t *wh, const HYBRID_INFO *info);
static bool M_TestSwitchOrKill(int16_t item_num, GAME_OBJECT_ID target_id);

void Creature_Initialise(int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    item->rot.y += (PHD_ANGLE)((Random_GetControl() - DEG_90) >> 1);
    item->collidable = 1;
    item->data = nullptr;
}

void Creature_AIInfo(ITEM *item, AI_INFO *info)
{
    CREATURE *creature = item->data;
    if (!creature) {
        return;
    }

    const bool flip_status = Room_GetFlipStatus();
    int16_t *zone;
    if (creature->lot.fly) {
        zone = g_FlyZone[flip_status];
    } else if (creature->lot.step == STEP_L) {
        zone = g_GroundZone[flip_status];
    } else {
        zone = g_GroundZone2[flip_status];
    }

    const ROOM *room = Room_Get(item->room_num);
    item->box_num = Room_GetWorldSector(room, item->pos.x, item->pos.z)->box;
    info->zone_num = zone[item->box_num];

    room = Room_Get(g_LaraItem->room_num);
    g_LaraItem->box_num =
        Room_GetWorldSector(room, g_LaraItem->pos.x, g_LaraItem->pos.z)->box;
    info->enemy_zone = zone[g_LaraItem->box_num];

    if (g_Boxes[g_LaraItem->box_num].overlap_index & creature->lot.block_mask) {
        info->enemy_zone |= BLOCKED;
    } else if (
        creature->lot.node[item->box_num].search_num
        == (creature->lot.search_num | BLOCKED_SEARCH)) {
        info->enemy_zone |= BLOCKED;
    }

    const OBJECT *const obj = Object_Get(item->object_id);
    int32_t z = g_LaraItem->pos.z
        - ((Math_Cos(item->rot.y) * obj->pivot_length) >> W2V_SHIFT)
        - item->pos.z;
    int32_t x = g_LaraItem->pos.x
        - ((Math_Sin(item->rot.y) * obj->pivot_length) >> W2V_SHIFT)
        - item->pos.x;

    PHD_ANGLE angle = Math_Atan(z, x);
    info->distance = SQUARE(x) + SQUARE(z);
    if (ABS(x) > MAX_CREATURE_DISTANCE || ABS(z) > MAX_CREATURE_DISTANCE) {
        info->distance = SQUARE(MAX_CREATURE_DISTANCE);
    }
    info->angle = angle - item->rot.y;
    info->enemy_facing = angle - g_LaraItem->rot.y + DEG_180;
    info->ahead = info->angle > -FRONT_ARC && info->angle < FRONT_ARC;
    info->bite = info->ahead && (g_LaraItem->pos.y > item->pos.y - STEP_L)
        && (g_LaraItem->pos.y < item->pos.y + STEP_L);
}

void Creature_Mood(ITEM *item, AI_INFO *info, bool violent)
{
    CREATURE *creature = item->data;
    if (!creature) {
        return;
    }

    LOT_INFO *lot = &creature->lot;
    if (lot->node[item->box_num].search_num
        == (lot->search_num | BLOCKED_SEARCH)) {
        lot->required_box = NO_BOX;
    }

    if (creature->mood != MOOD_ATTACK && lot->required_box != NO_BOX
        && !Box_ValidBox(item, info->zone_num, lot->target_box)) {
        if (info->zone_num == info->enemy_zone) {
            creature->mood = MOOD_BORED;
        }
        lot->required_box = NO_BOX;
    }

    MOOD_TYPE mood = creature->mood;

    if (g_LaraItem->hit_points <= 0) {
        creature->mood = MOOD_BORED;
    } else if (violent) {
        switch (mood) {
        case MOOD_ATTACK:
            if (info->zone_num != info->enemy_zone) {
                creature->mood = MOOD_BORED;
            }
            break;

        case MOOD_BORED:
        case MOOD_STALK:
            if (info->zone_num == info->enemy_zone) {
                creature->mood = MOOD_ATTACK;
            } else if (item->hit_status) {
                creature->mood = MOOD_ESCAPE;
            }
            break;

        case MOOD_ESCAPE:
            if (info->zone_num == info->enemy_zone) {
                creature->mood = MOOD_ATTACK;
            }
            break;
        }
    } else {
        switch (mood) {
        case MOOD_ATTACK:
            if (item->hit_status
                && (Random_GetControl() < ESCAPE_CHANCE
                    || info->zone_num != info->enemy_zone)) {
                creature->mood = MOOD_ESCAPE;
            } else if (info->zone_num != info->enemy_zone) {
                creature->mood = MOOD_BORED;
            }
            break;

        case MOOD_BORED:
        case MOOD_STALK:
            if (item->hit_status
                && (Random_GetControl() < ESCAPE_CHANCE
                    || info->zone_num != info->enemy_zone)) {
                creature->mood = MOOD_ESCAPE;
            } else if (info->zone_num == info->enemy_zone) {
                if (info->distance < ATTACK_RANGE
                    || (creature->mood == MOOD_STALK
                        && lot->required_box == NO_BOX)) {
                    creature->mood = MOOD_ATTACK;
                } else {
                    creature->mood = MOOD_STALK;
                }
            }
            break;

        case MOOD_ESCAPE:
            if (info->zone_num == info->enemy_zone
                && Random_GetControl() < RECOVER_CHANCE) {
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
    case MOOD_ATTACK:
        if (Random_GetControl() < Object_Get(item->object_id)->smartness) {
            lot->target.x = g_LaraItem->pos.x;
            lot->target.y = g_LaraItem->pos.y;
            lot->target.z = g_LaraItem->pos.z;
            lot->required_box = g_LaraItem->box_num;
            if (lot->fly && g_Lara.water_status == LWS_ABOVE_WATER) {
                const ANIM_FRAME *const frame = Item_GetBestFrame(g_LaraItem);
                lot->target.y += frame->bounds.min.y;
            }
        }
        break;

    case MOOD_BORED: {
        int box_num =
            lot->node[Random_GetControl() * lot->zone_count / 0x7FFF].box_num;
        if (Box_ValidBox(item, info->zone_num, box_num)) {
            if (Box_StalkBox(item, box_num)) {
                Box_TargetBox(lot, box_num);
                creature->mood = MOOD_STALK;
            } else if (lot->required_box == NO_BOX) {
                Box_TargetBox(lot, box_num);
            }
        }
        break;
    }

    case MOOD_STALK: {
        if (lot->required_box == NO_BOX
            || !Box_StalkBox(item, lot->required_box)) {
            int box_num =
                lot->node[Random_GetControl() * lot->zone_count / 0x7FFF]
                    .box_num;
            if (Box_ValidBox(item, info->zone_num, box_num)) {
                if (Box_StalkBox(item, box_num)) {
                    Box_TargetBox(lot, box_num);
                } else if (lot->required_box == NO_BOX) {
                    Box_TargetBox(lot, box_num);
                    if (info->zone_num != info->enemy_zone) {
                        creature->mood = MOOD_BORED;
                    }
                }
            }
        }
        break;
    }

    case MOOD_ESCAPE: {
        int box_num =
            lot->node[Random_GetControl() * lot->zone_count / 0x7FFF].box_num;
        if (Box_ValidBox(item, info->zone_num, box_num)
            && lot->required_box == NO_BOX) {
            if (Box_EscapeBox(item, box_num)) {
                Box_TargetBox(lot, box_num);
            } else if (
                info->zone_num == info->enemy_zone
                && Box_StalkBox(item, box_num)) {
                Box_TargetBox(lot, box_num);
                creature->mood = MOOD_STALK;
            }
        }
        break;
    }
    }

    if (lot->target_box == NO_BOX) {
        Box_TargetBox(lot, item->box_num);
    }

    Box_CalculateTarget(&creature->target, item, &creature->lot);
}

int16_t Creature_Turn(ITEM *item, int16_t maximum_turn)
{
    CREATURE *creature = item->data;
    if (!creature) {
        return 0;
    }

    if (!item->speed || !maximum_turn) {
        return 0;
    }

    int32_t x = creature->target.x - item->pos.x;
    int32_t z = creature->target.z - item->pos.z;
    int16_t angle = Math_Atan(z, x) - item->rot.y;
    int32_t range = (item->speed << 14) / maximum_turn;

    if (angle > FRONT_ARC || angle < -FRONT_ARC) {
        if (SQUARE(x) + SQUARE(z) < SQUARE(range)) {
            maximum_turn >>= 1;
        }
    }

    if (angle > maximum_turn) {
        angle = maximum_turn;
    } else if (angle < -maximum_turn) {
        angle = -maximum_turn;
    }

    item->rot.y += angle;

    return angle;
}

void Creature_Tilt(ITEM *item, int16_t angle)
{
    angle = angle * 4 - item->rot.z;
    if (angle < -MAX_TILT) {
        angle = -MAX_TILT;
    } else if (angle > MAX_TILT) {
        angle = MAX_TILT;
    }
    item->rot.z += angle;
}

void Creature_Head(ITEM *item, int16_t required)
{
    CREATURE *creature = item->data;
    if (!creature) {
        return;
    }

    int16_t change = required - creature->head_rotation;
    if (change > MAX_HEAD_CHANGE) {
        change = MAX_HEAD_CHANGE;
    } else if (change < -MAX_HEAD_CHANGE) {
        change = -MAX_HEAD_CHANGE;
    }

    creature->head_rotation += change;

    if (creature->head_rotation > FRONT_ARC) {
        creature->head_rotation = FRONT_ARC;
    } else if (creature->head_rotation < -FRONT_ARC) {
        creature->head_rotation = -FRONT_ARC;
    }
}

int16_t Creature_Effect(
    ITEM *item, BITE *bite,
    int16_t (*spawn)(
        int32_t x, int32_t y, int32_t z, int16_t speed, int16_t yrot,
        int16_t room_num))
{
    XYZ_32 pos = {
        .x = bite->x,
        .y = bite->y,
        .z = bite->z,
    };
    Collide_GetJointAbsPosition(item, &pos, bite->mesh_num);
    return spawn(pos.x, pos.y, pos.z, item->speed, item->rot.y, item->room_num);
}

bool Creature_CheckBaddieOverlap(int16_t item_num)
{
    const ITEM *item = Item_Get(item_num);

    int32_t x = item->pos.x;
    int32_t y = item->pos.y;
    int32_t z = item->pos.z;
    const int32_t radius = SQUARE(Object_Get(item->object_id)->radius);

    int16_t link = Room_Get(item->room_num)->item_num;
    do {
        item = Item_Get(link);

        if (link == item_num) {
            return false;
        }

        if (item != g_LaraItem && item->status == IS_ACTIVE
            && item->speed != 0) {
            int32_t distance = SQUARE(item->pos.x - x) + SQUARE(item->pos.y - y)
                + SQUARE(item->pos.z - z);
            if (distance < radius) {
                return true;
            }
        }

        link = item->next_item;
    } while (link != NO_ITEM);

    return false;
}

void Creature_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll)
{
    ITEM *const item = Item_Get(item_num);

    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }
    if (!Collide_TestCollision(item, lara_item)) {
        return;
    }

    if (coll->enable_baddie_push) {
        if (item->hit_points <= 0) {
            Lara_Push(item, coll, 0, 0);
        } else {
            Lara_Push(item, coll, coll->enable_hit, 0);
        }
    }
}

bool Creature_Animate(int16_t item_num, int16_t angle, int16_t tilt)
{
    ITEM *const item = Item_Get(item_num);
    CREATURE *creature = item->data;
    if (!creature) {
        return false;
    }
    LOT_INFO *lot = &creature->lot;

    XYZ_32 old = {
        .x = item->pos.x,
        .y = item->pos.y,
        .z = item->pos.z,
    };

    int32_t box_height = g_Boxes[item->box_num].height;

    const bool flip_status = Room_GetFlipStatus();
    int16_t *zone;
    if (lot->fly) {
        zone = g_FlyZone[flip_status];
    } else if (lot->step == STEP_L) {
        zone = g_GroundZone[flip_status];
    } else {
        zone = g_GroundZone2[flip_status];
    }

    Item_Animate(item);
    if (item->status == IS_DEACTIVATED) {
        item->collidable = 0;
        item->hit_points = DONT_TARGET;
        LOT_DisableBaddieAI(item_num);
        Item_RemoveActive(item_num);
        Carrier_TestItemDrops(item_num);
        return false;
    }

    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
    int32_t y = item->pos.y + bounds->min.y;

    int16_t room_num = item->room_num;
    const SECTOR *sector =
        Room_GetSector(item->pos.x, y, item->pos.z, &room_num);
    int32_t height = g_Boxes[sector->box].height;
    int16_t next_box = lot->node[sector->box].exit_box;
    int32_t next_height;
    if (next_box != NO_BOX) {
        next_height = g_Boxes[next_box].height;
    } else {
        next_height = height;
    }

    int32_t pos_x;
    int32_t pos_z;
    int32_t shift_x;
    int32_t shift_z;
    if (sector->box == NO_BOX || zone[item->box_num] != zone[sector->box]
        || box_height - height > lot->step || box_height - height < lot->drop) {
        pos_x = item->pos.x >> WALL_SHIFT;

        shift_x = old.x >> WALL_SHIFT;
        shift_z = old.z >> WALL_SHIFT;

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
        height = g_Boxes[sector->box].height;
        next_box = lot->node[sector->box].exit_box;
        if (next_box != NO_BOX) {
            next_height = g_Boxes[next_box].height;
        } else {
            next_height = height;
        }
    }

    int32_t x = item->pos.x;
    int32_t z = item->pos.z;

    pos_x = x & (WALL_L - 1);
    pos_z = z & (WALL_L - 1);
    shift_x = 0;
    shift_z = 0;

    const int32_t radius = Object_Get(item->object_id)->radius;

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
        item->pos.x = old.x;
        item->pos.y = old.y;
        item->pos.z = old.z;
        return true;
    }

    if (lot->fly) {
        int32_t dy = creature->target.y - item->pos.y;
        if (dy > lot->fly) {
            dy = lot->fly;
        } else if (dy < -lot->fly) {
            dy = -lot->fly;
        }

        height = Room_GetHeight(sector, item->pos.x, y, item->pos.z);
        if (item->pos.y + dy > height) {
            if (item->pos.y > height) {
                item->pos.x = old.x;
                item->pos.z = old.z;
                dy = -lot->fly;
            } else {
                dy = 0;
                item->pos.y = height;
            }
        } else {
            int32_t ceiling =
                Room_GetCeiling(sector, item->pos.x, y, item->pos.z);

            int32_t min_y = item->object_id == O_ALLIGATOR ? 0 : bounds->min.y;
            if (item->pos.y + min_y + dy < ceiling) {
                if (item->pos.y + min_y < ceiling) {
                    item->pos.x = old.x;
                    item->pos.z = old.z;
                    dy = lot->fly;
                } else {
                    dy = 0;
                }
            }
        }

        item->pos.y += dy;
        sector = Room_GetSector(item->pos.x, y, item->pos.z, &room_num);
        item->floor = Room_GetHeight(sector, item->pos.x, y, item->pos.z);

        angle = item->speed ? Math_Atan(item->speed, -dy) : 0;
        if (angle < item->rot.x - DEG_1) {
            item->rot.x -= DEG_1;
        } else if (angle > item->rot.x + DEG_1) {
            item->rot.x += DEG_1;
        } else {
            item->rot.x = angle;
        }
    } else {
        sector =
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

    if (item->room_num != room_num) {
        Item_NewRoom(item_num, room_num);
    }

    return true;
}

bool Creature_CanTargetEnemy(ITEM *item, AI_INFO *info)
{
    if (!info->ahead || info->distance >= CREATURE_SHOOT_RANGE) {
        return false;
    }

    GAME_VECTOR start;
    start.x = item->pos.x;
    start.y = item->pos.y - STEP_L * 3;
    start.z = item->pos.z;
    start.room_num = item->room_num;

    GAME_VECTOR target;
    target.x = g_LaraItem->pos.x;
    target.y = g_LaraItem->pos.y - STEP_L * 3;
    target.z = g_LaraItem->pos.z;

    return LOS_Check(&start, &target);
}

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

bool Creature_EnsureHabitat(
    const int16_t item_num, int32_t *const wh, const HYBRID_INFO *const info)
{
    // Test the environment for a hybrid creature. Record the water height and
    // return whether or not a type conversion has taken place.
    const ITEM *const item = Item_Get(item_num);
    *wh = Room_GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_num);

    return item->object_id == info->land.id
        ? M_SwitchToWater(item_num, wh, info)
        : M_SwitchToLand(item_num, wh, info);
}

bool Creature_IsBoss(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    return Object_IsType(item->object_id, g_BossObjects);
}

static bool M_SwitchToWater(
    const int16_t item_num, const int32_t *const wh,
    const HYBRID_INFO *const info)
{
    if (*wh == NO_HEIGHT) {
        return false;
    }

    ITEM *const item = Item_Get(item_num);

    if (item->hit_points <= 0) {
        // Dead land creatures should remain in their pose permanently.
        return false;
    }

    // The land creature is alive and the room has been flooded. Switch to the
    // water creature.
    if (!M_TestSwitchOrKill(item_num, info->water.id)) {
        return false;
    }

    item->object_id = info->water.id;
    Item_SwitchToAnim(item, info->water.active_anim, 0);
    item->current_anim_state = Item_GetAnim(item)->current_anim_state;
    item->goal_anim_state = item->current_anim_state;
    item->pos.y = *wh;

    return true;
}

static bool M_SwitchToLand(
    const int16_t item_num, const int32_t *const wh,
    const HYBRID_INFO *const info)
{
    if (*wh != NO_HEIGHT) {
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

        if (item->room_num != room_num) {
            Item_NewRoom(item_num, room_num);
        }
    }

    return true;
}

static bool M_TestSwitchOrKill(
    const int16_t item_num, const GAME_OBJECT_ID target_id)
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

bool Creature_IsHostile(const ITEM *const item)
{
    return Object_IsType(item->object_id, g_EnemyObjects);
}

bool Creature_IsAlly(const ITEM *const item)
{
    return Object_IsType(item->object_id, g_AllyObjects);
}
