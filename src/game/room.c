#include "game/room.h"

#include "game/camera.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/music.h"
#include "game/objects/keyhole.h"
#include "game/objects/pickup.h"
#include "game/objects/switch.h"
#include "game/objects/traps/lava.h"
#include "game/shell.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"
#include "util.h"

#include <stddef.h>

static void Room_TriggerMusicTrack(int16_t track, int16_t flags, int16_t type);
static void Room_AddFlipItems(ROOM_INFO *r);
static void Room_RemoveFlipItems(ROOM_INFO *r);

static void Room_TriggerMusicTrack(int16_t track, int16_t flags, int16_t type)
{
    if (track <= 1 || track >= MAX_CD_TRACKS) {
        return;
    }

    // handle g_Lara gym routines
    switch (track) {
    case MX_GYM_HINT_03:
        if ((g_MusicTrackFlags[track] & IF_ONESHOT)
            && g_LaraItem->current_anim_state == LS_JUMP_UP) {
            track = MX_GYM_HINT_04;
        }
        break;

    case MX_GYM_HINT_12:
        if (g_LaraItem->current_anim_state != LS_HANG) {
            return;
        }
        break;

    case MX_GYM_HINT_16:
        if (g_LaraItem->current_anim_state != LS_HANG) {
            return;
        }
        break;

    case MX_GYM_HINT_17:
        if ((g_MusicTrackFlags[track] & IF_ONESHOT)
            && g_LaraItem->current_anim_state == LS_HANG) {
            track = MX_GYM_HINT_18;
        }
        break;

    case MX_GYM_HINT_24:
        if (g_LaraItem->current_anim_state != LS_SURF_TREAD) {
            return;
        }
        break;

    case MX_GYM_HINT_25:
        if (g_MusicTrackFlags[track] & IF_ONESHOT) {
            static int16_t gym_completion_counter = 0;
            gym_completion_counter++;
            if (gym_completion_counter == FRAMES_PER_SECOND * 4) {
                g_LevelComplete = true;
                gym_completion_counter = 0;
            }
        } else if (g_LaraItem->current_anim_state != LS_WATER_OUT) {
            return;
        }
        break;
    }
    // end of g_Lara gym routines

    if (g_MusicTrackFlags[track] & IF_ONESHOT) {
        return;
    }

    if (type == TT_SWITCH) {
        g_MusicTrackFlags[track] ^= flags & IF_CODE_BITS;
    } else if (type == TT_ANTIPAD) {
        g_MusicTrackFlags[track] &= -1 - (flags & IF_CODE_BITS);
    } else if (flags & IF_CODE_BITS) {
        g_MusicTrackFlags[track] |= flags & IF_CODE_BITS;
    }

    if ((g_MusicTrackFlags[track] & IF_CODE_BITS) == IF_CODE_BITS) {
        if (flags & IF_ONESHOT) {
            g_MusicTrackFlags[track] |= IF_ONESHOT;
        }
        Music_Play(track);
    } else {
        Music_Stop();
    }
}

static void Room_AddFlipItems(ROOM_INFO *r)
{
    for (int16_t item_num = r->item_number; item_num != NO_ITEM;
         item_num = g_Items[item_num].next_item) {
        ITEM_INFO *item = &g_Items[item_num];

        switch (item->object_number) {
        case O_MOVABLE_BLOCK:
        case O_MOVABLE_BLOCK2:
        case O_MOVABLE_BLOCK3:
        case O_MOVABLE_BLOCK4:
            Room_AlterFloorHeight(item, -WALL_L);
            break;

        case O_ROLLING_BLOCK:
            Room_AlterFloorHeight(item, -WALL_L * 2);
            break;

        default:
            break;
        }
    }
}

static void Room_RemoveFlipItems(ROOM_INFO *r)
{
    for (int16_t item_num = r->item_number; item_num != NO_ITEM;
         item_num = g_Items[item_num].next_item) {
        ITEM_INFO *item = &g_Items[item_num];

        switch (item->object_number) {
        case O_MOVABLE_BLOCK:
        case O_MOVABLE_BLOCK2:
        case O_MOVABLE_BLOCK3:
        case O_MOVABLE_BLOCK4:
            Room_AlterFloorHeight(item, WALL_L);
            break;

        case O_ROLLING_BLOCK:
            Room_AlterFloorHeight(item, WALL_L * 2);
            break;

        default:
            break;
        }
    }
}

