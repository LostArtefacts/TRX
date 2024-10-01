#include "game/room.h"

#include "game/box.h"
#include "game/camera.h"
#include "game/items.h"
#include "game/lara/misc.h"
#include "game/lot.h"
#include "game/math.h"
#include "game/music.h"
#include "game/shell.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#include <assert.h>

int16_t Room_GetIndexFromPos(const int32_t x, const int32_t y, const int32_t z)
{
    // TODO: merge this to Room_FindByPos
    const int32_t room_num = Room_FindByPos(x, y, z);
    if (room_num == NO_ROOM_NEG) {
        return NO_ROOM;
    }
    return room_num;
}

int32_t __cdecl Room_FindByPos(
    const int32_t x, const int32_t y, const int32_t z)
{
    for (int32_t i = 0; i < g_RoomCount; i++) {
        const ROOM *const room = &g_Rooms[i];
        const int32_t x1 = room->pos.x + WALL_L;
        const int32_t x2 = room->pos.x + (room->size.x - 1) * WALL_L;
        const int32_t y1 = room->max_ceiling;
        const int32_t y2 = room->min_floor;
        const int32_t z1 = room->pos.z + WALL_L;
        const int32_t z2 = room->pos.z + (room->size.z - 1) * WALL_L;
        if (x >= x1 && x < x2 && y >= y1 && y <= y2 && z >= z1 && z < z2) {
            return i;
        }
    }

    return NO_ROOM_NEG;
}

int32_t __cdecl Room_FindGridShift(int32_t src, const int32_t dst)
{
    const int32_t src_w = src >> WALL_SHIFT;
    const int32_t dst_w = dst >> WALL_SHIFT;
    if (src_w == dst_w) {
        return 0;
    }

    src &= WALL_L - 1;
    if (dst_w > src_w) {
        return WALL_L - (src - 1);
    } else {
        return -(src + 1);
    }
}

void __cdecl Room_GetNearbyRooms(
    const int32_t x, const int32_t y, const int32_t z, const int32_t r,
    const int32_t h, const int16_t room_num)
{
    g_DrawRoomsArray[0] = room_num;
    g_DrawRoomsCount = 1;

    Room_GetNewRoom(r + x, y, r + z, room_num);
    Room_GetNewRoom(x - r, y, r + z, room_num);
    Room_GetNewRoom(r + x, y, z - r, room_num);
    Room_GetNewRoom(x - r, y, z - r, room_num);
    Room_GetNewRoom(r + x, y - h, r + z, room_num);
    Room_GetNewRoom(x - r, y - h, r + z, room_num);
    Room_GetNewRoom(r + x, y - h, z - r, room_num);
    Room_GetNewRoom(x - r, y - h, z - r, room_num);
}

void __cdecl Room_GetNewRoom(
    const int32_t x, const int32_t y, const int32_t z, int16_t room_num)
{
    Room_GetSector(x, y, z, &room_num);

    for (int32_t i = 0; i < g_DrawRoomsCount; i++) {
        if (g_DrawRoomsArray[i] == room_num) {
            return;
        }
    }

    // TODO: fix crash when trying to draw too many rooms
    g_DrawRoomsArray[g_DrawRoomsCount++] = room_num;
}

int16_t __cdecl Room_GetTiltType(
    const SECTOR *sector, const int32_t x, const int32_t y, const int32_t z)
{
    while (sector->pit_room != NO_ROOM) {
        const ROOM *const room = &g_Rooms[sector->pit_room];
        const int32_t z_sector = (z - room->pos.z) >> WALL_SHIFT;
        const int32_t x_sector = (x - room->pos.x) >> WALL_SHIFT;
        sector = &room->sectors[z_sector + x_sector * room->size.z];
    }

    if ((y + 512 >= (sector->floor << 8)) && sector->idx != 0) {
        const int16_t *fd = &g_FloorData[sector->idx];
        if (FLOORDATA_TYPE(fd[0]) == FT_TILT) {
            return fd[1];
        }
    }

    return 0;
}

