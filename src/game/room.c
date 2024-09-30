#include "game/room.h"

#include "game/camera.h"
#include "game/gamebuf.h"
#include "game/items.h"
#include "game/lara/misc.h"
#include "game/lot.h"
#include "game/music.h"
#include "game/objects/common.h"
#include "game/objects/general/keyhole.h"
#include "game/objects/general/pickup.h"
#include "game/objects/general/switch.h"
#include "game/shell.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#include <assert.h>
#include <stddef.h>

#define NULL_FD_INDEX 0
#define NEG_TILT(T, H) ((T * (H & (WALL_L - 1))) >> 2)
#define POS_TILT(T, H) ((T * ((WALL_L - 1 - H) & (WALL_L - 1))) >> 2)

int32_t g_FlipTimer = 0;
int32_t g_FlipEffect = -1;
int32_t g_FlipStatus = 0;
int32_t g_FlipMapTable[MAX_FLIP_MAPS] = { 0 };

static void M_TriggerMusicTrack(int16_t track, const TRIGGER *const trigger);
static void M_AddFlipItems(ROOM *r);
static void M_RemoveFlipItems(ROOM *r);

static int16_t M_GetFloorTiltHeight(
    const SECTOR *sector, const int32_t x, const int32_t z);
static int16_t M_GetCeilingTiltHeight(
    const SECTOR *sector, const int32_t x, const int32_t z);
static SECTOR *M_GetSkySector(const SECTOR *sector, int32_t x, int32_t z);
static void M_TestSectorTrigger(const ITEM *item, const SECTOR *sector);
static bool M_TestLava(const ITEM *const item);