int16_t Room_GetTiltType(FLOOR_INFO *floor, int32_t x, int32_t y, int32_t z)
{
    ROOM_INFO *r;

    while (floor->pit_room != NO_ROOM) {
        r = &g_RoomInfo[floor->pit_room];
        floor = &r->floor
                     [((z - r->z) >> WALL_SHIFT)
                      + ((x - r->x) >> WALL_SHIFT) * r->x_size];
    }

    if (y + 512 < ((int32_t)floor->floor << 8)) {
        return 0;
    }

    if (floor->index) {
        int16_t *data = &g_FloorData[floor->index];
        if ((data[0] & DATA_TYPE) == FT_TILT) {
            return data[1];
        }
    }

    return 0;
}

int32_t Room_FindGridShift(int32_t src, int32_t dst)
{
    int32_t srcw = src >> WALL_SHIFT;
    int32_t dstw = dst >> WALL_SHIFT;
    if (srcw == dstw) {
        return 0;
    }

    src &= WALL_L - 1;
    if (dstw > srcw) {
        return WALL_L - (src - 1);
    } else {
        return -(src + 1);
    }
}

void Room_GetNearByRooms(
    int32_t x, int32_t y, int32_t z, int32_t r, int32_t h, int16_t room_num)
{
    g_RoomsToDrawCount = 0;
    if (g_RoomsToDrawCount + 1 < MAX_ROOMS_TO_DRAW) {
        g_RoomsToDraw[g_RoomsToDrawCount++] = room_num;
    }
    Room_GetNewRoom(x + r, y, z + r, room_num);
    Room_GetNewRoom(x - r, y, z + r, room_num);
    Room_GetNewRoom(x + r, y, z - r, room_num);
    Room_GetNewRoom(x - r, y, z - r, room_num);
    Room_GetNewRoom(x + r, y - h, z + r, room_num);
    Room_GetNewRoom(x - r, y - h, z + r, room_num);
    Room_GetNewRoom(x + r, y - h, z - r, room_num);
    Room_GetNewRoom(x - r, y - h, z - r, room_num);
}

void Room_GetNewRoom(int32_t x, int32_t y, int32_t z, int16_t room_num)
{
    Room_GetFloor(x, y, z, &room_num);

    for (int i = 0; i < g_RoomsToDrawCount; i++) {
        int16_t drawn_room = g_RoomsToDraw[i];
        if (drawn_room == room_num) {
            return;
        }
    }

    if (g_RoomsToDrawCount + 1 < MAX_ROOMS_TO_DRAW) {
        g_RoomsToDraw[g_RoomsToDrawCount++] = room_num;
    }
}

FLOOR_INFO *Room_GetFloor(int32_t x, int32_t y, int32_t z, int16_t *room_num)
{
    int16_t data;
    FLOOR_INFO *floor;
    ROOM_INFO *r = &g_RoomInfo[*room_num];
    do {
        int32_t x_floor = (z - r->z) >> WALL_SHIFT;
        int32_t y_floor = (x - r->x) >> WALL_SHIFT;

        if (x_floor <= 0) {
            x_floor = 0;
            if (y_floor < 1) {
                y_floor = 1;
            } else if (y_floor > r->y_size - 2) {
                y_floor = r->y_size - 2;
            }
        } else if (x_floor >= r->x_size - 1) {
            x_floor = r->x_size - 1;
            if (y_floor < 1) {
                y_floor = 1;
            } else if (y_floor > r->y_size - 2) {
                y_floor = r->y_size - 2;
            }
        } else if (y_floor < 0) {
            y_floor = 0;
        } else if (y_floor >= r->y_size) {
            y_floor = r->y_size - 1;
        }

        floor = &r->floor[x_floor + y_floor * r->x_size];
        if (!floor->index) {
            break;
        }

        data = Room_GetDoor(floor);
        if (data != NO_ROOM) {
            *room_num = data;
            r = &g_RoomInfo[data];
        }
    } while (data != NO_ROOM);

    if (y >= ((int32_t)floor->floor << 8)) {
        do {
            if (floor->pit_room == NO_ROOM) {
                break;
            }

            *room_num = floor->pit_room;

            r = &g_RoomInfo[floor->pit_room];
            int32_t x_floor = (z - r->z) >> WALL_SHIFT;
            int32_t y_floor = (x - r->x) >> WALL_SHIFT;
            floor = &r->floor[x_floor + y_floor * r->x_size];
        } while (y >= ((int32_t)floor->floor << 8));
    } else if (y < ((int32_t)floor->ceiling << 8)) {
        do {
            if (floor->sky_room == NO_ROOM) {
                break;
            }

            *room_num = floor->sky_room;

            r = &g_RoomInfo[floor->sky_room];
            int32_t x_floor = (z - r->z) >> WALL_SHIFT;
            int32_t y_floor = (x - r->x) >> WALL_SHIFT;
            floor = &r->floor[x_floor + y_floor * r->x_size];
        } while (y < ((int32_t)floor->ceiling << 8));
    }

    return floor;
}

