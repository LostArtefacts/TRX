#include "game/creature.h"

#include "game/box.h"
#include "game/carrier.h"
#include "game/collide.h"
#include "game/effects.h"
#include "game/effects/gunshot.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/los.h"
#include "game/lot.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/room.h"
#include "global/vars.h"
#include "math/math.h"

#include <stddef.h>

#define MAX_CREATURE_DISTANCE (WALL_L * 30)

static bool Creature_SwitchToWater(
    int16_t item_num, int32_t *wh, const HYBRID_INFO *info);
static bool Creature_SwitchToLand(
    int16_t item_num, int32_t *wh, const HYBRID_INFO *info);

void Creature_Initialise(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    item->rot.y += (PHD_ANGLE)((Random_GetControl() - PHD_90) >> 1);
    item->collidable = 1;
    item->data = NULL;
}

void Creature_AIInfo(ITEM_INFO *item, AI_INFO *info)
{
    CREATURE_INFO *creature = item->data;
    if (!creature) {
        return;
    }

    int16_t *zone;
    if (creature->LOT.fly) {
        zone = g_FlyZone[g_FlipStatus];
    } else if (creature->LOT.step == STEP_L) {
        zone = g_GroundZone[g_FlipStatus];
    } else {
        zone = g_GroundZone2[g_FlipStatus];
    }

    ROOM_INFO *r = &g_RoomInfo[item->room_number];
    int32_t x_floor = (item->pos.z - r->z) >> WALL_SHIFT;
    int32_t y_floor = (item->pos.x - r->x) >> WALL_SHIFT;
    item->box_number = r->floor[x_floor + y_floor * r->x_size].box;
    info->zone_number = zone[item->box_number];

    r = &g_RoomInfo[g_LaraItem->room_number];
    x_floor = (g_LaraItem->pos.z - r->z) >> WALL_SHIFT;
    y_floor = (g_LaraItem->pos.x - r->x) >> WALL_SHIFT;
    g_LaraItem->box_number = r->floor[x_floor + y_floor * r->x_size].box;
    info->enemy_zone = zone[g_LaraItem->box_number];

    if (g_Boxes[g_LaraItem->box_number].overlap_index
        & creature->LOT.block_mask) {
        info->enemy_zone |= BLOCKED;
    } else if (
        creature->LOT.node[item->box_number].search_number
        == (creature->LOT.search_number | BLOCKED_SEARCH)) {
        info->enemy_zone |= BLOCKED;
    }

    OBJECT_INFO *object = &g_Objects[item->object_number];
    int32_t z = g_LaraItem->pos.z
        - ((Math_Cos(item->rot.y) * object->pivot_length) >> W2V_SHIFT)
        - item->pos.z;
    int32_t x = g_LaraItem->pos.x
        - ((Math_Sin(item->rot.y) * object->pivot_length) >> W2V_SHIFT)
        - item->pos.x;

    PHD_ANGLE angle = Math_Atan(z, x);
    info->distance = SQUARE(x) + SQUARE(z);
    if (ABS(x) > MAX_CREATURE_DISTANCE || ABS(z) > MAX_CREATURE_DISTANCE) {
        info->distance = SQUARE(MAX_CREATURE_DISTANCE);
    }
    info->angle = angle - item->rot.y;
    info->enemy_facing = angle - g_LaraItem->rot.y + PHD_180;
    info->ahead = info->angle > -FRONT_ARC && info->angle < FRONT_ARC;
    info->bite = info->ahead && (g_LaraItem->pos.y > item->pos.y - STEP_L)
        && (g_LaraItem->pos.y < item->pos.y + STEP_L);
}

