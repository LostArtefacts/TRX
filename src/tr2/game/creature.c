#include "game/creature.h"

#include "decomp/effects.h"
#include "game/box.h"
#include "game/items.h"
#include "game/lara/misc.h"
#include "game/los.h"
#include "game/lot.h"
#include "game/math.h"
#include "game/objects/common.h"
#include "game/objects/vars.h"
#include "game/random.h"
#include "game/room.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#define FRONT_ARC PHD_90
#define ESCAPE_CHANCE 2048
#define RECOVER_CHANCE 256
#define MAX_X_ROT (20 * PHD_DEGREE) // = 3640
#define MAX_TILT (3 * PHD_DEGREE) // = 546
#define MAX_HEAD_CHANGE (5 * PHD_DEGREE) // = 910
#define HEAD_ARC 0x3000 // = 12288
#define FLOAT_SPEED 32
#define TARGET_TOLERANCE 0x400000

#define CREATURE_ATTACK_RANGE SQUARE(WALL_L * 3) // = 0x900000 = 9437184
#define CREATURE_SHOOT_RANGE SQUARE(WALL_L * 8) // = 0x4000000 = 67108864

void __cdecl Creature_Initialise(const int16_t item_num)
{
    ITEM *const item = &g_Items[item_num];
    item->rot.y += (Random_GetControl() - PHD_90) >> 1;
    item->collidable = 1;
    item->data = 0;
}

int32_t __cdecl Creature_Activate(const int16_t item_num)
{
    ITEM *const item = &g_Items[item_num];
    if (item->status != IS_INVISIBLE) {
        return true;
    }

    if (!LOT_EnableBaddieAI(item_num, false)) {
        return false;
    }

    item->status = IS_ACTIVE;
    return true;
}

void __cdecl Creature_AIInfo(ITEM *const item, AI_INFO *const info)
{
    CREATURE *const creature = (CREATURE *)item->data;
    if (creature == NULL) {
        return;
    }

    switch (item->object_id) {
    case O_BANDIT_1:
    case O_BANDIT_2:
        Creature_GetBaddieTarget(creature->item_num, false);
        break;

    case O_MONK_1:
    case O_MONK_2:
        Creature_GetBaddieTarget(creature->item_num, true);
        break;

    default:
        creature->enemy = g_LaraItem;
        break;
    }

    ITEM *enemy = creature->enemy;
    if (enemy == NULL) {
        enemy = g_LaraItem;
    }

    int16_t *zone;
    if (creature->lot.fly) {
        zone = g_FlyZone[g_FlipStatus];
    } else {
        zone = g_GroundZone[BOX_ZONE(creature->lot.step)][g_FlipStatus];
    }

    {
        const ROOM *const r = &g_Rooms[item->room_num];
        const int32_t z_sector = (item->pos.z - r->pos.z) >> WALL_SHIFT;
        const int32_t x_sector = (item->pos.x - r->pos.x) >> WALL_SHIFT;
        item->box_num = r->sectors[z_sector + x_sector * r->size.z].box;
        info->zone_num = zone[item->box_num];
    }

    {
        const ROOM *const r = &g_Rooms[enemy->room_num];
        const int32_t z_sector = (enemy->pos.z - r->pos.z) >> WALL_SHIFT;
        const int32_t x_sector = (enemy->pos.x - r->pos.x) >> WALL_SHIFT;
        enemy->box_num = r->sectors[z_sector + x_sector * r->size.z].box;
        info->enemy_zone_num = zone[enemy->box_num];
    }

    if (((g_Boxes[enemy->box_num].overlap_index & creature->lot.block_mask)
         != 0)
        || (creature->lot.node[item->box_num].search_num
            == (creature->lot.search_num | BOX_BLOCKED_SEARCH))) {
        info->enemy_zone_num |= BOX_BLOCKED;
    }

    const OBJECT *const object = &g_Objects[item->object_id];
    const int32_t z = enemy->pos.z
        - ((object->pivot_length * Math_Cos(item->rot.y)) >> W2V_SHIFT)
        - item->pos.z;
    const int32_t x = enemy->pos.x
        - ((object->pivot_length * Math_Sin(item->rot.y)) >> W2V_SHIFT)
        - item->pos.x;
    PHD_ANGLE angle = Math_Atan(z, x);
    if (creature->enemy != NULL) {
        info->distance = SQUARE(x) + SQUARE(z);
    } else {
        info->distance = 0x7FFFFFFF;
    }

    info->angle = angle - item->rot.y;
    info->enemy_facing = angle - enemy->rot.y + PHD_180;
    info->ahead = info->angle > -FRONT_ARC && info->angle < FRONT_ARC;
    info->bite = info->ahead && enemy->hit_points > 0
        && ABS(enemy->pos.y - item->pos.y) <= STEP_L;
}