int16_t Room_GetCeiling(FLOOR_INFO *floor, int32_t x, int32_t y, int32_t z)
{
    int16_t *data;
    int16_t type;
    int16_t trigger;

    FLOOR_INFO *f = floor;
    while (f->sky_room != NO_ROOM) {
        ROOM_INFO *r = &g_RoomInfo[f->sky_room];
        int32_t x_floor = (z - r->z) >> WALL_SHIFT;
        int32_t y_floor = (x - r->x) >> WALL_SHIFT;
        f = &r->floor[x_floor + y_floor * r->x_size];
    }

    int16_t height = f->ceiling << 8;

    if (f->index) {
        data = &g_FloorData[f->index];
        type = *data++ & DATA_TYPE;

        if (type == FT_TILT) {
            data++;
            type = *data++ & DATA_TYPE;
        }

        if (type == FT_ROOF) {
            int32_t xoff = data[0] >> 8;
            int32_t yoff = (int8_t)data[0];

            if (!g_ChunkyFlag
                || (xoff >= -2 && xoff <= 2 && yoff >= -2 && yoff <= 2)) {
                if (xoff < 0) {
                    height += (int16_t)((xoff * (z & (WALL_L - 1))) >> 2);
                } else {
                    height -=
                        (int16_t)((xoff * ((WALL_L - 1 - z) & (WALL_L - 1))) >> 2);
                }

                if (yoff < 0) {
                    height +=
                        (int16_t)((yoff * ((WALL_L - 1 - x) & (WALL_L - 1))) >> 2);
                } else {
                    height -= (int16_t)((yoff * (x & (WALL_L - 1))) >> 2);
                }
            }
        }
    }

    while (floor->pit_room != NO_ROOM) {
        ROOM_INFO *r = &g_RoomInfo[floor->pit_room];
        int32_t x_floor = (z - r->z) >> WALL_SHIFT;
        int32_t y_floor = (x - r->x) >> WALL_SHIFT;
        floor = &r->floor[x_floor + y_floor * r->x_size];
    }

    if (!floor->index) {
        return height;
    }

    data = &g_FloorData[floor->index];
    do {
        type = *data++;

        switch (type & DATA_TYPE) {
        case FT_DOOR:
        case FT_TILT:
        case FT_ROOF:
            data++;
            break;

        case FT_LAVA:
            break;

        case FT_TRIGGER:
            data++;
            do {
                trigger = *data++;
                if (TRIG_BITS(trigger) != TO_OBJECT) {
                    if (TRIG_BITS(trigger) == TO_CAMERA) {
                        trigger = *data++;
                    }
                } else {
                    ITEM_INFO *item = &g_Items[trigger & VALUE_BITS];
                    OBJECT_INFO *object = &g_Objects[item->object_number];
                    if (object->ceiling) {
                        object->ceiling(item, x, y, z, &height);
                    }
                }
            } while (!(trigger & END_BIT));
            break;

        default:
            Shell_ExitSystem("GetCeiling(): Unknown type");
            break;
        }
    } while (!(type & END_BIT));

    return height;
}

int16_t Room_GetDoor(FLOOR_INFO *floor)
{
    if (!floor->index) {
        return NO_ROOM;
    }

    int16_t *data = &g_FloorData[floor->index];
    int16_t type = *data++;

    if (type == FT_TILT) {
        data++;
        type = *data++;
    }

    if (type == FT_ROOF) {
        data++;
        type = *data++;
    }

    if ((type & DATA_TYPE) == FT_DOOR) {
        return *data;
    }
    return NO_ROOM;
}

