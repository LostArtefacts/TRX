#include "game/lara/misc.h"

#include "game/effects.h"
#include "game/input.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/lara/const.h"
#include "game/random.h"
#include "game/rooms.h"

#define M_CLIMB_SHIFT 70
#define M_CLIMB_HANG 900

static int32_t M_TestClimb(
    int32_t x, int32_t y, int32_t z, int32_t x_front, int32_t z_front,
    int32_t item_height, int16_t item_room, int32_t *shift);

static int32_t M_TestClimb(
    const int32_t x, const int32_t y, const int32_t z, const int32_t x_front,
    const int32_t z_front, const int32_t item_height, const int16_t item_room,
    int32_t *const shift)
{
#if TR_VERSION == 1
    return 0;
#else
    *shift = 0;
    bool hang = true;
    if (!Lara_GetLaraInfo()->climb_status) {
        return 0;
    }

    const SECTOR *sector;
    int32_t height;
    int32_t ceiling;

    int16_t room_num = item_room;
    sector = Room_GetSector(x, y - 128, z, &room_num);
    height = Room_GetHeight(sector, x, y, z);
    if (height == NO_HEIGHT) {
        return 0;
    }

    height -= y + item_height + STEP_L / 2;
    if (height < -M_CLIMB_SHIFT) {
        return 0;
    }
    if (height < 0) {
        *shift = height;
    }

    ceiling = Room_GetCeiling(sector, x, y, z) - y;
    if (ceiling > M_CLIMB_SHIFT) {
        return 0;
    }
    if (ceiling > 0) {
        if (*shift) {
            return 0;
        }
        *shift = ceiling;
    }

    if (item_height + height < M_CLIMB_HANG) {
        hang = false;
    }

    const int32_t x2 = x + x_front;
    const int32_t z2 = z + z_front;
    sector = Room_GetSector(x2, y, z2, &room_num);
    height = Room_GetHeight(sector, x2, y, z2);
    if (height != NO_HEIGHT) {
        height -= y;
    }

    if (height > M_CLIMB_SHIFT) {
        ceiling = Room_GetCeiling(sector, x2, y, z2) - y;
        if (ceiling >= LARA_CLIMB_HEIGHT) {
            return 1;
        }

        if (ceiling > LARA_CLIMB_HEIGHT - M_CLIMB_SHIFT) {
            if (*shift > 0) {
                return hang ? -1 : 0;
            }
            *shift = ceiling - LARA_CLIMB_HEIGHT;
            return 1;
        }

        if (ceiling > 0) {
            return hang ? -1 : 0;
        }

        if (ceiling > -M_CLIMB_SHIFT && hang && *shift <= 0) {
            if (*shift > ceiling) {
                *shift = ceiling;
            }

            return -1;
        }

        return 0;
    }

    if (height > 0) {
        if (*shift < 0) {
            return 0;
        }
        if (height > *shift) {
            *shift = height;
        }
    }

    room_num = item_room;
    sector = Room_GetSector(x, item_height + y, z, &room_num);
    sector = Room_GetSector(x2, item_height + y, z2, &room_num);
    ceiling = Room_GetCeiling(sector, x2, item_height + y, z2);
    if (ceiling == NO_HEIGHT) {
        return 1;
    }

    ceiling -= y;
    if (ceiling <= height) {
        return 1;
    }

    if (ceiling >= LARA_CLIMB_HEIGHT) {
        return 1;
    }

    if (ceiling > LARA_CLIMB_HEIGHT - M_CLIMB_SHIFT) {
        if (*shift > 0) {
            return hang ? -1 : 0;
        }
        *shift = ceiling - LARA_CLIMB_HEIGHT;
        return 1;
    }

    return hang ? -1 : 0;
#endif
}