void Creature_Mood(ITEM_INFO *item, AI_INFO *info, bool violent)
{
    CREATURE_INFO *creature = item->data;
    if (!creature) {
        return;
    }

    LOT_INFO *LOT = &creature->LOT;
    if (LOT->node[item->box_number].search_number
        == (LOT->search_number | BLOCKED_SEARCH)) {
        LOT->required_box = NO_BOX;
    }

    if (creature->mood != MOOD_ATTACK && LOT->required_box != NO_BOX
        && !Box_ValidBox(item, info->zone_number, LOT->target_box)) {
        if (info->zone_number == info->enemy_zone) {
            creature->mood = MOOD_BORED;
        }
        LOT->required_box = NO_BOX;
    }

    MOOD_TYPE mood = creature->mood;

    if (g_LaraItem->hit_points <= 0) {
        creature->mood = MOOD_BORED;
    } else if (violent) {
        switch (mood) {
        case MOOD_ATTACK:
            if (info->zone_number != info->enemy_zone) {
                creature->mood = MOOD_BORED;
            }
            break;

        case MOOD_BORED:
        case MOOD_STALK:
            if (info->zone_number == info->enemy_zone) {
                creature->mood = MOOD_ATTACK;
            } else if (item->hit_status) {
                creature->mood = MOOD_ESCAPE;
            }
            break;

        case MOOD_ESCAPE:
            if (info->zone_number == info->enemy_zone) {
                creature->mood = MOOD_ATTACK;
            }
            break;
        }
    } else {
        switch (mood) {
        case MOOD_ATTACK:
            if (item->hit_status
                && (Random_GetControl() < ESCAPE_CHANCE
                    || info->zone_number != info->enemy_zone)) {
                creature->mood = MOOD_ESCAPE;
            } else if (info->zone_number != info->enemy_zone) {
                creature->mood = MOOD_BORED;
            }
            break;

        case MOOD_BORED:
        case MOOD_STALK:
            if (item->hit_status
                && (Random_GetControl() < ESCAPE_CHANCE
                    || info->zone_number != info->enemy_zone)) {
                creature->mood = MOOD_ESCAPE;
            } else if (info->zone_number == info->enemy_zone) {
                if (info->distance < ATTACK_RANGE
                    || (creature->mood == MOOD_STALK
                        && LOT->required_box == NO_BOX)) {
                    creature->mood = MOOD_ATTACK;
                } else {
                    creature->mood = MOOD_STALK;
                }
            }
            break;

        case MOOD_ESCAPE:
            if (info->zone_number == info->enemy_zone
                && Random_GetControl() < RECOVER_CHANCE) {
                creature->mood = MOOD_STALK;
            }
            break;
        }
    }

    if (mood != creature->mood) {
        if (mood == MOOD_ATTACK) {
            Box_TargetBox(LOT, LOT->target_box);
        }
        LOT->required_box = NO_BOX;
    }

    switch (creature->mood) {
    case MOOD_ATTACK:
        if (Random_GetControl() < g_Objects[item->object_number].smartness) {
            LOT->target.x = g_LaraItem->pos.x;
            LOT->target.y = g_LaraItem->pos.y;
            LOT->target.z = g_LaraItem->pos.z;
            LOT->required_box = g_LaraItem->box_number;
            if (LOT->fly && g_Lara.water_status == LWS_ABOVE_WATER) {
                const FRAME_INFO *const frame = Item_GetBestFrame(g_LaraItem);
                LOT->target.y += frame->bounds.min.y;
            }
        }
        break;

    case MOOD_BORED: {
        int box_number =
            LOT->node[Random_GetControl() * LOT->zone_count / 0x7FFF]
                .box_number;
        if (Box_ValidBox(item, info->zone_number, box_number)) {
            if (Box_StalkBox(item, box_number)) {
                Box_TargetBox(LOT, box_number);
                creature->mood = MOOD_STALK;
            } else if (LOT->required_box == NO_BOX) {
                Box_TargetBox(LOT, box_number);
            }
        }
        break;
    }

    case MOOD_STALK: {
        if (LOT->required_box == NO_BOX
            || !Box_StalkBox(item, LOT->required_box)) {
            int box_number =
                LOT->node[Random_GetControl() * LOT->zone_count / 0x7FFF]
                    .box_number;
            if (Box_ValidBox(item, info->zone_number, box_number)) {
                if (Box_StalkBox(item, box_number)) {
                    Box_TargetBox(LOT, box_number);
                } else if (LOT->required_box == NO_BOX) {
                    Box_TargetBox(LOT, box_number);
                    if (info->zone_number != info->enemy_zone) {
                        creature->mood = MOOD_BORED;
                    }
                }
            }
        }
        break;
    }

    case MOOD_ESCAPE: {
        int box_number =
            LOT->node[Random_GetControl() * LOT->zone_count / 0x7FFF]
                .box_number;
        if (Box_ValidBox(item, info->zone_number, box_number)
            && LOT->required_box == NO_BOX) {
            if (Box_EscapeBox(item, box_number)) {
                Box_TargetBox(LOT, box_number);
            } else if (
                info->zone_number == info->enemy_zone
                && Box_StalkBox(item, box_number)) {
                Box_TargetBox(LOT, box_number);
                creature->mood = MOOD_STALK;
            }
        }
        break;
    }
    }

    if (LOT->target_box == NO_BOX) {
        Box_TargetBox(LOT, item->box_number);
    }

    Box_CalculateTarget(&creature->target, item, &creature->LOT);
}