int16_t Room_GetHeight(FLOOR_INFO *floor, int32_t x, int32_t y, int32_t z)
{
    g_HeightType = HT_WALL;
    while (floor->pit_room != NO_ROOM) {
        ROOM_INFO *r = &g_RoomInfo[floor->pit_room];
        int32_t x_floor = (z - r->z) >> WALL_SHIFT;
        int32_t y_floor = (x - r->x) >> WALL_SHIFT;
        floor = &r->floor[x_floor + y_floor * r->x_size];
    }

    int16_t height = floor->floor << 8;

    g_TriggerIndex = NULL;

    if (!floor->index) {
        return height;
    }

    int16_t *data = &g_FloorData[floor->index];
    int16_t type;
    int16_t trigger;
    do {
        type = *data++;

        switch (type & DATA_TYPE) {
        case FT_TILT: {
            int32_t xoff = data[0] >> 8;
            int32_t yoff = (int8_t)data[0];

            if (!g_ChunkyFlag || (ABS(xoff) <= 2 && ABS(yoff) <= 2)) {
                if (ABS(xoff) > 2 || ABS(yoff) > 2) {
                    g_HeightType = HT_BIG_SLOPE;
                } else {
                    g_HeightType = HT_SMALL_SLOPE;
                }

                if (xoff < 0) {
                    height -= (int16_t)((xoff * (z & (WALL_L - 1))) >> 2);
                } else {
                    height +=
                        (int16_t)((xoff * ((WALL_L - 1 - z) & (WALL_L - 1))) >> 2);
                }

                if (yoff < 0) {
                    height -= (int16_t)((yoff * (x & (WALL_L - 1))) >> 2);
                } else {
                    height +=
                        (int16_t)((yoff * ((WALL_L - 1 - x) & (WALL_L - 1))) >> 2);
                }
            }

            data++;
            break;
        }

        case FT_ROOF:
        case FT_DOOR:
            data++;
            break;

        case FT_LAVA:
            g_TriggerIndex = data - 1;
            break;

        case FT_TRIGGER:
            if (!g_TriggerIndex) {
                g_TriggerIndex = data - 1;
            }

            data++;
            do {
                trigger = *data++;
                if (TRIG_BITS(trigger) != TO_OBJECT) {
                    if (TRIG_BITS(trigger) == TO_CAMERA) {
                        trigger = *data++;
                    }
                } else {
                    ITEM_INFO *item = &g_Items[trigger & VALUE_BITS];
                    OBJECT_INFO *object = &g_Objects[item->object_number];
                    if (object->floor) {
                        object->floor(item, x, y, z, &height);
                    }
                }
            } while (!(trigger & END_BIT));
            break;

        default:
            Shell_ExitSystem("GetHeight(): Unknown type");
            break;
        }
    } while (!(type & END_BIT));

    return height;
}

int16_t Room_GetWaterHeight(int32_t x, int32_t y, int32_t z, int16_t room_num)
{
    ROOM_INFO *r = &g_RoomInfo[room_num];

    int16_t data;
    FLOOR_INFO *floor;
    int32_t x_floor, y_floor;

    do {
        x_floor = (z - r->z) >> WALL_SHIFT;
        y_floor = (x - r->x) >> WALL_SHIFT;

        if (x_floor <= 0) {
            x_floor = 0;
            if (y_floor < 1) {
                y_floor = 1;
            } else if (y_floor > r->y_size - 2) {
                y_floor = r->y_size - 2;
            }
        } else if (x_floor >= r->x_size - 1) {
            x_floor = r->x_size - 1;
            if (y_floor < 1) {
                y_floor = 1;
            } else if (y_floor > r->y_size - 2) {
                y_floor = r->y_size - 2;
            }
        } else if (y_floor < 0) {
            y_floor = 0;
        } else if (y_floor >= r->y_size) {
            y_floor = r->y_size - 1;
        }

        floor = &r->floor[x_floor + y_floor * r->x_size];
        data = Room_GetDoor(floor);
        if (data != NO_ROOM) {
            r = &g_RoomInfo[data];
        }
    } while (data != NO_ROOM);

    if (r->flags & RF_UNDERWATER) {
        while (floor->sky_room != NO_ROOM) {
            r = &g_RoomInfo[floor->sky_room];
            if (!(r->flags & RF_UNDERWATER)) {
                break;
            }
            x_floor = (z - r->z) >> WALL_SHIFT;
            y_floor = (x - r->x) >> WALL_SHIFT;
            floor = &r->floor[x_floor + y_floor * r->x_size];
        }
        return floor->ceiling << 8;
    } else {
        while (floor->pit_room != NO_ROOM) {
            r = &g_RoomInfo[floor->pit_room];
            if (r->flags & RF_UNDERWATER) {
                return floor->floor << 8;
            }
            x_floor = (z - r->z) >> WALL_SHIFT;
            y_floor = (x - r->x) >> WALL_SHIFT;
            floor = &r->floor[x_floor + y_floor * r->x_size];
        }
        return NO_HEIGHT;
    }
}