SECTOR *__cdecl Room_GetSector(
    const int32_t x, const int32_t y, const int32_t z, int16_t *const room_num)
{
    SECTOR *sector = NULL;

    while (true) {
        const ROOM *r = &g_Rooms[*room_num];
        int32_t z_sector = (z - r->pos.z) >> WALL_SHIFT;
        int32_t x_sector = (x - r->pos.x) >> WALL_SHIFT;

        if (z_sector <= 0) {
            z_sector = 0;
            if (x_sector < 1) {
                x_sector = 1;
            } else if (x_sector > r->size.x - 2) {
                x_sector = r->size.x - 2;
            }
        } else if (z_sector >= r->size.z - 1) {
            z_sector = r->size.z - 1;
            if (x_sector < 1) {
                x_sector = 1;
            } else if (x_sector > r->size.x - 2) {
                x_sector = r->size.x - 2;
            }
        } else if (x_sector < 0) {
            x_sector = 0;
        } else if (x_sector >= r->size.x) {
            x_sector = r->size.x - 1;
        }

        sector = &r->sectors[z_sector + x_sector * r->size.z];
        const int16_t door = Room_GetDoor(sector);
        if (door == NO_ROOM) {
            break;
        }
        *room_num = door;
    }

    assert(sector != NULL);

    if (y >= (sector->floor << 8)) {
        while (sector->pit_room != NO_ROOM) {
            *room_num = sector->pit_room;
            const ROOM *const r = &g_Rooms[*room_num];
            const int32_t z_sector = ((z - r->pos.z) >> WALL_SHIFT);
            const int32_t x_sector = ((x - r->pos.x) >> WALL_SHIFT);
            sector = &r->sectors[z_sector + x_sector * r->size.z];
            if (y < (sector->floor << 8)) {
                break;
            }
        }
    } else if (y < (sector->ceiling << 8)) {
        while (sector->sky_room != NO_ROOM) {
            *room_num = sector->sky_room;
            const ROOM *const r = &g_Rooms[sector->sky_room];
            const int32_t z_sector = (z - r->pos.z) >> WALL_SHIFT;
            const int32_t x_sector = (x - r->pos.x) >> WALL_SHIFT;
            sector = &r->sectors[z_sector + x_sector * r->size.z];
            if (y >= (sector->ceiling << 8)) {
                break;
            }
        }
    }

    return sector;
}

int32_t __cdecl Room_GetWaterHeight(
    const int32_t x, const int32_t y, const int32_t z, int16_t room_num)
{
    const SECTOR *sector = NULL;
    const ROOM *r = NULL;

    do {
        r = &g_Rooms[room_num];
        int32_t z_sector = (z - r->pos.z) >> WALL_SHIFT;
        int32_t x_sector = (x - r->pos.x) >> WALL_SHIFT;

        if (z_sector <= 0) {
            z_sector = 0;
            if (x_sector < 1) {
                x_sector = 1;
            } else if (x_sector > r->size.x - 2) {
                x_sector = r->size.x - 2;
            }
        } else if (z_sector >= r->size.z - 1) {
            z_sector = r->size.z - 1;
            if (x_sector < 1) {
                x_sector = 1;
            } else if (x_sector > r->size.x - 2) {
                x_sector = r->size.x - 2;
            }
        } else if (x_sector < 0) {
            x_sector = 0;
        } else if (x_sector >= r->size.x) {
            x_sector = r->size.x - 1;
        }

        sector = &r->sectors[z_sector + x_sector * r->size.z];
        room_num = Room_GetDoor(sector);
    } while (room_num != NO_ROOM);

    if (r->flags & RF_UNDERWATER) {
        while (sector->sky_room != NO_ROOM) {
            r = &g_Rooms[sector->sky_room];
            if (!(r->flags & RF_UNDERWATER)) {
                break;
            }
            const int32_t z_sector = (z - r->pos.z) >> WALL_SHIFT;
            const int32_t x_sector = (x - r->pos.x) >> WALL_SHIFT;
            sector = &r->sectors[z_sector + x_sector * r->size.z];
        }
        return sector->ceiling << 8;
    } else {
        while (sector->pit_room != NO_ROOM) {
            r = &g_Rooms[sector->pit_room];
            if (r->flags & RF_UNDERWATER) {
                return sector->floor << 8;
            }
            const int32_t z_sector = (z - r->pos.z) >> WALL_SHIFT;
            const int32_t x_sector = (x - r->pos.x) >> WALL_SHIFT;
            sector = &r->sectors[z_sector + x_sector * r->size.z];
        }
        return NO_HEIGHT;
    }
}