void Lara_TouchLava(void)
{
    ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (lara_item->hit_points < 0 || lara_info->water_status == LWS_CHEAT) {
        return;
    }

    int16_t room_num = lara_item->room_num;
    const SECTOR *const sector = Room_GetSector(
        lara_item->pos.x, MAX_HEIGHT, lara_item->pos.z, &room_num);
    const int32_t height =
        Room_GetHeight(sector, lara_item->pos.x, MAX_HEIGHT, lara_item->pos.z);
    if (lara_item->floor != height) {
        return;
    }

    lara_item->hit_points = -1;
    lara_item->hit_status = 1;

    if (lara_info->water_status != LWS_ABOVE_WATER) {
        return;
    }

    const OBJECT *const obj = Object_Get(O_FLAME);
    for (int32_t i = 0; i < 10; i++) {
        const int16_t effect_num = Effect_Create(lara_item->room_num);
        if (effect_num != NO_EFFECT) {
            EFFECT *const effect = Effect_Get(effect_num);
            effect->object_id = O_FLAME;
            effect->frame_num = obj->mesh_count * Random_GetControl() / 0x7FFF;
            effect->counter = -1 - 24 * Random_GetControl() / 0x7FFF;
        }
    }
}

int16_t Lara_FloorFront(
    const ITEM *const item, const int16_t ang, const int32_t dist)
{
    const int32_t x = item->pos.x + ((dist * Math_Sin(ang)) >> W2V_SHIFT);
    const int32_t y = item->pos.y - LARA_HEIGHT;
    const int32_t z = item->pos.z + ((dist * Math_Cos(ang)) >> W2V_SHIFT);
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
    int32_t height = Room_GetHeight(sector, x, y, z);
    if (height != NO_HEIGHT) {
        height -= item->pos.y;
    }
    return height;
}

void Lara_GetCollisionInfo(const ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    coll->facing = lara->move_angle;
    Collide_GetCollisionInfo(
        coll, item->pos.x, item->pos.y, item->pos.z, item->room_num,
        LARA_HEIGHT);
}

void Lara_UpdateRoomToHeight(const int32_t height)
{
    ITEM *const lara_item = Lara_GetItem();
    const int32_t x = lara_item->pos.x;
    const int32_t y = height + lara_item->pos.y;
    const int32_t z = lara_item->pos.z;

    int16_t room_num = lara_item->room_num;
    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
    lara_item->floor = Room_GetHeight(sector, x, y, z);

    const int16_t item_num = Item_GetIndex(lara_item);
    Item_UpdateRoom(item_num, room_num);
}

void Lara_ShiftCol(COLL_INFO *const coll)
{
    ITEM *const lara_item = Lara_GetItem();
    lara_item->pos.x += coll->shift.x;
    lara_item->pos.y += coll->shift.y;
    lara_item->pos.z += coll->shift.z;
    coll->shift.z = 0;
    coll->shift.y = 0;
    coll->shift.x = 0;
}