void Room_AlterFloorHeight(ITEM_INFO *item, int32_t height)
{
    int16_t room_num = item->room_number;
    FLOOR_INFO *floor =
        Room_GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    FLOOR_INFO *ceiling = Room_GetFloor(
        item->pos.x, item->pos.y + height - WALL_L, item->pos.z, &room_num);

    if (floor->floor == NO_HEIGHT / 256) {
        floor->floor = ceiling->ceiling + height / 256;
    } else {
        floor->floor += height / 256;
        if (floor->floor == ceiling->ceiling) {
            floor->floor = NO_HEIGHT / 256;
        }
    }

    if (g_Boxes[floor->box].overlap_index & BLOCKABLE) {
        if (height < 0) {
            g_Boxes[floor->box].overlap_index |= BLOCKED;
        } else {
            g_Boxes[floor->box].overlap_index &= ~BLOCKED;
        }
    }
}

void Room_FlipMap(void)
{
    Sound_StopAmbientSounds();

    for (int i = 0; i < g_RoomCount; i++) {
        ROOM_INFO *r = &g_RoomInfo[i];
        if (r->flipped_room < 0) {
            continue;
        }

        Room_RemoveFlipItems(r);

        ROOM_INFO *flipped = &g_RoomInfo[r->flipped_room];
        ROOM_INFO temp = *r;
        *r = *flipped;
        *flipped = temp;

        r->flipped_room = flipped->flipped_room;
        flipped->flipped_room = -1;

        // XXX: is this really necessary given the assignments above?
        r->item_number = flipped->item_number;
        r->fx_number = flipped->fx_number;

        Room_AddFlipItems(r);
    }

    g_FlipStatus = !g_FlipStatus;
}