int16_t Creature_Turn(ITEM_INFO *item, int16_t maximum_turn)
{
    CREATURE_INFO *creature = item->data;
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

void Creature_Tilt(ITEM_INFO *item, int16_t angle)
{
    angle = angle * 4 - item->rot.z;
    if (angle < -MAX_TILT) {
        angle = -MAX_TILT;
    } else if (angle > MAX_TILT) {
        angle = MAX_TILT;
    }
    item->rot.z += angle;
}

void Creature_Head(ITEM_INFO *item, int16_t required)
{
    CREATURE_INFO *creature = item->data;
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
    ITEM_INFO *item, BITE_INFO *bite,
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
    return spawn(
        pos.x, pos.y, pos.z, item->speed, item->rot.y, item->room_number);
}

bool Creature_CheckBaddieOverlap(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    int32_t x = item->pos.x;
    int32_t y = item->pos.y;
    int32_t z = item->pos.z;
    int32_t radius = SQUARE(g_Objects[item->object_number].radius);

    int16_t link = g_RoomInfo[item->room_number].item_number;
    do {
        item = &g_Items[link];

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

void Creature_Collision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];

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
            Lara_Push(item, coll, coll->enable_spaz, 0);
        }
    }
}

bool Creature_Animate(int16_t item_num, int16_t angle, int16_t tilt)
{
    ITEM_INFO *item = &g_Items[item_num];
    CREATURE_INFO *creature = item->data;
    if (!creature) {
        return false;
    }
    LOT_INFO *LOT = &creature->LOT;

    XYZ_32 old = {
        .x = item->pos.x,
        .y = item->pos.y,
        .z = item->pos.z,
    };

    int32_t box_height = g_Boxes[item->box_number].height;

    int16_t *zone;
    if (LOT->fly) {
        zone = g_FlyZone[g_FlipStatus];
    } else if (LOT->step == STEP_L) {
        zone = g_GroundZone[g_FlipStatus];
    } else {
        zone = g_GroundZone2[g_FlipStatus];
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

    int16_t room_num = item->room_number;
    FLOOR_INFO *floor = Room_GetFloor(item->pos.x, y, item->pos.z, &room_num);
    int32_t height = g_Boxes[floor->box].height;
    int16_t next_box = LOT->node[floor->box].exit_box;
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
    if (floor->box == NO_BOX || zone[item->box_number] != zone[floor->box]
        || box_height - height > LOT->step || box_height - height < LOT->drop) {
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

        floor = Room_GetFloor(item->pos.x, y, item->pos.z, &room_num);
        height = g_Boxes[floor->box].height;
        next_box = LOT->node[floor->box].exit_box;
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

    int32_t radius = g_Objects[item->object_number].radius;

    if (pos_z < radius) {
        if (Box_BadFloor(
                x, y, z - radius, height, next_height, room_num, LOT)) {
            shift_z = radius - pos_z;
        }

        if (pos_x < radius) {
            if (Box_BadFloor(
                    x - radius, y, z, height, next_height, room_num, LOT)) {
                shift_x = radius - pos_x;
            } else if (
                !shift_z
                && Box_BadFloor(
                    x - radius, y, z - radius, height, next_height, room_num,
                    LOT)) {
                if (item->rot.y > -PHD_135 && item->rot.y < PHD_45) {
                    shift_z = radius - pos_z;
                } else {
                    shift_x = radius - pos_x;
                }
            }
        } else if (pos_x > WALL_L - radius) {
            if (Box_BadFloor(
                    x + radius, y, z, height, next_height, room_num, LOT)) {
                shift_x = WALL_L - radius - pos_x;
            } else if (
                !shift_z
                && Box_BadFloor(
                    x + radius, y, z - radius, height, next_height, room_num,
                    LOT)) {
                if (item->rot.y > -PHD_45 && item->rot.y < PHD_135) {
                    shift_z = radius - pos_z;
                } else {
                    shift_x = WALL_L - radius - pos_x;
                }
            }
        }
    } else if (pos_z > WALL_L - radius) {
        if (Box_BadFloor(
                x, y, z + radius, height, next_height, room_num, LOT)) {
            shift_z = WALL_L - radius - pos_z;
        }

        if (pos_x < radius) {
            if (Box_BadFloor(
                    x - radius, y, z, height, next_height, room_num, LOT)) {
                shift_x = radius - pos_x;
            } else if (
                !shift_z
                && Box_BadFloor(
                    x - radius, y, z + radius, height, next_height, room_num,
                    LOT)) {
                if (item->rot.y > -PHD_45 && item->rot.y < PHD_135) {
                    shift_x = radius - pos_x;
                } else {
                    shift_z = WALL_L - radius - pos_z;
                }
            }
        } else if (pos_x > WALL_L - radius) {
            if (Box_BadFloor(
                    x + radius, y, z, height, next_height, room_num, LOT)) {
                shift_x = WALL_L - radius - pos_x;
            } else if (
                !shift_z
                && Box_BadFloor(
                    x + radius, y, z + radius, height, next_height, room_num,
                    LOT)) {
                if (item->rot.y > -PHD_135 && item->rot.y < PHD_45) {
                    shift_x = WALL_L - radius - pos_x;
                } else {
                    shift_z = WALL_L - radius - pos_z;
                }
            }
        }
    } else if (pos_x < radius) {
        if (Box_BadFloor(
                x - radius, y, z, height, next_height, room_num, LOT)) {
            shift_x = radius - pos_x;
        }
    } else if (pos_x > WALL_L - radius) {
        if (Box_BadFloor(
                x + radius, y, z, height, next_height, room_num, LOT)) {
            shift_x = WALL_L - radius - pos_x;
        }
    }

    item->pos.x += shift_x;
    item->pos.z += shift_z;

    if (shift_x || shift_z) {
        floor = Room_GetFloor(item->pos.x, y, item->pos.z, &room_num);

        item->rot.y += angle;
        Creature_Tilt(item, tilt * 2);
    }

    if (Creature_CheckBaddieOverlap(item_num)) {
        item->pos.x = old.x;
        item->pos.y = old.y;
        item->pos.z = old.z;
        return true;
    }

    if (LOT->fly) {
        int32_t dy = creature->target.y - item->pos.y;
        if (dy > LOT->fly) {
            dy = LOT->fly;
        } else if (dy < -LOT->fly) {
            dy = -LOT->fly;
        }

        height = Room_GetHeight(floor, item->pos.x, y, item->pos.z);
        if (item->pos.y + dy > height) {
            if (item->pos.y > height) {
                item->pos.x = old.x;
                item->pos.z = old.z;
                dy = -LOT->fly;
            } else {
                dy = 0;
                item->pos.y = height;
            }
        } else {
            int32_t ceiling =
                Room_GetCeiling(floor, item->pos.x, y, item->pos.z);

            int32_t min_y =
                item->object_number == O_ALLIGATOR ? 0 : bounds->min.y;
            if (item->pos.y + min_y + dy < ceiling) {
                if (item->pos.y + min_y < ceiling) {
                    item->pos.x = old.x;
                    item->pos.z = old.z;
                    dy = LOT->fly;
                } else {
                    dy = 0;
                }
            }
        }

        item->pos.y += dy;
        floor = Room_GetFloor(item->pos.x, y, item->pos.z, &room_num);
        item->floor = Room_GetHeight(floor, item->pos.x, y, item->pos.z);

        angle = item->speed ? Math_Atan(item->speed, -dy) : 0;
        if (angle < item->rot.x - PHD_DEGREE) {
            item->rot.x -= PHD_DEGREE;
        } else if (angle > item->rot.x + PHD_DEGREE) {
            item->rot.x += PHD_DEGREE;
        } else {
            item->rot.x = angle;
        }
    } else {
        floor = Room_GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
        item->floor =
            Room_GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);

        if (item->pos.y > item->floor) {
            item->pos.y = item->floor;
        } else if (item->floor - item->pos.y > STEP_L / 4) {
            item->pos.y += STEP_L / 4;
        } else if (item->pos.y < item->floor) {
            item->pos.y = item->floor;
        }

        item->rot.x = 0;
    }

    if (item->room_number != room_num) {
        Item_NewRoom(item_num, room_num);
    }

    return true;
}

bool Creature_CanTargetEnemy(ITEM_INFO *item, AI_INFO *info)
{
    if (!info->ahead || info->distance >= CREATURE_SHOOT_RANGE) {
        return false;
    }

    GAME_VECTOR start;
    start.x = item->pos.x;
    start.y = item->pos.y - STEP_L * 3;
    start.z = item->pos.z;
    start.room_number = item->room_number;

    GAME_VECTOR target;
    target.x = g_LaraItem->pos.x;
    target.y = g_LaraItem->pos.y - STEP_L * 3;
    target.z = g_LaraItem->pos.z;

    return LOS_Check(&start, &target);
}

bool Creature_ShootAtLara(
    ITEM_INFO *item, int32_t distance, BITE_INFO *gun, int16_t extra_rotation,
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

    int16_t fx_num;
    if (hit) {
        fx_num = Creature_Effect(item, gun, Effect_GunShotHit);
    } else {
        fx_num = Creature_Effect(item, gun, Effect_GunShotMiss);
    }

    if (fx_num != NO_ITEM) {
        g_Effects[fx_num].rot.y += extra_rotation;
    }

    if (hit) {
        Lara_TakeDamage(damage, true);
    }

    return hit;
}

bool Creature_EnsureHabitat(
    int16_t item_num, int32_t *wh, const HYBRID_INFO *info)
{
    // Test the environment for a hybrid creature. Record the water height and
    // return whether or not a type conversion has taken place.
    ITEM_INFO *item = &g_Items[item_num];
    *wh = Room_GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_number);

    return item->object_number == info->land.id
        ? Creature_SwitchToWater(item_num, wh, info)
        : Creature_SwitchToLand(item_num, wh, info);
}