int32_t __cdecl Room_GetHeight(
    const SECTOR *sector, const int32_t x, const int32_t y, const int32_t z)
{
    g_HeightType = 0;
    g_TriggerIndex = NULL;

    while (sector->pit_room != NO_ROOM) {
        const ROOM *const r = &g_Rooms[sector->pit_room];
        const int32_t z_sector = (z - r->pos.z) >> WALL_SHIFT;
        const int32_t x_sector = (x - r->pos.x) >> WALL_SHIFT;
        sector = &r->sectors[z_sector + x_sector * r->size.z];
    }

    int32_t height = sector->floor << 8;
    if (g_GF_NoFloor && g_GF_NoFloor == height) {
        height = 0x4000;
    }

    if (!sector->idx) {
        return height;
    }

    int16_t *fd = &g_FloorData[sector->idx];
    while (true) {
        const int16_t fd_cmd = *fd++;

        switch (FLOORDATA_TYPE(fd_cmd)) {
        case FT_DOOR:
        case FT_ROOF:
            fd++;
            break;

        case FT_TILT: {
            const int32_t x_off = *fd >> 8;
            const int32_t y_off = (int8_t)*fd;
            fd++;

            if (!g_IsChunkyCamera || (ABS(x_off) <= 2 && ABS(y_off) <= 2)) {
                if (ABS(x_off) > 2 || ABS(y_off) > 2) {
                    g_HeightType = HT_BIG_SLOPE;
                } else {
                    g_HeightType = HT_SMALL_SLOPE;
                }

                if (x_off < 0) {
                    height -= (x_off * (z & (WALL_L - 1))) >> 2;
                } else {
                    height += (x_off * ((WALL_L - 1 - z) & (WALL_L - 1))) >> 2;
                }

                if (y_off < 0) {
                    height -= (y_off * (x & (WALL_L - 1))) >> 2;
                } else {
                    height += (y_off * ((WALL_L - 1 - x) & (WALL_L - 1))) >> 2;
                }
            }
            break;
        }

        case FT_TRIGGER:
            if (g_TriggerIndex == NULL) {
                g_TriggerIndex = fd - 1;
            }
            fd++;

            int16_t trigger;
            do {
                trigger = *fd++;
                switch (TRIGGER_TYPE(trigger)) {
                case TO_OBJECT:
                    const int16_t value = TRIGGER_VALUE(trigger);
                    const ITEM *const item = &g_Items[value];
                    const OBJECT *const object = &g_Objects[item->object_id];
                    if (object->floor) {
                        object->floor(item, x, y, z, &height);
                    }
                    break;

                case TO_CAMERA:
                    trigger = *fd++;
                    break;
                }
            } while (!TRIGGER_IS_END(trigger));
            break;

        case FT_LAVA:
            g_TriggerIndex = fd - 1;
            break;

        case FT_CLIMB:
            if (g_TriggerIndex == NULL) {
                g_TriggerIndex = fd - 1;
            }
            break;

        default:
            Shell_ExitSystem("GetHeight(): Unknown floordata type");
            break;
        }

        if (FLOORDATA_IS_END(fd_cmd)) {
            break;
        }
    }

    return height;
}