void Room_TestTriggers(int16_t *data, bool heavy)
{
    if (!data) {
        return;
    }

    if ((*data & DATA_TYPE) == FT_LAVA) {
        if (!heavy && g_LaraItem->pos.y == g_LaraItem->floor) {
            if (Lava_TestFloor(g_LaraItem)) {
                Lava_Burn(g_LaraItem);
            }
        }

        if (*data & END_BIT) {
            return;
        }

        data++;
    }

    int16_t type = (*data++ >> 8) & 0x3F;
    int32_t switch_off = 0;
    int32_t flip = 0;
    int32_t new_effect = -1;
    int16_t flags = *data++;
    int16_t timer = flags & 0xFF;

    if (g_Camera.type != CAM_HEAVY) {
        Camera_RefreshFromTrigger(type, data);
    }

    if (heavy) {
        if (type != TT_HEAVY) {
            return;
        }
    } else {
        switch (type) {
        case TT_SWITCH: {
            int16_t value = *data++ & VALUE_BITS;
            if (!Switch_Trigger(value, timer)) {
                return;
            }
            switch_off = g_Items[value].current_anim_state == LS_RUN;
            break;
        }

        case TT_PAD:
        case TT_ANTIPAD:
            if (g_LaraItem->pos.y != g_LaraItem->floor) {
                return;
            }
            break;

        case TT_KEY: {
            int16_t value = *data++ & VALUE_BITS;
            if (!KeyHole_Trigger(value)) {
                return;
            }
            break;
        }

        case TT_PICKUP: {
            int16_t value = *data++ & VALUE_BITS;
            if (!Pickup_Trigger(value)) {
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
        }
    }

    ITEM_INFO *camera_item = NULL;
    int16_t trigger;
    do {
        trigger = *data++;
        int16_t value = trigger & VALUE_BITS;

        switch (TRIG_BITS(trigger)) {
        case TO_OBJECT: {
            ITEM_INFO *item = &g_Items[value];

            if (item->flags & IF_ONESHOT) {
                break;
            }

            item->timer = timer;
            if (timer != 1) {
                item->timer *= FRAMES_PER_SECOND;
            }

            if (type == TT_SWITCH) {
                item->flags ^= flags & IF_CODE_BITS;
            } else if (type == TT_ANTIPAD) {
                item->flags &= -1 - (flags & IF_CODE_BITS);
            } else if (flags & IF_CODE_BITS) {
                item->flags |= flags & IF_CODE_BITS;
            }

            if ((item->flags & IF_CODE_BITS) != IF_CODE_BITS) {
                break;
            }

            if (flags & IF_ONESHOT) {
                item->flags |= IF_ONESHOT;
            }

            if (!item->active) {
                if (g_Objects[item->object_number].intelligent) {
                    if (item->status == IS_NOT_ACTIVE) {
                        item->touch_bits = 0;
                        item->status = IS_ACTIVE;
                        Item_AddActive(value);
                        LOT_EnableBaddieAI(value, 1);
                    } else if (item->status == IS_INVISIBLE) {
                        item->touch_bits = 0;
                        if (LOT_EnableBaddieAI(value, 0)) {
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
            trigger = *data++;
            int16_t camera_flags = trigger;
            int16_t camera_timer = trigger & 0xFF;

            if (g_Camera.fixed[value].flags & IF_ONESHOT) {
                break;
            }

            g_Camera.number = value;

            if (g_Camera.type == CAM_LOOK || g_Camera.type == CAM_COMBAT) {
                break;
            }

            if (type == TT_COMBAT) {
                break;
            }

            if (type == TT_SWITCH && timer && switch_off) {
                break;
            }

            if (g_Camera.number == g_Camera.last && type != TT_SWITCH) {
                break;
            }

            g_Camera.timer = camera_timer;
            if (g_Camera.timer != 1) {
                g_Camera.timer *= FRAMES_PER_SECOND;
            }

            if (camera_flags & IF_ONESHOT) {
                g_Camera.fixed[g_Camera.number].flags |= IF_ONESHOT;
            }

            g_Camera.speed = ((camera_flags & IF_CODE_BITS) >> 6) + 1;
            g_Camera.type = heavy ? CAM_HEAVY : CAM_FIXED;
            break;
        }

        case TO_TARGET:
            camera_item = &g_Items[value];
            break;

        case TO_SINK: {
            OBJECT_VECTOR *obvector = &g_Camera.fixed[value];

            if (g_Lara.LOT.required_box != obvector->flags) {
                g_Lara.LOT.target.x = obvector->x;
                g_Lara.LOT.target.y = obvector->y;
                g_Lara.LOT.target.z = obvector->z;
                g_Lara.LOT.required_box = obvector->flags;
            }

            g_Lara.current_active = obvector->data * 6;
            break;
        }

        case TO_FLIPMAP:
            if (g_FlipMapTable[value] & IF_ONESHOT) {
                break;
            }

            if (type == TT_SWITCH) {
                g_FlipMapTable[value] ^= flags & IF_CODE_BITS;
            } else if (flags & IF_CODE_BITS) {
                g_FlipMapTable[value] |= flags & IF_CODE_BITS;
            }

            if ((g_FlipMapTable[value] & IF_CODE_BITS) == IF_CODE_BITS) {
                if (flags & IF_ONESHOT) {
                    g_FlipMapTable[value] |= IF_ONESHOT;
                }

                if (!g_FlipStatus) {
                    flip = 1;
                }
            } else if (g_FlipStatus) {
                flip = 1;
            }
            break;

        case TO_FLIPON:
            if ((g_FlipMapTable[value] & IF_CODE_BITS) == IF_CODE_BITS
                && !g_FlipStatus) {
                flip = 1;
            }
            break;

        case TO_FLIPOFF:
            if ((g_FlipMapTable[value] & IF_CODE_BITS) == IF_CODE_BITS
                && g_FlipStatus) {
                flip = 1;
            }
            break;

        case TO_FLIPEFFECT:
            new_effect = value;
            break;

        case TO_FINISH:
            g_LevelComplete = true;
            break;

        case TO_CD:
            Room_TriggerMusicTrack(value, flags, type);
            break;

        case TO_SECRET:
            if ((g_GameInfo.current[g_CurrentLevel].stats.secret_flags
                 & (1 << value))) {
                break;
            }
            g_GameInfo.current[g_CurrentLevel].stats.secret_flags |= 1 << value;
            Music_Play(13);
            break;
        }
    } while (!(trigger & END_BIT));

    if (camera_item
        && (g_Camera.type == CAM_FIXED || g_Camera.type == CAM_HEAVY)) {
        g_Camera.item = camera_item;
    }

    if (flip) {
        Room_FlipMap();
        if (new_effect != -1) {
            g_FlipEffect = new_effect;
            g_FlipTimer = 0;
        }
    }
}