bool Creature_IsBoss(const int16_t item_num)
{
    const ITEM_INFO *const item = &g_Items[item_num];
    return Object_IsObjectType(item->object_number, g_BossObjects);
}

static bool Creature_SwitchToWater(
    int16_t item_num, int32_t *wh, const HYBRID_INFO *info)
{
    if (*wh == NO_HEIGHT) {
        return false;
    }

    ITEM_INFO *item = &g_Items[item_num];

    if (item->hit_points <= 0) {
        // Dead land creatures should remain in their pose permanently.
        return false;
    }

    // The land creature is alive and the room has been flooded. Switch to the
    // water creature.
    item->object_number = info->water.id;
    Item_SwitchToAnim(item, info->water.active_anim, 0);
    item->current_anim_state = g_Anims[item->anim_number].current_anim_state;
    item->goal_anim_state = item->current_anim_state;
    item->pos.y = *wh;

    return true;
}

static bool Creature_SwitchToLand(
    int16_t item_num, int32_t *wh, const HYBRID_INFO *info)
{
    if (*wh != NO_HEIGHT) {
        return false;
    }

    ITEM_INFO *item = &g_Items[item_num];

    // Switch to the land creature regardless of death state.
    item->object_number = info->land.id;
    item->rot.x = 0;

    if (item->hit_points > 0) {
        Item_SwitchToAnim(item, info->land.active_anim, 0);
        item->current_anim_state =
            g_Anims[item->anim_number].current_anim_state;
        item->goal_anim_state = item->current_anim_state;

    } else {
        Item_SwitchToAnim(item, info->land.death_anim, -1);
        item->current_anim_state = info->land.death_state;
        item->goal_anim_state = item->current_anim_state;

        int16_t room_num = item->room_number;
        FLOOR_INFO *floor =
            Room_GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
        item->floor =
            Room_GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
        item->pos.y = item->floor;

        if (item->room_number != room_num) {
            Item_NewRoom(item_num, room_num);
        }
    }

    return true;
}