void __cdecl Room_TestTriggers(const int16_t *fd, bool heavy)
{
    ITEM *camera_item = NULL;
    bool switch_off = false;
    bool flip = false;
    bool flip_available = false;
    int32_t new_effect = -1;

    if (!heavy) {
        g_Lara.climb_status = 0;
    }
    if (fd == NULL) {
        return;
    }

    if (FLOORDATA_TYPE(*fd) == FT_LAVA) {
        if (!heavy
            && (g_LaraItem->pos.y == g_LaraItem->floor
                || g_Lara.water_status != LWS_ABOVE_WATER)) {
            Lara_TouchLava(g_LaraItem);
        }

        if (FLOORDATA_IS_END(*fd)) {
            return;
        }

        fd++;
    }

    if (FLOORDATA_TYPE(*fd) == FT_CLIMB) {
        if (!heavy) {
            const int32_t quad = Math_GetDirection(g_LaraItem->rot.y);
            if (*fd & (1 << (quad + 8))) {
                g_Lara.climb_status = 1;
            }
        }

        if (FLOORDATA_IS_END(*fd)) {
            return;
        }

        fd++;
    }

    const int16_t type = (*fd++ >> 8) & 0x3F;
    const int16_t flags = *fd++;
    const int16_t timer = flags & 0xFF;

    if (g_Camera.type != CAM_HEAVY) {
        Camera_RefreshFromTrigger(type, fd);
    }

    if (heavy) {
        if (type != TT_HEAVY) {
            return;
        }
    } else {
        switch (type) {
        case TT_PAD:
        case TT_ANTIPAD:
            if (g_LaraItem->pos.y != g_LaraItem->floor) {
                return;
            }
            break;

        case TT_SWITCH: {
            const int16_t value = TRIGGER_VALUE(*fd++);
            if (!SwitchTrigger(value, timer)) {
                return;
            }
            switch_off = g_Items[value].current_anim_state == LS_RUN;
            break;
        }

        case TT_KEY: {
            const int16_t value = TRIGGER_VALUE(*fd++);
            if (!KeyTrigger(value)) {
                return;
            }
            break;
        }

        case TT_PICKUP: {
            const int16_t value = TRIGGER_VALUE(*fd++);
            if (!PickupTrigger(value)) {
                return;
            }
            break;
        }

        case TT_HEAVY:
        case TT_DUMMY:
            return;

        case TT_COMBAT:
            if (g_Lara.gun_status != LGS_READY) {
                return;
            }
            break;

        default:
            break;
        }
    }

    int16_t trigger;
    do {
        trigger = *fd++;
        const int32_t value = TRIGGER_VALUE(trigger);

        switch (TRIGGER_TYPE(trigger)) {
        case TO_OBJECT: {
            ITEM *const item = &g_Items[value];
            if (item->flags & IF_ONE_SHOT) {
                break;
            }

            item->timer = timer;
            if (timer != 1) {
                item->timer *= FRAMES_PER_SECOND;
            }

            if (type == TT_SWITCH) {
                item->flags ^= flags & IF_CODE_BITS;
            } else if (type == TT_ANTIPAD || type == TT_ANTITRIGGER) {
                item->flags &= ~(flags & IF_CODE_BITS);
                if (flags & IF_ONE_SHOT) {
                    item->flags |= IF_ONE_SHOT;
                }
            } else if ((flags & IF_CODE_BITS) != 0) {
                item->flags |= flags & IF_CODE_BITS;
            }

            if ((item->flags & IF_CODE_BITS) != IF_CODE_BITS) {
                break;
            }

            if (flags & IF_ONE_SHOT) {
                item->flags |= IF_ONE_SHOT;
            }

            if (!item->active) {
                if (g_Objects[item->object_id].intelligent) {
                    if (item->status == IS_INACTIVE) {
                        item->touch_bits = 0;
                        item->status = IS_ACTIVE;
                        Item_AddActive(value);
                        LOT_EnableBaddieAI(value, true);
                    } else if (item->status == IS_INVISIBLE) {
                        item->touch_bits = 0;
                        if (LOT_EnableBaddieAI(value, false)) {
                            item->status = IS_ACTIVE;
                        } else {
                            item->status = IS_INVISIBLE;
                        }
                        Item_AddActive(value);
                    }
                } else {
                    item->touch_bits = 0;
                    item->status = IS_ACTIVE;
                    Item_AddActive(value);
                }
            }
            break;
        }

        case TO_CAMERA: {
            trigger = *fd++;
            const int16_t camera_flags = trigger;
            const int16_t camera_timer = trigger & 0xFF;

            if (g_Camera.fixed[value].flags & IF_ONE_SHOT) {
                break;
            }

            g_Camera.num = value;

            if (g_Camera.type == CAM_LOOK || g_Camera.type == CAM_COMBAT) {
                break;
            }
            if (type == TT_COMBAT) {
                break;
            }
            if (type == TT_SWITCH && timer && switch_off) {
                break;
            }
            if (value == g_Camera.last && type != TT_SWITCH) {
                break;
            }

            g_Camera.timer = FRAMES_PER_SECOND * camera_timer;
            if (camera_flags & IF_ONE_SHOT) {
                g_Camera.fixed[value].flags |= IF_ONE_SHOT;
            }
            g_Camera.speed = ((camera_flags & IF_CODE_BITS) >> 6) + 1;
            g_Camera.type = heavy ? CAM_HEAVY : CAM_FIXED;
            break;
        }

        case TO_SINK: {
            const OBJECT_VECTOR object_vector = g_Camera.fixed[value];
            if (!g_Lara.creature) {
                LOT_EnableBaddieAI(g_Lara.item_num, true);
            }
            g_Lara.creature->lot.target.x = object_vector.x;
            g_Lara.creature->lot.target.y = object_vector.y;
            g_Lara.creature->lot.target.z = object_vector.z;
            g_Lara.creature->lot.required_box = object_vector.flags;
            g_Lara.current_active = object_vector.data * 6;
            break;
        }

        case TO_FLIP_MAP:
            flip_available = true;
            if (g_FlipMaps[value] & IF_ONE_SHOT) {
                break;
            }

            if (type == TT_SWITCH) {
                g_FlipMaps[value] ^= flags & IF_CODE_BITS;
            } else if (flags & IF_CODE_BITS) {
                g_FlipMaps[value] |= flags & IF_CODE_BITS;
            }

            if ((g_FlipMaps[value] & IF_CODE_BITS) == IF_CODE_BITS) {
                if (flags & IF_ONE_SHOT) {
                    g_FlipMaps[value] |= IF_ONE_SHOT;
                }

                if (!g_FlipStatus) {
                    flip = 1;
                }
            } else if (g_FlipStatus) {
                flip = 1;
            }
            break;

        case TO_FLIP_ON:
            flip_available = true;
            if ((g_FlipMaps[value] & IF_CODE_BITS) == IF_CODE_BITS
                && !g_FlipStatus) {
                flip = 1;
            }
            break;

        case TO_FLIP_OFF:
            flip_available = true;
            if ((g_FlipMaps[value] & IF_CODE_BITS) == IF_CODE_BITS
                && g_FlipStatus) {
                flip = 1;
            }
            break;

        case TO_TARGET:
            camera_item = &g_Items[value];
            break;

        case TO_FINISH:
            g_LevelComplete = true;
            break;

        case TO_CD:
            Room_TriggerMusicTrack(value, flags, type);
            break;

        case TO_FLIP_EFFECT:
            new_effect = value;
            break;

        case TO_BODY_BAG:
            Item_ClearKilled();
            break;

        default:
            break;
        }
    } while (!TRIGGER_IS_END(trigger));

    if (camera_item != NULL
        && (g_Camera.type == CAM_FIXED || g_Camera.type == CAM_HEAVY)) {
        g_Camera.item = camera_item;
    }

    if (flip) {
        Room_FlipMap();
    }

    if (new_effect != -1 && (flip || !flip_available)) {
        g_FlipEffect = new_effect;
        g_FlipTimer = 0;
    }
}

