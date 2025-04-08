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
    const ITEM *const item, const BITE *const bite,
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

    const int32_t box_height = Box_GetBox(item->box_num)->height;

    const int16_t *const zone = Box_GetLotZone(lot);

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
    int32_t height = Box_GetBox(sector->box)->height;
    int16_t next_box = lot->node[sector->box].exit_box;
    int32_t next_height;
    if (next_box != NO_BOX) {
        next_height = Box_GetBox(next_box)->height;
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
        height = Box_GetBox(sector->box)->height;
        next_box = lot->node[sector->box].exit_box;
        if (next_box != NO_BOX) {
            next_height = Box_GetBox(next_box)->height;
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

bool Creature_IsBoss(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    return Object_IsType(item->object_id, g_BossObjects);
}

bool Creature_IsHostile(const ITEM *const item)
{
    return Object_IsType(item->object_id, g_EnemyObjects);
}

bool Creature_IsAlly(const ITEM *const item)
{
    return Object_IsType(item->object_id, g_AllyObjects);
}