void __cdecl Creature_Mood(
    const ITEM *item, const AI_INFO *info, int32_t violent)
{
    CREATURE *const creature = item->data;
    if (creature == NULL) {
        return;
    }

    const LOT_INFO *const lot = &creature->lot;
    const ITEM *enemy = creature->enemy;
    if (creature->lot.node[item->box_num].search_num
        == (creature->lot.search_num | 0x8000)) {
        creature->lot.required_box = NO_BOX;
    }
    if (creature->mood != MOOD_ATTACK && creature->lot.required_box != NO_BOX
        && !Box_ValidBox(item, info->zone_num, creature->lot.target_box)) {
        if (info->zone_num == info->enemy_zone_num) {
            creature->mood = MOOD_BORED;
        }
        creature->lot.required_box = NO_BOX;
    }

    const MOOD_TYPE mood = creature->mood;
    if (enemy == NULL) {
        creature->mood = MOOD_BORED;
        enemy = g_LaraItem;
    } else if (enemy->hit_points <= 0) {
        creature->mood = MOOD_BORED;
    } else if (violent) {
        switch (mood) {
        case MOOD_BORED:
        case MOOD_STALK:
            if (info->zone_num == info->enemy_zone_num) {
                creature->mood = MOOD_ATTACK;
            } else if (item->hit_status != 0) {
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
            if (item->hit_status != 0
                && (Random_GetControl() < ESCAPE_CHANCE
                    || info->zone_num != info->enemy_zone_num)) {
                creature->mood = MOOD_ESCAPE;
            } else if (info->zone_num == info->enemy_zone_num) {
                if (info->distance < CREATURE_ATTACK_RANGE
                    || (creature->mood == MOOD_STALK
                        && creature->lot.required_box == NO_BOX)) {
                    creature->mood = MOOD_ATTACK;
                } else {
                    creature->mood = MOOD_STALK;
                }
            }
            break;

        case MOOD_ATTACK:
            if (item->hit_status != 0
                && (Random_GetControl() < ESCAPE_CHANCE
                    || info->zone_num != info->enemy_zone_num)) {
                creature->mood = MOOD_ESCAPE;
            } else if (info->zone_num != info->enemy_zone_num) {
                creature->mood = MOOD_BORED;
            }
            break;

        case MOOD_ESCAPE:
            if (info->zone_num == info->enemy_zone_num
                && Random_GetControl() < RECOVER_CHANCE) {
                creature->mood = MOOD_STALK;
            }
            break;
        }
    }

    if (mood != creature->mood) {
        if (mood == MOOD_ATTACK) {
            Box_TargetBox(&creature->lot, creature->lot.target_box);
        }
        creature->lot.required_box = NO_BOX;
    }

    switch (creature->mood) {
    case MOOD_BORED: {
        const int16_t box_num =
            lot->node[(creature->lot.zone_count * Random_GetControl()) >> 15]
                .box_num;
        if (Box_ValidBox(item, info->zone_num, box_num)) {
            if (Box_StalkBox(item, enemy, box_num) && creature->enemy != NULL
                && enemy->hit_points > 0) {
                Box_TargetBox(&creature->lot, box_num);
                creature->mood = MOOD_STALK;
            } else if (creature->lot.required_box == NO_BOX) {
                Box_TargetBox(&creature->lot, box_num);
            }
        }
        break;
    }

    case MOOD_ATTACK:
        creature->lot.target = enemy->pos;
        creature->lot.required_box = enemy->box_num;
        if (creature->lot.fly != 0 && g_Lara.water_status == LWS_ABOVE_WATER) {
            creature->lot.target.y += Item_GetBestFrame(enemy)->bounds.min_y;
        }
        break;

    case MOOD_ESCAPE: {
        const int16_t box_num =
            lot->node[(creature->lot.zone_count * Random_GetControl()) >> 15]
                .box_num;
        if (Box_ValidBox(item, info->zone_num, box_num)
            && creature->lot.required_box == NO_BOX) {
            if (Box_EscapeBox(item, enemy, box_num)) {
                Box_TargetBox(&creature->lot, box_num);
            } else if (
                info->zone_num == info->enemy_zone_num
                && Box_StalkBox(item, enemy, box_num)) {
                Box_TargetBox(&creature->lot, box_num);
                creature->mood = MOOD_STALK;
            }
        }
        break;
    }

    case MOOD_STALK: {
        int16_t box_num = creature->lot.required_box;
        if (box_num == NO_BOX || !Box_StalkBox(item, enemy, box_num)) {
            box_num =
                lot->node
                    [(creature->lot.zone_count * Random_GetControl()) >> 15]
                        .box_num;
            if (Box_ValidBox(item, info->zone_num, box_num)) {
                if (Box_StalkBox(item, enemy, box_num)) {
                    Box_TargetBox(&creature->lot, box_num);
                } else if (creature->lot.required_box == NO_BOX) {
                    Box_TargetBox(&creature->lot, box_num);
                    if (info->zone_num != info->enemy_zone_num) {
                        creature->mood = MOOD_BORED;
                    }
                }
            }
        }
        break;
    }
    }

    if (creature->lot.target_box == NO_BOX) {
        Box_TargetBox(&creature->lot, item->box_num);
    }
    Box_CalculateTarget(&creature->target, item, &creature->lot);
}

int32_t __cdecl Creature_CheckBaddieOverlap(const int16_t item_num)
{
    ITEM *item = &g_Items[item_num];

    const int32_t x = item->pos.x;
    const int32_t y = item->pos.y;
    const int32_t z = item->pos.z;
    const int32_t radius = SQUARE(g_Objects[item->object_id].radius);

    int16_t link = g_Rooms[item->room_num].item_num;
    while (link != NO_ITEM && link != item_num) {
        item = &g_Items[link];
        if (item != g_LaraItem && item->status == IS_ACTIVE
            && item->speed != 0) {
            const int32_t distance =
                (SQUARE(item->pos.z - z) + SQUARE(item->pos.y - y)
                 + SQUARE(item->pos.x - x));
            if (distance < radius) {
                return true;
            }
        }

        link = item->next_item;
    }

    return false;
}

void __cdecl Creature_Die(const int16_t item_num, const bool explode)
{
    ITEM *const item = &g_Items[item_num];

    if (item->object_id == O_DRAGON_FRONT) {
        item->hit_points = 0;
        return;
    }

    if (item->object_id == O_SKIDMAN) {
        if (explode) {
            Effect_ExplodingDeath(item_num, -1, 0);
        }
        const int16_t vehicle_item_num = (int16_t)(intptr_t)item->data;
        ITEM *const vehicle_item = &g_Items[vehicle_item_num];
        vehicle_item->hit_points = 0;
        return;
    }

    item->collidable = 0;
    item->hit_points = DONT_TARGET;
    if (explode) {
        Effect_ExplodingDeath(item_num, -1, 0);
        Item_Kill(item_num);
    } else {
        Item_RemoveActive(item_num);
    }

    if (g_Objects[item->object_id].intelligent) {
        LOT_DisableBaddieAI(item_num);
    }
    item->flags |= IF_ONE_SHOT;

    if (item->killed) {
        item->next_active = g_PrevItemActive;
        g_PrevItemActive = item_num;
    }

    if (g_Objects[item->object_id].intelligent) {
        int16_t pickup_num = item->carried_item;
        while (pickup_num != NO_ITEM) {
            ITEM *const pickup = &g_Items[pickup_num];
            pickup->pos = item->pos;
            Item_NewRoom(pickup_num, item->room_num);
            pickup_num = pickup->carried_item;
        }
    }
}

int32_t __cdecl Creature_Animate(
    const int16_t item_num, const int16_t angle, const int16_t tilt)
{
    ITEM *const item = &g_Items[item_num];
    const CREATURE *const creature = item->data;
    const OBJECT *const object = &g_Objects[item->object_id];
    if (creature == NULL) {
        return false;
    }

    const LOT_INFO *const lot = &creature->lot;
    const XYZ_32 old = item->pos;

    int16_t *zone;
    if (lot->fly) {
        zone = g_FlyZone[g_FlipStatus];
    } else {
        zone = g_GroundZone[BOX_ZONE(lot->step)][g_FlipStatus];
    }

    if (!object->water_creature) {
        int16_t room_num = item->room_num;
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
        if (room_num != item->room_num) {
            Item_NewRoom(item_num, room_num);
        }
    }

    Item_Animate(item);
    if (item->status == IS_DEACTIVATED) {
        Creature_Die(item_num, false);
        return false;
    }

    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
    int32_t y = item->pos.y + bounds->min_y;

    int16_t room_num = item->room_num;
    Room_GetSector(old.x, y, old.z, &room_num);
    const SECTOR *sector =
        Room_GetSector(item->pos.x, y, item->pos.z, &room_num);
    int32_t height = g_Boxes[sector->box].height;
    int16_t next_box = lot->node[sector->box].exit_box;
    int32_t next_height =
        next_box != NO_BOX ? g_Boxes[next_box].height : height;

    const int32_t box_height = g_Boxes[item->box_num].height;
    if (sector->box == NO_BOX || zone[item->box_num] != zone[sector->box]
        || box_height - height > lot->step || box_height - height < lot->drop) {
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
        height = g_Boxes[sector->box].height;
        next_box = lot->node[sector->box].exit_box;
        next_height = next_box != NO_BOX ? g_Boxes[next_box].height : height;
    }

    const int32_t x = item->pos.x;
    const int32_t z = item->pos.z;
    const int32_t pos_x = x & (WALL_L - 1);
    const int32_t pos_z = z & (WALL_L - 1);
    int32_t shift_x = 0;
    int32_t shift_z = 0;
    const int32_t radius = object->radius;

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
                if (item->rot.y > -PHD_135 && item->rot.y < PHD_45) {
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
                if (item->rot.y > -PHD_45 && item->rot.y < PHD_135) {
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
                if (item->rot.y > -PHD_45 && item->rot.y < PHD_135) {
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
                if (item->rot.y > -PHD_135 && item->rot.y < PHD_45) {
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
        if (tilt != 0) {
            Creature_Tilt(item, tilt * 2);
        }
    }

    if (Creature_CheckBaddieOverlap(item_num)) {
        item->pos = old;
        return true;
    }

    if (lot->fly) {
        int32_t dy = creature->target.y - item->pos.y;
        CLAMP(dy, -lot->fly, lot->fly);

        height = Room_GetHeight(sector, item->pos.x, y, item->pos.z);
        if (item->pos.y + dy <= height) {
            const int32_t ceiling =
                Room_GetCeiling(sector, item->pos.x, y, item->pos.z);
            const int32_t min_y =
                item->object_id == O_SHARK ? 128 : bounds->min_y;
            if (item->pos.y + min_y + dy < ceiling) {
                if (item->pos.y + min_y < ceiling) {
                    item->pos.x = old.x;
                    item->pos.z = old.z;
                    dy = lot->fly;
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
            dy = -lot->fly;
        }

        item->pos.y += dy;
        sector = Room_GetSector(item->pos.x, y, item->pos.z, &room_num);
        item->floor = Room_GetHeight(sector, item->pos.x, y, item->pos.z);

        int16_t angle = item->speed != 0 ? Math_Atan(item->speed, -dy) : 0;
        CLAMP(angle, -MAX_X_ROT, MAX_X_ROT);

        if (angle < item->rot.x - PHD_DEGREE) {
            item->rot.x -= PHD_DEGREE;
        } else if (angle > item->rot.x + PHD_DEGREE) {
            item->rot.x += PHD_DEGREE;
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

    if (!object->water_creature) {
        Room_GetSector(
            item->pos.x, item->pos.y - (STEP_L * 2), item->pos.z, &room_num);
        if (g_Rooms[room_num].flags & RF_UNDERWATER) {
            item->hit_points = 0;
        }
    }

    if (item->room_num != room_num) {
        Item_NewRoom(item_num, room_num);
    }
    return true;
}

int16_t __cdecl Creature_Turn(ITEM *const item, int16_t max_turn)
{
    const CREATURE *const creature = item->data;
    if (creature == NULL || item->speed == 0 || max_turn == 0) {
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

void __cdecl Creature_Tilt(ITEM *const item, int16_t angle)
{
    angle = 4 * angle - item->rot.z;
    CLAMP(angle, -MAX_TILT, MAX_TILT);
    item->rot.z += angle;
}

void __cdecl Creature_Head(ITEM *item, int16_t required)
{
    CREATURE *const creature = item->data;
    if (creature == NULL) {
        return;
    }

    int16_t change = required - creature->head_rotation;
    CLAMP(change, -MAX_HEAD_CHANGE, MAX_HEAD_CHANGE);

    creature->head_rotation += change;
    CLAMP(creature->head_rotation, -HEAD_ARC, HEAD_ARC);
}

void __cdecl Creature_Neck(ITEM *const item, const int16_t required)
{
    CREATURE *const creature = item->data;
    if (creature == NULL) {
        return;
    }

    int16_t change = required - creature->neck_rotation;
    CLAMP(change, -MAX_HEAD_CHANGE, MAX_HEAD_CHANGE);

    creature->neck_rotation += change;
    CLAMP(creature->neck_rotation, -HEAD_ARC, HEAD_ARC);
}

void __cdecl Creature_Float(const int16_t item_num)
{
    ITEM *const item = &g_Items[item_num];

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

void __cdecl Creature_Underwater(ITEM *const item, const int32_t depth)
{
    const int32_t wh = Room_GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_num);
    if (item->pos.y >= wh + depth) {
        return;
    }

    item->pos.y = wh + depth;
    if (item->rot.x > 2 * PHD_DEGREE) {
        item->rot.x -= 2 * PHD_DEGREE;
    } else {
        CLAMPG(item->rot.x, 0);
    }
}

int16_t __cdecl Creature_Effect(
    const ITEM *const item, const BITE *const bite,
    int16_t(__cdecl *const spawn)(
        int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
        int16_t room_num))
{
    XYZ_32 pos = bite->pos;
    Collide_GetJointAbsPosition(item, &pos, bite->mesh_num);
    return (*spawn)(
        pos.x, pos.y, pos.z, item->speed, item->rot.y, item->room_num);
}

int32_t __cdecl Creature_Vault(
    const int16_t item_num, const int16_t angle, int32_t vault,
    const int32_t shift)
{
    ITEM *const item = &g_Items[item_num];
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
            item->rot.y = -PHD_90;
            item->pos.x = (old_x_sector << WALL_SHIFT) + shift;
        } else {
            item->rot.y = PHD_90;
            item->pos.x = (x_sector << WALL_SHIFT) - shift;
        }
    } else if (old_x_sector == x_sector) {
        if (old_z_sector >= z_sector) {
            item->rot.y = PHD_180;
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

void __cdecl Creature_Kill(
    ITEM *const item, const int32_t kill_anim, const int32_t kill_state,
    const int32_t lara_kill_state)
{
    item->anim_num = g_Objects[item->object_id].anim_idx + kill_anim;
    item->frame_num = g_Anims[item->anim_num].frame_base;
    item->current_anim_state = kill_state;

    g_LaraItem->anim_num = g_Objects[O_LARA_EXTRA].anim_idx;
    g_LaraItem->frame_num = g_Anims[g_LaraItem->anim_num].frame_base;
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

void __cdecl Creature_GetBaddieTarget(
    const int16_t item_num, const int32_t goody)
{
    ITEM *const item = &g_Items[item_num];
    CREATURE *const creature = item->data;

    ITEM *best_item = NULL;
    int32_t best_distance = INT32_MAX;
    for (int32_t i = 0; i < NUM_SLOTS; i++) {
        const int16_t target_item_num = g_BaddieSlots[i].item_num;
        if (target_item_num == NO_ITEM || target_item_num == item_num) {
            continue;
        }

        ITEM *target = &g_Items[target_item_num];
        const GAME_OBJECT_ID object_id = target->object_id;
        if (goody && object_id != O_BANDIT_1 && object_id != O_BANDIT_2) {
            continue;
        } else if (!goody && object_id != O_MONK_1 && object_id != O_MONK_2) {
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

    if (best_item == NULL) {
        if (!goody || g_IsMonkAngry) {
            creature->enemy = g_LaraItem;
        } else {
            creature->enemy = NULL;
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
    if (target == NULL || target->status != IS_ACTIVE) {
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

void __cdecl Creature_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = &g_Items[item_num];
    if (!Item_TestBoundsCollide(item, lara_item, coll->radius)) {
        return;
    }

    if (!Collide_TestCollision(item, lara_item)) {
        return;
    }

    if (coll->enable_baddie_push && g_Lara.water_status != LWS_UNDERWATER
        && g_Lara.water_status != LWS_SURFACE) {
        Lara_Push(item, lara_item, coll, coll->enable_spaz, false);
    }
}

int32_t __cdecl Creature_CanTargetEnemy(
    const ITEM *const item, const AI_INFO *const info)
{
    const CREATURE *const creature = item->data;
    const ITEM *const enemy = creature->enemy;
    if (enemy->hit_points <= 0 || !info->ahead
        || info->distance >= CREATURE_SHOOT_RANGE) {
        return 0;
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

bool Creature_IsEnemy(const ITEM *const item)
{
    return Object_IsObjectType(item->object_id, g_EnemyObjects)
        || (g_IsMonkAngry
            && (item->object_id == O_MONK_1 || item->object_id == O_MONK_2));
}

bool Creature_IsAlly(const ITEM *const item)
{
    return Object_IsObjectType(item->object_id, g_AllyObjects);
}