int32_t __cdecl Room_GetCeiling(
    const SECTOR *const sector, const int32_t x, const int32_t y,
    const int32_t z)
{
    const SECTOR *f = sector;

    while (f->sky_room != NO_ROOM) {
        const ROOM *const r = &g_Rooms[f->sky_room];
        const int32_t z_sector = (z - r->pos.z) >> WALL_SHIFT;
        const int32_t x_sector = (x - r->pos.x) >> WALL_SHIFT;
        f = &r->sectors[z_sector + x_sector * r->size.z];
    }

    int32_t height = f->ceiling << 8;

    if (f->idx) {
        const int16_t *fd = &g_FloorData[f->idx];
        int16_t type = FLOORDATA_TYPE(*fd++);

        if (type == FT_TILT) {
            fd++;
            type = FLOORDATA_TYPE(*fd++);
        }

        if (type == FT_ROOF) {
            const int32_t x_off = *fd >> 8;
            const int32_t y_off = (int8_t)*fd;

            if (!g_IsChunkyCamera || (ABS(x_off) <= 2 && ABS(y_off) <= 2)) {
                if (x_off < 0) {
                    height += (x_off * (z & (WALL_L - 1))) >> 2;
                } else {
                    height -= (x_off * ((WALL_L - 1 - z) & (WALL_L - 1))) >> 2;
                }

                if (y_off < 0) {
                    height += (y_off * ((WALL_L - 1 - x) & (WALL_L - 1))) >> 2;
                } else {
                    height -= (y_off * (x & (WALL_L - 1))) >> 2;
                }
            }
        }
    }

    f = sector;
    while (f->pit_room != NO_ROOM) {
        const ROOM *const r = &g_Rooms[f->pit_room];
        const int32_t z_sector = (z - r->pos.z) >> WALL_SHIFT;
        const int32_t x_sector = (x - r->pos.x) >> WALL_SHIFT;
        f = &r->sectors[z_sector + x_sector * r->size.z];
    }

    if (!f->idx) {
        return height;
    }

    const int16_t *fd = &g_FloorData[f->idx];
    while (true) {
        const int16_t fd_cmd = *fd++;

        switch (FLOORDATA_TYPE(fd_cmd)) {
        case FT_DOOR:
        case FT_TILT:
        case FT_ROOF:
            fd++;
            break;

        case FT_TRIGGER: {
            fd++;

            int16_t trigger;
            do {
                trigger = *fd++;
                switch (TRIGGER_TYPE(trigger)) {
                case TO_OBJECT:
                    const int16_t value = TRIGGER_VALUE(trigger);
                    const ITEM *const item = &g_Items[value];
                    const OBJECT *const object = &g_Objects[item->object_id];
                    if (object->ceiling) {
                        object->ceiling(item, x, y, z, &height);
                    }
                    break;

                case TO_CAMERA:
                    trigger = *fd++;
                    break;
                }
            } while (!TRIGGER_IS_END(trigger));
            break;
        }

        case FT_LAVA:
        case FT_CLIMB:
            break;

        default:
            Shell_ExitSystem("GetCeiling(): Unknown type");
            break;
        }

        if (FLOORDATA_IS_END(fd_cmd)) {
            break;
        }
    }

    return height;
}

