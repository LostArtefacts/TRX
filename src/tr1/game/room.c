#include "game/room.h"

#include "game/camera.h"
#include "game/game.h"
#include "game/items.h"
#include "game/lara/misc.h"
#include "game/lot.h"
#include "game/music.h"
#include "game/objects/common.h"
#include "game/objects/general/keyhole.h"
#include "game/objects/general/pickup.h"
#include "game/objects/general/switch.h"
#include "game/savegame.h"
#include "game/shell.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/game/game_buf.h>
#include <libtrx/utils.h>

static void M_TriggerMusicTrack(int16_t track, const TRIGGER *const trigger);

static int16_t M_GetFloorTiltHeight(
    const SECTOR *sector, const int32_t x, const int32_t z);
static int16_t M_GetCeilingTiltHeight(
    const SECTOR *sector, const int32_t x, const int32_t z);
static SECTOR *M_GetSkySector(const SECTOR *sector, int32_t x, int32_t z);
static bool M_TestLava(const ITEM *const item);

static void M_TriggerMusicTrack(int16_t track, const TRIGGER *const trigger)
{
    if (track == MX_UNUSED_0 && trigger->type == TT_ANTIPAD) {
        Music_Stop();
        return;
    }

    if (track <= MX_UNUSED_1 || track >= MAX_MUSIC_TRACKS) {
        return;
    }

    // handle g_Lara gym routines
    uint16_t flags = Music_GetTrackFlags(track);
    switch (track) {
    case MX_GYM_HINT_03:
        if ((flags & IF_ONE_SHOT)
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
        if ((flags & IF_ONE_SHOT)
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
        if (flags & IF_ONE_SHOT) {
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

    if (flags & IF_ONE_SHOT) {
        return;
    }

    if (trigger->type == TT_SWITCH) {
        flags ^= trigger->mask;
    } else if (trigger->type == TT_ANTIPAD) {
        flags &= -1 - trigger->mask;
    } else if (trigger->mask) {
        flags |= trigger->mask;
    }

    if ((flags & IF_CODE_BITS) == IF_CODE_BITS) {
        if (trigger->one_shot) {
            flags |= IF_ONE_SHOT;
        }
        Music_Play(track, MPM_TRACKED);
    } else {
        Music_StopTrack(track);
    }

    Music_SetTrackFlags(track, flags);
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
    Room_DrawReset();
    Room_MarkToBeDrawn(room_num);
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
    Room_MarkToBeDrawn(room_num);
}

SECTOR *Room_GetPitSector(
    const SECTOR *sector, const int32_t x, const int32_t z)
{
    while (sector->portal_room.pit != NO_ROOM) {
        const ROOM *const room = Room_Get(sector->portal_room.pit);
        sector = Room_GetWorldSector(room, x, z);
    }

    return (SECTOR *)sector;
}

static SECTOR *M_GetSkySector(
    const SECTOR *sector, const int32_t x, const int32_t z)
{
    while (sector->portal_room.sky != NO_ROOM) {
        const ROOM *const room = Room_Get(sector->portal_room.sky);
        sector = Room_GetWorldSector(room, x, z);
    }

    return (SECTOR *)sector;
}

SECTOR *Room_GetSector(int32_t x, int32_t y, int32_t z, int16_t *room_num)
{
    int16_t portal_room;
    SECTOR *sector;
    const ROOM *room = Room_Get(*room_num);
    do {
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
        portal_room = sector->portal_room.wall;
        if (portal_room != NO_ROOM) {
            *room_num = portal_room;
            room = Room_Get(portal_room);
        }
    } while (portal_room != NO_ROOM);

    if (y >= sector->floor.height) {
        do {
            if (sector->portal_room.pit == NO_ROOM) {
                break;
            }

            *room_num = sector->portal_room.pit;
            room = Room_Get(sector->portal_room.pit);
            sector = Room_GetWorldSector(room, x, z);
        } while (y >= sector->floor.height);
    } else if (y < sector->ceiling.height) {
        do {
            if (sector->portal_room.sky == NO_ROOM) {
                break;
            }

            *room_num = sector->portal_room.sky;
            room = Room_Get(sector->portal_room.sky);
            sector = Room_GetWorldSector(room, x, z);
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
    if (sector->trigger == nullptr) {
        return height;
    }

    const TRIGGER_CMD *cmd = sector->trigger->command;
    for (; cmd != nullptr; cmd = cmd->next_cmd) {
        if (cmd->type != TO_OBJECT) {
            continue;
        }

        const ITEM *const item = Item_Get((int16_t)(intptr_t)cmd->parameter);
        const OBJECT *const obj = Object_Get(item->object_id);
        if (obj->ceiling_height_func) {
            height = obj->ceiling_height_func(item, x, y, z, height);
        }
    }

    return height;
}

int16_t Room_GetHeight(const SECTOR *sector, int32_t x, int32_t y, int32_t z)
{
    g_HeightType = HT_WALL;
    sector = Room_GetPitSector(sector, x, z);

    int16_t height = M_GetFloorTiltHeight(sector, x, z);

    if (sector->trigger == nullptr) {
        return height;
    }

    const TRIGGER_CMD *cmd = sector->trigger->command;
    for (; cmd != nullptr; cmd = cmd->next_cmd) {
        if (cmd->type != TO_OBJECT) {
            continue;
        }

        const ITEM *const item = Item_Get((int16_t)(intptr_t)cmd->parameter);
        const OBJECT *const obj = Object_Get(item->object_id);
        if (obj->floor_height_func) {
            height = obj->floor_height_func(item, x, y, z, height);
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
    if (Camera_IsChunky() && slope_type == HT_BIG_SLOPE) {
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

    if (Camera_IsChunky() && (ABS(z_off) > 2 || ABS(x_off) > 2)) {
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
    const ROOM *room = Room_Get(room_num);

    int16_t portal_room;
    const SECTOR *sector;
    int32_t z_sector, x_sector;

    do {
        z_sector = (z - room->pos.z) >> WALL_SHIFT;
        x_sector = (x - room->pos.x) >> WALL_SHIFT;

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
        portal_room = sector->portal_room.wall;
        if (portal_room != NO_ROOM) {
            room = Room_Get(portal_room);
        }
    } while (portal_room != NO_ROOM);

    if (room->flags & RF_UNDERWATER) {
        while (sector->portal_room.sky != NO_ROOM) {
            room = Room_Get(sector->portal_room.sky);
            if (!(room->flags & RF_UNDERWATER)) {
                break;
            }
            sector = Room_GetWorldSector(room, x, z);
        }
        return sector->ceiling.height;
    } else {
        while (sector->portal_room.pit != NO_ROOM) {
            room = Room_Get(sector->portal_room.pit);
            if (room->flags & RF_UNDERWATER) {
                return sector->floor.height;
            }
            sector = Room_GetWorldSector(room, x, z);
        }
        return NO_HEIGHT;
    }
}

void Room_AlterFloorHeight(const ITEM *const item, const int32_t height)
{
    if (!height) {
        return;
    }

    int16_t portal_room;
    SECTOR *sector;
    const ROOM *room = Room_Get(item->room_num);

    do {
        int32_t z_sector = (item->pos.z - room->pos.z) >> WALL_SHIFT;
        int32_t x_sector = (item->pos.x - room->pos.x) >> WALL_SHIFT;

        if (z_sector <= 0) {
            z_sector = 0;
            CLAMP(x_sector, 1, room->size.x - 2);
        } else if (z_sector >= room->size.z - 1) {
            z_sector = room->size.z - 1;
            CLAMP(x_sector, 1, room->size.x - 2);
        } else {
            CLAMP(x_sector, 0, room->size.x - 1);
        }

        sector = Room_GetUnitSector(room, x_sector, z_sector);
        portal_room = sector->portal_room.wall;
        if (portal_room != NO_ROOM) {
            room = Room_Get(portal_room);
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

void Room_TestTriggers(const ITEM *const item)
{
    int16_t room_num = item->room_num;
    const SECTOR *sector =
        Room_GetSector(item->pos.x, MAX_HEIGHT, item->pos.z, &room_num);

    Room_TestSectorTrigger(item, sector);
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
            Room_TestSectorTrigger(item, sector);
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

void Room_TestSectorTrigger(const ITEM *const item, const SECTOR *const sector)
{
    const bool is_heavy = item->object_id != O_LARA;
    if (!is_heavy && sector->is_death_sector && M_TestLava(item)) {
        Lara_CatchFire();
    }

    const TRIGGER *const trigger = sector->trigger;
    if (trigger == nullptr) {
        return;
    }

    if (g_Camera.type != CAM_HEAVY) {
        Camera_RefreshFromTrigger(trigger);
    }

    bool switch_off = false;
    bool flip_map = false;
    int32_t new_effect = -1;
    ITEM *camera_item = nullptr;
    const bool flip_status = Room_GetFlipStatus();

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
                Item_Get(trigger->item_index)->current_anim_state == LS_RUN;
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

    const TRIGGER_CMD *cmd = trigger->command;
    for (; cmd != nullptr; cmd = cmd->next_cmd) {
        switch (cmd->type) {
        case TO_OBJECT: {
            const int16_t item_num = (int16_t)(intptr_t)cmd->parameter;
            ITEM *const item = Item_Get(item_num);
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
                if (Object_Get(item->object_id)->intelligent) {
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
            OBJECT_VECTOR *const camera =
                Camera_GetFixedObject(cam_data->camera_num);
            if (camera->flags & IF_ONE_SHOT) {
                break;
            }

            g_Camera.num = cam_data->camera_num;

            if (g_Camera.type == CAM_LOOK || g_Camera.type == CAM_COMBAT) {
                break;
            }

            if (trigger->type == TT_COMBAT) {
                break;
            }

            if (trigger->type == TT_SWITCH && trigger->timer && switch_off) {
                break;
            }

            if (g_Camera.num == g_Camera.last && trigger->type != TT_SWITCH) {
                break;
            }

            g_Camera.timer = cam_data->timer;
            if (g_Camera.timer != 1) {
                g_Camera.timer *= LOGIC_FPS;
            }

            if (cam_data->one_shot) {
                camera->flags |= IF_ONE_SHOT;
            }

            g_Camera.speed = cam_data->glide + 1;
            g_Camera.type = is_heavy ? CAM_HEAVY : CAM_FIXED;
            break;
        }

        case TO_TARGET:
            camera_item = Item_Get((int16_t)(intptr_t)cmd->parameter);
            break;

        case TO_SINK: {
            const OBJECT_VECTOR *const sink =
                Camera_GetFixedObject((int16_t)(intptr_t)cmd->parameter);

            if (g_Lara.lot.required_box != sink->flags) {
                g_Lara.lot.target = sink->pos;
                g_Lara.lot.required_box = sink->flags;
            }

            g_Lara.current_active = sink->data * 6;
            break;
        }

        case TO_FLIPMAP: {
            const int16_t flip_slot = (int16_t)(intptr_t)cmd->parameter;
            int32_t slot_flags = Room_GetFlipSlotFlags(flip_slot);
            if (slot_flags & IF_ONE_SHOT) {
                break;
            }

            if (trigger->type == TT_SWITCH) {
                slot_flags ^= trigger->mask;
            } else if (trigger->mask) {
                slot_flags |= trigger->mask;
            }

            if ((slot_flags & IF_CODE_BITS) == IF_CODE_BITS) {
                if (trigger->one_shot) {
                    slot_flags |= IF_ONE_SHOT;
                }

                if (!flip_status) {
                    flip_map = true;
                }
            } else if (flip_status) {
                flip_map = true;
            }

            Room_SetFlipSlotFlags(flip_slot, slot_flags);
            break;
        }

        case TO_FLIPON: {
            const int16_t flip_slot = (int16_t)(intptr_t)cmd->parameter;
            const int32_t slot_flags = Room_GetFlipSlotFlags(flip_slot);
            if ((slot_flags & IF_CODE_BITS) == IF_CODE_BITS && !flip_status) {
                flip_map = true;
            }
            break;
        }

        case TO_FLIPOFF: {
            const int16_t flip_slot = (int16_t)(intptr_t)cmd->parameter;
            const int32_t slot_flags = Room_GetFlipSlotFlags(flip_slot);
            if ((slot_flags & IF_CODE_BITS) == IF_CODE_BITS && flip_status) {
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
            RESUME_INFO *resume_info =
                Savegame_GetCurrentInfo(Game_GetCurrentLevel());
            if (resume_info->stats.secret_flags & secret_num) {
                break;
            }
            resume_info->stats.secret_flags |= secret_num;
            resume_info->stats.secret_count++;
            Music_Play(MX_SECRET, MPM_ALWAYS);
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
            Room_SetFlipEffect(new_effect);
            Room_SetFlipTimer(0);
        }
    }
}

bool Room_IsOnWalkable(
    const SECTOR *sector, const int32_t x, const int32_t y, const int32_t z,
    const int32_t room_height)
{
    sector = Room_GetPitSector(sector, x, z);
    if (sector->trigger == nullptr) {
        return false;
    }

    int16_t height = sector->floor.height;
    bool object_found = false;
    const TRIGGER_CMD *cmd = sector->trigger->command;
    for (; cmd != nullptr; cmd = cmd->next_cmd) {
        if (cmd->type != TO_OBJECT) {
            continue;
        }

        const int16_t item_num = (int16_t)(intptr_t)cmd->parameter;
        const ITEM *const item = Item_Get(item_num);
        const OBJECT *const obj = Object_Get(item->object_id);
        if (obj->floor_height_func) {
            height = obj->floor_height_func(item, x, y, z, height);
            object_found = true;
        }
    }

    return object_found && room_height == height;
}