static void M_TriggerMusicTrack(int16_t track, const TRIGGER *const trigger)
{
    if (track == MX_UNUSED_0 && trigger->type == TT_ANTIPAD) {
        Music_Stop();
        return;
    }

    if (track <= MX_UNUSED_1 || track >= MAX_CD_TRACKS) {
        return;
    }

    // handle g_Lara gym routines
    switch (track) {
    case MX_GYM_HINT_03:
        if ((g_MusicTrackFlags[track] & IF_ONE_SHOT)
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
        if ((g_MusicTrackFlags[track] & IF_ONE_SHOT)
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
        if (g_MusicTrackFlags[track] & IF_ONE_SHOT) {
            static int16_t gym_completion_counter = 0;
            gym_completion_counter++;
            if (gym_completion_counter == LOGIC_FPS * 4) {
                g_LevelComplete = true;
                gym_completion_counter = 0;
            }
        } else if (g_LaraItem->current_anim_state != LS_WATER_OUT) {
            return;
        }
        break;
    }
    // end of g_Lara gym routines

    if (g_MusicTrackFlags[track] & IF_ONE_SHOT) {
        return;
    }

    if (trigger->type == TT_SWITCH) {
        g_MusicTrackFlags[track] ^= trigger->mask;
    } else if (trigger->type == TT_ANTIPAD) {
        g_MusicTrackFlags[track] &= -1 - trigger->mask;
    } else if (trigger->mask) {
        g_MusicTrackFlags[track] |= trigger->mask;
    }

    if ((g_MusicTrackFlags[track] & IF_CODE_BITS) == IF_CODE_BITS) {
        if (trigger->one_shot) {
            g_MusicTrackFlags[track] |= IF_ONE_SHOT;
        }
        Music_Play(track);
    } else {
        Music_StopTrack(track);
    }
}

static void M_AddFlipItems(ROOM *r)
{
    for (int16_t item_num = r->item_num; item_num != NO_ITEM;
         item_num = g_Items[item_num].next_item) {
        ITEM *item = &g_Items[item_num];

        switch (item->object_id) {
        case O_MOVABLE_BLOCK_1:
        case O_MOVABLE_BLOCK_2:
        case O_MOVABLE_BLOCK_3:
        case O_MOVABLE_BLOCK_4:
            Room_AlterFloorHeight(item, -WALL_L);
            break;

        case O_SLIDING_PILLAR:
            Room_AlterFloorHeight(item, -WALL_L * 2);
            break;

        default:
            break;
        }
    }
}

static void M_RemoveFlipItems(ROOM *r)
{
    for (int16_t item_num = r->item_num; item_num != NO_ITEM;
         item_num = g_Items[item_num].next_item) {
        ITEM *item = &g_Items[item_num];

        switch (item->object_id) {
        case O_MOVABLE_BLOCK_1:
        case O_MOVABLE_BLOCK_2:
        case O_MOVABLE_BLOCK_3:
        case O_MOVABLE_BLOCK_4:
            Room_AlterFloorHeight(item, WALL_L);
            break;

        case O_SLIDING_PILLAR:
            Room_AlterFloorHeight(item, WALL_L * 2);
            break;

        default:
            break;
        }
    }
}

int16_t Room_GetTiltType(const SECTOR *sector, int32_t x, int32_t y, int32_t z)
{
    sector = Room_GetPitSector(sector, x, z);

    if ((y + STEP_L * 2) < sector->floor.height) {
        return 0;
    }

    return sector->floor.tilt;
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
    Room_GetSector(x, y, z, &room_num);

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

SECTOR *Room_GetPitSector(
    const SECTOR *sector, const int32_t x, const int32_t z)
{
    while (sector->portal_room.pit != NO_ROOM) {
        const ROOM *const r = &g_RoomInfo[sector->portal_room.pit];
        const int32_t z_sector = (z - r->pos.z) >> WALL_SHIFT;
        const int32_t x_sector = (x - r->pos.x) >> WALL_SHIFT;
        sector = &r->sectors[z_sector + x_sector * r->size.z];
    }

    return (SECTOR *)sector;
}

static SECTOR *M_GetSkySector(
    const SECTOR *sector, const int32_t x, const int32_t z)
{
    while (sector->portal_room.sky != NO_ROOM) {
        const ROOM *const r = &g_RoomInfo[sector->portal_room.sky];
        const int32_t z_sector = (z - r->pos.z) >> WALL_SHIFT;
        const int32_t x_sector = (x - r->pos.x) >> WALL_SHIFT;
        sector = &r->sectors[z_sector + x_sector * r->size.z];
    }

    return (SECTOR *)sector;
}

SECTOR *Room_GetSector(int32_t x, int32_t y, int32_t z, int16_t *room_num)
{
    int16_t portal_room;
    SECTOR *sector;
    const ROOM *r = &g_RoomInfo[*room_num];
    do {
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
        portal_room = sector->portal_room.wall;
        if (portal_room != NO_ROOM) {
            *room_num = portal_room;
            r = &g_RoomInfo[portal_room];
        }
    } while (portal_room != NO_ROOM);

    if (y >= sector->floor.height) {
        do {
            if (sector->portal_room.pit == NO_ROOM) {
                break;
            }

            *room_num = sector->portal_room.pit;

            r = &g_RoomInfo[sector->portal_room.pit];
            const int32_t z_sector = (z - r->pos.z) >> WALL_SHIFT;
            const int32_t x_sector = (x - r->pos.x) >> WALL_SHIFT;
            sector = &r->sectors[z_sector + x_sector * r->size.z];
        } while (y >= sector->floor.height);
    } else if (y < sector->ceiling.height) {
        do {
            if (sector->portal_room.sky == NO_ROOM) {
                break;
            }

            *room_num = sector->portal_room.sky;

            r = &g_RoomInfo[sector->portal_room.sky];
            const int32_t z_sector = (z - r->pos.z) >> WALL_SHIFT;
            const int32_t x_sector = (x - r->pos.x) >> WALL_SHIFT;
            sector = &r->sectors[z_sector + x_sector * r->size.z];
        } while (y < sector->ceiling.height);
    }

    return sector;
}

int16_t Room_GetCeiling(const SECTOR *sector, int32_t x, int32_t y, int32_t z)
{
    int16_t *data;
    int16_t type;
    int16_t trigger;

    const SECTOR *const sky_sector = M_GetSkySector(sector, x, z);
    int16_t height = M_GetCeilingTiltHeight(sky_sector, x, z);

    sector = Room_GetPitSector(sector, x, z);
    if (sector->trigger == NULL) {
        return height;
    }

    for (int32_t i = 0; i < sector->trigger->command_count; i++) {
        const TRIGGER_CMD *const cmd = &sector->trigger->commands[i];
        if (cmd->type != TO_OBJECT) {
            continue;
        }

        const ITEM *const item = &g_Items[(int16_t)(intptr_t)cmd->parameter];
        const OBJECT *const object = &g_Objects[item->object_id];
        if (object->ceiling_height_func) {
            height = object->ceiling_height_func(item, x, y, z, height);
        }
    }

    return height;
}

int16_t Room_GetHeight(const SECTOR *sector, int32_t x, int32_t y, int32_t z)
{
    g_HeightType = HT_WALL;
    sector = Room_GetPitSector(sector, x, z);

    int16_t height = M_GetFloorTiltHeight(sector, x, z);

    if (sector->trigger == NULL) {
        return height;
    }

    for (int32_t i = 0; i < sector->trigger->command_count; i++) {
        const TRIGGER_CMD *const cmd = &sector->trigger->commands[i];
        if (cmd->type != TO_OBJECT) {
            continue;
        }

        const ITEM *const item = &g_Items[(int16_t)(intptr_t)cmd->parameter];
        const OBJECT *const object = &g_Objects[item->object_id];
        if (object->floor_height_func) {
            height = object->floor_height_func(item, x, y, z, height);
        }
    }

    return height;
}

static int16_t M_GetFloorTiltHeight(
    const SECTOR *sector, const int32_t x, const int32_t z)
{
    int16_t height = sector->floor.height;
    if (sector->floor.tilt == 0) {
        return height;
    }

    const int32_t z_off = sector->floor.tilt >> 8;
    const int32_t x_off = (int8_t)sector->floor.tilt;

    const HEIGHT_TYPE slope_type =
        (ABS(z_off) > 2 || ABS(x_off) > 2) ? HT_BIG_SLOPE : HT_SMALL_SLOPE;
    if (g_ChunkyFlag && slope_type == HT_BIG_SLOPE) {
        return height;
    }

    g_HeightType = slope_type;

    if (z_off < 0) {
        height -= (int16_t)NEG_TILT(z_off, z);
    } else {
        height += (int16_t)POS_TILT(z_off, z);
    }

    if (x_off < 0) {
        height -= (int16_t)NEG_TILT(x_off, x);
    } else {
        height += (int16_t)POS_TILT(x_off, x);
    }

    return height;
}

static int16_t M_GetCeilingTiltHeight(
    const SECTOR *sector, const int32_t x, const int32_t z)
{
    int16_t height = sector->ceiling.height;
    if (sector->ceiling.tilt == 0) {
        return height;
    }

    const int32_t z_off = sector->ceiling.tilt >> 8;
    const int32_t x_off = (int8_t)sector->ceiling.tilt;

    if (g_ChunkyFlag && (ABS(z_off) > 2 || ABS(x_off) > 2)) {
        return height;
    }

    if (z_off < 0) {
        height += (int16_t)NEG_TILT(z_off, z);
    } else {
        height -= (int16_t)POS_TILT(z_off, z);
    }

    if (x_off < 0) {
        height += (int16_t)POS_TILT(x_off, x);
    } else {
        height -= (int16_t)NEG_TILT(x_off, x);
    }

    return height;
}

int16_t Room_GetWaterHeight(int32_t x, int32_t y, int32_t z, int16_t room_num)
{
    const ROOM *r = &g_RoomInfo[room_num];

    int16_t portal_room;
    const SECTOR *sector;
    int32_t z_sector, x_sector;

    do {
        z_sector = (z - r->pos.z) >> WALL_SHIFT;
        x_sector = (x - r->pos.x) >> WALL_SHIFT;

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
        portal_room = sector->portal_room.wall;
        if (portal_room != NO_ROOM) {
            r = &g_RoomInfo[portal_room];
        }
    } while (portal_room != NO_ROOM);

    if (r->flags & RF_UNDERWATER) {
        while (sector->portal_room.sky != NO_ROOM) {
            r = &g_RoomInfo[sector->portal_room.sky];
            if (!(r->flags & RF_UNDERWATER)) {
                break;
            }
            z_sector = (z - r->pos.z) >> WALL_SHIFT;
            x_sector = (x - r->pos.x) >> WALL_SHIFT;
            sector = &r->sectors[z_sector + x_sector * r->size.z];
        }
        return sector->ceiling.height;
    } else {
        while (sector->portal_room.pit != NO_ROOM) {
            r = &g_RoomInfo[sector->portal_room.pit];
            if (r->flags & RF_UNDERWATER) {
                return sector->floor.height;
            }
            z_sector = (z - r->pos.z) >> WALL_SHIFT;
            x_sector = (x - r->pos.x) >> WALL_SHIFT;
            sector = &r->sectors[z_sector + x_sector * r->size.z];
        }
        return NO_HEIGHT;
    }
}

int16_t Room_GetIndexFromPos(const int32_t x, const int32_t y, const int32_t z)
{
    for (int i = 0; i < g_RoomCount; i++) {
        const ROOM *const room = &g_RoomInfo[i];
        const int32_t x1 = room->pos.x + WALL_L;
        const int32_t x2 = room->pos.x + (room->size.x << WALL_SHIFT) - WALL_L;
        const int32_t y1 = room->max_ceiling;
        const int32_t y2 = room->min_floor;
        const int32_t z1 = room->pos.z + WALL_L;
        const int32_t z2 = room->pos.z + (room->size.z << WALL_SHIFT) - WALL_L;
        if (x >= x1 && x < x2 && y >= y1 && y <= y2 && z >= z1 && z < z2) {
            return i;
        }
    }
    return NO_ROOM;
}

void Room_AlterFloorHeight(ITEM *item, int32_t height)
{
    if (!height) {
        return;
    }

    int16_t portal_room;
    SECTOR *sector;
    const ROOM *r = &g_RoomInfo[item->room_num];

    do {
        int32_t z_sector = (item->pos.z - r->pos.z) >> WALL_SHIFT;
        int32_t x_sector = (item->pos.x - r->pos.x) >> WALL_SHIFT;

        if (z_sector <= 0) {
            z_sector = 0;
            CLAMP(x_sector, 1, r->size.x - 2);
        } else if (z_sector >= r->size.z - 1) {
            z_sector = r->size.z - 1;
            CLAMP(x_sector, 1, r->size.x - 2);
        } else {
            CLAMP(x_sector, 0, r->size.x - 1);
        }

        sector = &r->sectors[z_sector + x_sector * r->size.z];
        portal_room = sector->portal_room.wall;
        if (portal_room != NO_ROOM) {
            r = &g_RoomInfo[portal_room];
        }
    } while (portal_room != NO_ROOM);

    const SECTOR *const sky_sector =
        M_GetSkySector(sector, item->pos.x, item->pos.z);
    sector = Room_GetPitSector(sector, item->pos.x, item->pos.z);

    if (sector->floor.height != NO_HEIGHT) {
        sector->floor.height += ROUND_TO_CLICK(height);
        if (sector->floor.height == sky_sector->ceiling.height) {
            sector->floor.height = NO_HEIGHT;
        }
    } else {
        sector->floor.height =
            sky_sector->ceiling.height + ROUND_TO_CLICK(height);
    }

    if (g_Boxes[sector->box].overlap_index & BLOCKABLE) {
        if (height < 0) {
            g_Boxes[sector->box].overlap_index |= BLOCKED;
        } else {
            g_Boxes[sector->box].overlap_index &= ~BLOCKED;
        }
    }
}

bool Room_GetFlipStatus(void)
{
    return g_FlipStatus;
}

void Room_FlipMap(void)
{
    Sound_StopAmbientSounds();

    for (int i = 0; i < g_RoomCount; i++) {
        ROOM *r = &g_RoomInfo[i];
        if (r->flipped_room < 0) {
            continue;
        }

        M_RemoveFlipItems(r);

        ROOM *flipped = &g_RoomInfo[r->flipped_room];
        ROOM temp = *r;
        *r = *flipped;
        *flipped = temp;

        r->flipped_room = flipped->flipped_room;
        flipped->flipped_room = -1;

        // XXX: is this really necessary given the assignments above?
        r->item_num = flipped->item_num;
        r->fx_num = flipped->fx_num;

        M_AddFlipItems(r);
    }

    g_FlipStatus = !g_FlipStatus;
}

void Room_ParseFloorData(const int16_t *floor_data)
{
    for (int32_t i = 0; i < g_RoomCount; i++) {
        const ROOM *const room = &g_RoomInfo[i];
        for (int32_t j = 0; j < room->size.x * room->size.z; j++) {
            SECTOR *const sector = &room->sectors[j];
            Room_PopulateSectorData(
                &room->sectors[j], floor_data, sector->idx, NULL_FD_INDEX);
        }
    }
}

void Room_PopulateSectorData(
    SECTOR *const sector, const int16_t *floor_data, const uint16_t start_index,
    const uint16_t null_index)
{
    sector->floor.tilt = 0;
    sector->ceiling.tilt = 0;
    sector->portal_room.wall = NO_ROOM;
    sector->is_death_sector = false;
    sector->trigger = NULL;

    if (start_index == null_index) {
        return;
    }

    const int16_t *data = &floor_data[start_index];
    int16_t fd_entry;
    do {
        fd_entry = *data++;

        switch (fd_entry & DATA_TYPE) {
        case FT_TILT:
            sector->floor.tilt = *data++;
            break;

        case FT_ROOF:
            sector->ceiling.tilt = *data++;
            break;

        case FT_DOOR:
            sector->portal_room.wall = *data++;
            break;

        case FT_LAVA:
            sector->is_death_sector = true;
            break;

        case FT_TRIGGER: {
            assert(sector->trigger == NULL);

            TRIGGER *const trigger =
                GameBuf_Alloc(sizeof(TRIGGER), GBUF_FLOOR_DATA);
            sector->trigger = trigger;

            const int16_t trig_setup = *data++;
            trigger->type = TRIG_TYPE(fd_entry);
            trigger->timer = trig_setup & 0xFF;
            trigger->one_shot = trig_setup & IF_ONE_SHOT;
            trigger->mask = trig_setup & IF_CODE_BITS;
            trigger->item_index = NO_ITEM;
            trigger->command_count = 0;

            if (trigger->type == TT_SWITCH || trigger->type == TT_KEY
                || trigger->type == TT_PICKUP) {
                const int16_t item_data = *data++;
                trigger->item_index = item_data & VALUE_BITS;
                if (item_data & END_BIT) {
                    // See City of Khamoon room 49 - two dangling key triggers
                    // with no commands. Exit early to avoid populating garbage
                    // command data.
                    break;
                }
            }

            const int16_t *command_data = data;
            while (true) {
                trigger->command_count++;
                int16_t command = *data++;
                if (TRIG_BITS(command) == TO_CAMERA) {
                    command = *data++;
                }
                if (command & END_BIT) {
                    break;
                }
            }

            trigger->commands = GameBuf_Alloc(
                sizeof(TRIGGER_CMD) * trigger->command_count, GBUF_FLOOR_DATA);
            for (int32_t i = 0; i < trigger->command_count; i++) {
                int16_t command = *command_data++;
                TRIGGER_CMD *const cmd = &trigger->commands[i];
                cmd->type = TRIG_BITS(command);

                if (cmd->type == TO_CAMERA) {
                    TRIGGER_CAMERA_DATA *const cam_data = GameBuf_Alloc(
                        sizeof(TRIGGER_CAMERA_DATA), GBUF_FLOOR_DATA);
                    cmd->parameter = (void *)cam_data;
                    cam_data->camera_num = command & VALUE_BITS;

                    command = *command_data++;
                    cam_data->timer = command & 0xFF;
                    cam_data->glide = (command & IF_CODE_BITS) >> 6;
                    cam_data->one_shot = command & IF_ONE_SHOT;
                } else {
                    cmd->parameter = (void *)(intptr_t)(command & VALUE_BITS);
                }
            }

            break;
        }

        default:
            break;
        }
    } while (!(fd_entry & END_BIT));
}

void Room_TestTriggers(const ITEM *const item)
{
    int16_t room_num = item->room_num;
    const SECTOR *sector =
        Room_GetSector(item->pos.x, MAX_HEIGHT, item->pos.z, &room_num);

    M_TestSectorTrigger(item, sector);
    if (item->object_id != O_TORSO) {
        return;
    }

    for (int32_t dx = -1; dx < 2; dx++) {
        for (int32_t dz = -1; dz < 2; dz++) {
            if (!dx && !dz) {
                continue;
            }

            room_num = item->room_num;
            sector = Room_GetSector(
                item->pos.x + dx * WALL_L, MAX_HEIGHT,
                item->pos.z + dz * WALL_L, &room_num);
            M_TestSectorTrigger(item, sector);
        }
    }
}

static bool M_TestLava(const ITEM *const item)
{
    if (item->hit_points < 0 || g_Lara.water_status == LWS_CHEAT
        || (g_Lara.water_status == LWS_ABOVE_WATER
            && item->pos.y != item->floor)) {
        return false;
    }

    // OG fix: check if floor index has lava
    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, MAX_HEIGHT, item->pos.z, &room_num);
    return sector->is_death_sector;
}

static void M_TestSectorTrigger(
    const ITEM *const item, const SECTOR *const sector)
{
    const bool is_heavy = item->object_id != O_LARA;
    if (!is_heavy && sector->is_death_sector && M_TestLava(item)) {
        Lara_CatchFire();
    }

    const TRIGGER *const trigger = sector->trigger;
    if (trigger == NULL) {
        return;
    }

    if (g_Camera.type != CAM_HEAVY) {
        Camera_RefreshFromTrigger(trigger);
    }

    bool switch_off = false;
    bool flip_map = false;
    int32_t new_effect = -1;
    ITEM *camera_item = NULL;

    if (is_heavy) {
        if (trigger->type != TT_HEAVY) {
            return;
        }
    } else {
        switch (trigger->type) {
        case TT_TRIGGER:
            break;

        case TT_SWITCH: {
            if (!Switch_Trigger(trigger->item_index, trigger->timer)) {
                return;
            }
            switch_off =
                g_Items[trigger->item_index].current_anim_state == LS_RUN;
            break;
        }

        case TT_PAD:
        case TT_ANTIPAD:
            if (item->pos.y != item->floor) {
                return;
            }
            break;

        case TT_KEY: {
            if (!KeyHole_Trigger(trigger->item_index)) {
                return;
            }
            break;
        }

        case TT_PICKUP: {
            if (!Pickup_Trigger(trigger->item_index)) {
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

    for (int32_t i = 0; i < trigger->command_count; i++) {
        const TRIGGER_CMD *const cmd = &trigger->commands[i];

        switch (cmd->type) {
        case TO_OBJECT: {
            const int16_t item_num = (int16_t)(intptr_t)cmd->parameter;
            ITEM *const item = &g_Items[item_num];
            if (item->flags & IF_ONE_SHOT) {
                break;
            }

            item->timer = trigger->timer;
            if (item->timer != 1) {
                item->timer *= LOGIC_FPS;
            }

            if (trigger->type == TT_SWITCH) {
                item->flags ^= trigger->mask;
            } else if (trigger->type == TT_ANTIPAD) {
                item->flags &= -1 - trigger->mask;
            } else if (trigger->mask) {
                item->flags |= trigger->mask;
            }

            if ((item->flags & IF_CODE_BITS) != IF_CODE_BITS) {
                break;
            }

            if (trigger->one_shot) {
                item->flags |= IF_ONE_SHOT;
            }

            if (!item->active) {
                if (g_Objects[item->object_id].intelligent) {
                    if (item->status == IS_INACTIVE) {
                        item->touch_bits = 0;
                        item->status = IS_ACTIVE;
                        Item_AddActive(item_num);
                        LOT_EnableBaddieAI(item_num, 1);
                    } else if (item->status == IS_INVISIBLE) {
                        item->touch_bits = 0;
                        if (LOT_EnableBaddieAI(item_num, 0)) {
                            item->status = IS_ACTIVE;
                        } else {
                            item->status = IS_INVISIBLE;
                        }
                        Item_AddActive(item_num);
                    }
                } else {
                    item->touch_bits = 0;
                    item->status = IS_ACTIVE;
                    Item_AddActive(item_num);
                }
            }
            break;
        }

        case TO_CAMERA: {
            const TRIGGER_CAMERA_DATA *const cam_data =
                (TRIGGER_CAMERA_DATA *)cmd->parameter;
            if (g_Camera.fixed[cam_data->camera_num].flags & IF_ONE_SHOT) {
                break;
            }

            g_Camera.number = cam_data->camera_num;

            if (g_Camera.type == CAM_LOOK || g_Camera.type == CAM_COMBAT) {
                break;
            }

            if (trigger->type == TT_COMBAT) {
                break;
            }

            if (trigger->type == TT_SWITCH && trigger->timer && switch_off) {
                break;
            }

            if (g_Camera.number == g_Camera.last
                && trigger->type != TT_SWITCH) {
                break;
            }

            g_Camera.timer = cam_data->timer;
            if (g_Camera.timer != 1) {
                g_Camera.timer *= LOGIC_FPS;
            }

            if (cam_data->one_shot) {
                g_Camera.fixed[g_Camera.number].flags |= IF_ONE_SHOT;
            }

            g_Camera.speed = cam_data->glide + 1;
            g_Camera.type = is_heavy ? CAM_HEAVY : CAM_FIXED;
            break;
        }

        case TO_TARGET:
            camera_item = &g_Items[(int16_t)(intptr_t)cmd->parameter];
            break;

        case TO_SINK: {
            const OBJECT_VECTOR *const obvector =
                &g_Camera.fixed[(int16_t)(intptr_t)cmd->parameter];

            if (g_Lara.lot.required_box != obvector->flags) {
                g_Lara.lot.target.x = obvector->x;
                g_Lara.lot.target.y = obvector->y;
                g_Lara.lot.target.z = obvector->z;
                g_Lara.lot.required_box = obvector->flags;
            }

            g_Lara.current_active = obvector->data * 6;
            break;
        }

        case TO_FLIPMAP: {
            const int16_t flip_slot = (int16_t)(intptr_t)cmd->parameter;
            if (g_FlipMapTable[flip_slot] & IF_ONE_SHOT) {
                break;
            }

            if (trigger->type == TT_SWITCH) {
                g_FlipMapTable[flip_slot] ^= trigger->mask;
            } else if (trigger->mask) {
                g_FlipMapTable[flip_slot] |= trigger->mask;
            }

            if ((g_FlipMapTable[flip_slot] & IF_CODE_BITS) == IF_CODE_BITS) {
                if (trigger->one_shot) {
                    g_FlipMapTable[flip_slot] |= IF_ONE_SHOT;
                }

                if (!g_FlipStatus) {
                    flip_map = true;
                }
            } else if (g_FlipStatus) {
                flip_map = true;
            }
            break;
        }

        case TO_FLIPON: {
            const int16_t flip_slot = (int16_t)(intptr_t)cmd->parameter;
            if ((g_FlipMapTable[flip_slot] & IF_CODE_BITS) == IF_CODE_BITS
                && !g_FlipStatus) {
                flip_map = true;
            }
            break;
        }

        case TO_FLIPOFF: {
            const int16_t flip_slot = (int16_t)(intptr_t)cmd->parameter;
            if ((g_FlipMapTable[flip_slot] & IF_CODE_BITS) == IF_CODE_BITS
                && g_FlipStatus) {
                flip_map = true;
            }
            break;
        }

        case TO_FLIPEFFECT:
            new_effect = (int16_t)(intptr_t)cmd->parameter;
            break;

        case TO_FINISH:
            g_LevelComplete = true;
            break;

        case TO_CD:
            M_TriggerMusicTrack((int16_t)(intptr_t)cmd->parameter, trigger);
            break;

        case TO_SECRET: {
            const int16_t secret_num = 1 << (int16_t)(intptr_t)cmd->parameter;
            if (g_GameInfo.current[g_CurrentLevel].stats.secret_flags
                & secret_num) {
                break;
            }
            g_GameInfo.current[g_CurrentLevel].stats.secret_flags |= secret_num;
            Music_Play(MX_SECRET);
            break;
        }
        }
    }

    if (camera_item
        && (g_Camera.type == CAM_FIXED || g_Camera.type == CAM_HEAVY)) {
        g_Camera.item = camera_item;
    }

    if (flip_map) {
        Room_FlipMap();
        if (new_effect != -1) {
            g_FlipEffect = new_effect;
            g_FlipTimer = 0;
        }
    }
}

bool Room_IsOnWalkable(
    const SECTOR *sector, const int32_t x, const int32_t y, const int32_t z,
    const int32_t room_height)
{
    sector = Room_GetPitSector(sector, x, z);
    if (sector->trigger == NULL) {
        return false;
    }

    int16_t height = sector->floor.height;
    bool object_found = false;
    for (int32_t i = 0; i < sector->trigger->command_count; i++) {
        const TRIGGER_CMD *const cmd = &sector->trigger->commands[i];
        if (cmd->type != TO_OBJECT) {
            continue;
        }

        const int16_t item_num = (int16_t)(intptr_t)cmd->parameter;
        const ITEM *const item = &g_Items[item_num];
        const OBJECT *const object = &g_Objects[item->object_id];
        if (object->floor_height_func) {
            height = object->floor_height_func(item, x, y, z, height);
            object_found = true;
        }
    }

    return object_found && room_height == height;
}

int32_t Room_GetTotalCount(void)
{
    return g_RoomCount;
}

ROOM *Room_Get(const int32_t room_num)
{
    return &g_RoomInfo[room_num];
}