int16_t __cdecl Room_GetDoor(const SECTOR *const sector)
{
    if (!sector->idx) {
        return NO_ROOM;
    }

    const int16_t *fd = &g_FloorData[sector->idx];
    while (true) {
        const int16_t fd_cmd = *fd++;

        switch (FLOORDATA_TYPE(fd_cmd)) {
        case FT_DOOR:
            return *fd;

        case FT_ROOF:
        case FT_TILT:
            fd++;
            break;

        case FT_TRIGGER:
            fd++;

            while (true) {
                int16_t trigger = *fd++;
                switch (TRIGGER_TYPE(trigger)) {
                case TO_CAMERA:
                    trigger = *fd++;
                    break;

                default:
                    break;
                }

                if (TRIGGER_IS_END(trigger)) {
                    break;
                }
            }
            break;

        case FT_LAVA:
        case FT_CLIMB:
            break;

        default:
            Shell_ExitSystem("GetDoor(): Unknown floordata type");
            break;
        }

        if (FLOORDATA_IS_END(fd_cmd)) {
            break;
        }
    }

    return NO_ROOM;
}

void __cdecl Room_AlterFloorHeight(const ITEM *const item, const int32_t height)
{
    int16_t room_num = item->room_num;

    SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    const SECTOR *ceiling = Room_GetSector(
        item->pos.x, item->pos.y + height - WALL_L, item->pos.z, &room_num);

    if (sector->floor == NO_HEIGHT / 256) {
        sector->floor = ceiling->ceiling + height / 256;
    } else {
        sector->floor += height / 256;
        if (sector->floor == ceiling->ceiling) {
            sector->floor = NO_HEIGHT / 256;
        }
    }

    BOX_INFO *const box = &g_Boxes[sector->box];
    if (box->overlap_index & BOX_BLOCKABLE) {
        if (height < 0) {
            box->overlap_index |= BOX_BLOCKED;
        } else {
            box->overlap_index &= ~BOX_BLOCKED;
        }
    }
}