int32_t Lara_GetWaterDepth(
    const int32_t x, const int32_t y, const int32_t z, int16_t room_num)
{
    const ROOM *room = Room_Get(room_num);
    const SECTOR *sector;

    while (true) {
        int32_t z_sector = (z - room->pos.z) >> WALL_SHIFT;
        int32_t x_sector = (x - room->pos.x) >> WALL_SHIFT;

        if (z_sector <= 0) {
            z_sector = 0;
            if (x_sector < 1) {
                x_sector = 1;
            } else if (x_sector > room->size.x - 2) {
                x_sector = room->size.x - 2;
            }
        } else if (z_sector >= room->size.z - 1) {
            z_sector = room->size.z - 1;
            if (x_sector < 1) {
                x_sector = 1;
            } else if (x_sector > room->size.x - 2) {
                x_sector = room->size.x - 2;
            }
        } else if (x_sector < 0) {
            x_sector = 0;
        } else if (x_sector >= room->size.x) {
            x_sector = room->size.x - 1;
        }

        sector = Room_GetUnitSector(room, x_sector, z_sector);
        if (sector->portal_room.wall == NO_ROOM) {
            break;
        }
        room_num = sector->portal_room.wall;
        room = Room_Get(room_num);
    }

    if ((room->flags & RF_UNDERWATER) != 0) {
        while (sector->portal_room.sky != NO_ROOM) {
            room = Room_Get(sector->portal_room.sky);
            if ((room->flags & RF_UNDERWATER) == 0) {
                const int32_t water_height = sector->ceiling.height;
                sector = Room_GetSector(x, y, z, &room_num);
                return Room_GetHeight(sector, x, y, z) - water_height;
            }
            sector = Room_GetWorldSector(room, x, z);
        }
        return 0x7FFF;
    }

    while (sector->portal_room.pit != NO_ROOM) {
        room = Room_Get(sector->portal_room.pit);
        if ((room->flags & RF_UNDERWATER) != 0) {
            const int32_t water_height = sector->floor.height;
            sector = Room_GetSector(x, y, z, &room_num);
            return Room_GetHeight(sector, x, y, z) - water_height;
        }
        sector = Room_GetWorldSector(room, x, z);
    }
    return NO_HEIGHT;
}

bool Lara_TestWall(
    const ITEM *const item, const int32_t front, const int32_t right,
    const int32_t down)
{
    int32_t x = item->pos.x;
    int32_t y = item->pos.y + down;
    int32_t z = item->pos.z;

    const DIRECTION dir = Math_GetDirection(item->rot.y);
    switch (dir) {
    case DIR_NORTH:
        x -= right;
        break;
    case DIR_EAST:
        z -= right;
        break;
    case DIR_SOUTH:
        x += right;
        break;
    case DIR_WEST:
        z += right;
        break;
    default:
        break;
    }

    int16_t room_num = item->room_num;
    Room_GetSector(x, y, z, &room_num);

    switch (dir) {
    case DIR_NORTH:
        z += front;
        break;
    case DIR_EAST:
        x += front;
        break;
    case DIR_SOUTH:
        z -= front;
        break;
    case DIR_WEST:
        x -= front;
        break;
    default:
        break;
    }

    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
    const int32_t height = Room_GetHeight(sector, x, y, z);
    const int32_t ceiling = Room_GetCeiling(sector, x, y, z);
    if (height != NO_HEIGHT && height - y > 0 && ceiling - y < 0) {
        return false;
    }
    return true;
}

int32_t Lara_TestClimbPos(
    const ITEM *const item, const int32_t front, const int32_t right,
    const int32_t origin, const int32_t height, int32_t *const shift)
{
    const int32_t y = item->pos.y + origin;
    int32_t x;
    int32_t z;
    int32_t x_front = 0;
    int32_t z_front = 0;

    switch (Math_GetDirection(item->rot.y)) {
    case DIR_NORTH:
        x = item->pos.x + right;
        z = item->pos.z + front;
        z_front = 2;
        break;

    case DIR_EAST:
        x = item->pos.x + front;
        z = item->pos.z - right;
        x_front = 2;
        break;

    case DIR_SOUTH:
        x = item->pos.x - right;
        z = item->pos.z - front;
        z_front = -2;
        break;

    case DIR_WEST:
        x = item->pos.x - front;
        z = item->pos.z + right;
        x_front = -2;
        break;

    default:
        x = front;
        z = front;
        break;
    }

    return M_TestClimb(
        x, y, z, x_front, z_front, height, item->room_num, shift);
}

bool Lara_IsM16Active(void)
{
#if TR_VERSION == 1
    return false;
#else
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->gun_item_num == NO_ITEM || lara->gun_type != LGT_M16) {
        return false;
    }

    const ITEM *const item = Item_Get(lara->gun_item_num);
    return item->current_anim_state == 0 || item->current_anim_state == 2
        || item->current_anim_state == 4;
#endif
}