bool Room_GetFlipStatus(void)
{
    return g_FlipStatus;
}

void __cdecl Room_FlipMap(void)
{
    for (int32_t i = 0; i < g_RoomCount; i++) {
        ROOM *const r = &g_Rooms[i];
        if (r->flipped_room == NO_ROOM_NEG) {
            continue;
        }

        Room_RemoveFlipItems(r);

        ROOM *const flipped = &g_Rooms[r->flipped_room];
        ROOM temp = *r;
        *r = *flipped;
        *flipped = temp;

        r->flipped_room = flipped->flipped_room;
        flipped->flipped_room = NO_ROOM_NEG;

        // TODO: is this really necessary given the assignments above?
        r->item_num = flipped->item_num;
        r->fx_num = flipped->fx_num;

        Room_AddFlipItems(r);
    }

    g_FlipStatus = !g_FlipStatus;
}

void __cdecl Room_RemoveFlipItems(const ROOM *const r)
{
    int16_t item_num = r->item_num;

    while (item_num != NO_ITEM) {
        ITEM *const item = &g_Items[item_num];

        switch (item->object_id) {
        case O_MOVABLE_BLOCK_1:
        case O_MOVABLE_BLOCK_2:
        case O_MOVABLE_BLOCK_3:
        case O_MOVABLE_BLOCK_4:
            Room_AlterFloorHeight(item, WALL_L);
            break;

        default:
            break;
        }

        if (item->flags & IF_ONE_SHOT && g_Objects[item->object_id].intelligent
            && item->hit_points <= 0) {
            Item_RemoveDrawn(item_num);
            item->flags |= IF_KILLED;
        }

        item_num = item->next_item;
    }
}

void __cdecl Room_AddFlipItems(const ROOM *const r)
{
    int16_t item_num = r->item_num;
    while (item_num != NO_ITEM) {
        const ITEM *const item = &g_Items[item_num];

        switch (item->object_id) {
        case O_MOVABLE_BLOCK_1:
        case O_MOVABLE_BLOCK_2:
        case O_MOVABLE_BLOCK_3:
        case O_MOVABLE_BLOCK_4:
            Room_AlterFloorHeight(item, -WALL_L);
            break;

        default:
            break;
        }

        item_num = item->next_item;
    }
}

void __cdecl Room_TriggerMusicTrack(
    const int16_t track, const int16_t flags, const int16_t type)
{
    if (track < MX_CUTSCENE_THE_GREAT_WALL || track >= MX_TITLE_THEME) {
        return;
    }

    if (type != TT_SWITCH) {
        const int32_t code = flags & IF_CODE_BITS;
        if (g_MusicTrackFlags[track] & code) {
            return;
        }
        if (flags & IF_ONE_SHOT) {
            g_MusicTrackFlags[track] |= code;
        }
    }

    if (track != g_CD_TrackID) {
        const int32_t timer = flags & 0xFF;
        if (timer) {
            g_CD_TrackID = track;
            g_MusicTrackFlags[track] =
                (g_MusicTrackFlags[track] & 0xFF00) | ((30 * timer) & 0xFF);
        } else {
            Music_Play(track, false);
        }
    } else {
        int32_t timer = g_MusicTrackFlags[track] & 0xFF;
        if (timer) {
            timer--;
            if (timer == 0) {
                g_CD_TrackID = -1;
                Music_Play(track, false);
            }
            g_MusicTrackFlags[track] =
                (g_MusicTrackFlags[track] & 0xFF00) | (timer & 0xFF);
        }
    }
}

int32_t Room_GetTotalCount(void)
{
    return g_RoomCount;
}

ROOM *Room_Get(const int32_t room_num)
{
    return &g_Rooms[room_num];
}
