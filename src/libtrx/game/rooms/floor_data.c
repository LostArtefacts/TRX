#include "game/rooms/floor_data.h"

#include "game/camera.h"
#include "game/game.h"
#include "game/game_buf.h"
#include "game/gym.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/music.h"
#include "game/objects/general/keyhole.h"
#include "game/objects/general/pickup.h"
#include "game/objects/general/switch.h"
#include "game/pathing.h"
#include "game/rooms.h"
#include "game/stats.h"

#define M_NULL_INDEX 0
#define M_IS_DONE(t) ((t & 0x8000) == 0x8000)

#define M_ENTRY_TYPE(t) (t & 0x1F)
#define M_TRIG_TYPE(t) ((t & 0x7F00) >> 8)
#define M_TRIG_TIMER(t) (t & 0xFF)
#define M_TRIG_ONE_SHOT(t) ((t & 0x100) == 0x100)
#define M_TRIG_MASK(t) (t & 0x3E00)
#define M_TRIG_CMD_TYPE(t) ((t & 0x7C00) >> 10)
#define M_TRIG_CMD_ARG(t) (t & 0x3FF)
#define M_TRIG_CAM_GLIDE(t) ((t & 0x3E00) >> 6)
#define M_LADDER_TYPE(t) ((t & 0x7F00) >> 8)

static const int16_t *M_ReadTrigger(
    const int16_t *data, const int16_t fd_entry, SECTOR *const sector)
{
    TRIGGER *const trigger = GameBuf_Alloc(sizeof(TRIGGER), GBUF_FLOOR_DATA);

    const int16_t trig_setup = *data++;
    trigger->type = M_TRIG_TYPE(fd_entry);
    trigger->timer = M_TRIG_TIMER(trig_setup);
    trigger->one_shot = M_TRIG_ONE_SHOT(trig_setup);
    trigger->mask = M_TRIG_MASK(trig_setup);
    trigger->item_index = NO_ITEM;

    if (trigger->type == TT_SWITCH || trigger->type == TT_KEY
        || trigger->type == TT_PICKUP) {
        const int16_t item_data = *data++;
        trigger->item_index = M_TRIG_CMD_ARG(item_data);
        if (M_IS_DONE(item_data)) {
            return data;
        }
    }

    TRIGGER_CMD *cmd;
    if (sector->trigger == nullptr) {
        sector->trigger = trigger;
        sector->trigger->command =
            GameBuf_Alloc(sizeof(TRIGGER_CMD), GBUF_FLOOR_DATA);
        cmd = sector->trigger->command;
    } else {
        // Some old TRLEs have incorrectly formatted floor data, with multiple
        // trigger entries defined where regular triggers overlap dummies. In
        // this case we link the new commands onto the old.
        cmd = sector->trigger->command;
        while (cmd->next_cmd != nullptr) {
            cmd = cmd->next_cmd;
        }
        cmd->next_cmd = GameBuf_Alloc(sizeof(TRIGGER_CMD), GBUF_FLOOR_DATA);
        cmd = cmd->next_cmd;
    }

    while (true) {
        int16_t command = *data++;
        cmd->type = M_TRIG_CMD_TYPE(command);

        if (cmd->type == TO_CAMERA) {
            TRIGGER_CAMERA_DATA *const cam_data =
                GameBuf_Alloc(sizeof(TRIGGER_CAMERA_DATA), GBUF_FLOOR_DATA);
            cmd->parameter = (void *)cam_data;
            cam_data->camera_num = M_TRIG_CMD_ARG(command);

            command = *data++;
            cam_data->timer = M_TRIG_TIMER(command);
            cam_data->glide = M_TRIG_CAM_GLIDE(command);
            cam_data->one_shot = M_TRIG_ONE_SHOT(command);
        } else {
            cmd->parameter = (void *)(intptr_t)M_TRIG_CMD_ARG(command);
        }

        if (M_IS_DONE(command)) {
            cmd->next_cmd = nullptr;
            break;
        }

        cmd->next_cmd = GameBuf_Alloc(sizeof(TRIGGER_CMD), GBUF_FLOOR_DATA);
        cmd = cmd->next_cmd;
    }

    return data;
}

static void M_ReadTriangulation(
    SURFACE *const surface, const int16_t func_data, const int16_t tilt_data)
{
    switch (M_ENTRY_TYPE(func_data)) {
    case FT_FLOOR_NWSE_SOLID:
    case FT_ROOF_NWSE_SOLID:
        surface->split.type = SPLIT_NWSE_SOLID;
        break;
    case FT_FLOOR_NESW_SOLID:
    case FT_ROOF_NESW_SOLID:
        surface->split.type = SPLIT_NESW_SOLID;
        break;
    case FT_FLOOR_NWSE_PORTAL_SW:
    case FT_ROOF_NWSE_PORTAL_SW:
        surface->split.type = SPLIT_NWSE_PORTAL_SW;
        break;
    case FT_FLOOR_NWSE_PORTAL_NE:
    case FT_ROOF_NWSE_PORTAL_NE:
        surface->split.type = SPLIT_NWSE_PORTAL_NE;
        break;
    case FT_FLOOR_NESW_PORTAL_SE:
    case FT_ROOF_NESW_PORTAL_SE:
        surface->split.type = SPLIT_NESW_PORTAL_SE;
        break;
    case FT_FLOOR_NESW_PORTAL_NW:
    case FT_ROOF_NESW_PORTAL_NW:
        surface->split.type = SPLIT_NESW_PORTAL_NW;
        break;
    default:
        return;
    }

    surface->is_split = true;
    surface->split.h1 = (func_data & 0x03E0) >> 5;
    surface->split.h2 = (func_data & 0x7C00) >> 10;
    for (int32_t i = 0; i < 4; i++) {
        surface->split.tilts[i] = (tilt_data >> (i * 4)) & 0xF;
    }
}

static bool M_TestLava(const ITEM *const item)
{
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (item->hit_points < 0 || lara_info->water_status == LWS_CHEAT
        || (lara_info->water_status == LWS_ABOVE_WATER
            && item->pos.y != item->floor)) {
        return false;
    }

    // OG fix: check if floor index has lava
    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, MAX_HEIGHT, item->pos.z, &room_num);
    return sector->is_death_sector;
}

static void M_TriggerMusicTrack(MUSIC_ID track_id, const TRIGGER *const trigger)
{
    if (track_id == (MUSIC_ID)0
        && (trigger->type == TT_ANTIPAD || trigger->type == TT_ANTITRIGGER)) {
        Music_Stop();
        return;
    }

    if (track_id <= Music_ToGameID(MX_UNUSED_1) || track_id >= MAX_MUSIC_TRACKS
        || (Game_IsInGym() && !Gym_CanPlayMusicTrack(&track_id))) {
        return;
    }

    uint16_t flags = Music_GetTrackFlags(track_id);
    // TODO: consolidate
#if TR_VERSION == 1
    if ((flags & IF_ONE_SHOT) != 0) {
        return;
    }

    if (trigger->type == TT_SWITCH) {
        flags ^= trigger->mask;
    } else if (trigger->type == TT_ANTIPAD || trigger->type == TT_ANTITRIGGER) {
        flags &= -1 - trigger->mask;
    } else if (trigger->mask) {
        flags |= trigger->mask;
    }

    if ((flags & IF_CODE_BITS) == IF_CODE_BITS) {
        if (trigger->one_shot) {
            flags |= IF_ONE_SHOT;
        }
        Music_Play_Direct(track_id, MPM_TRACKED);
    } else {
        Music_StopTrack_Direct(track_id);
    }
#else
    if (trigger->type != TT_SWITCH) {
        const int32_t code = trigger->mask;
        if ((flags & code) != 0) {
            return;
        }
        if (trigger->one_shot) {
            flags |= code;
        }
    }

    if (trigger->timer == 0) {
        Music_Play_Direct(track_id, MPM_TRACKED);
        goto finish;
    }

    if (track_id != Music_GetDelayedTrack()) {
        Music_Play_Direct(track_id, MPM_DELAYED);
        flags = (flags & 0xFF00) | ((LOGIC_FPS * trigger->timer) & 0xFF);
        goto finish;
    }

    int32_t timer = flags & 0xFF;
    if (timer == 0) {
        goto finish;
    }

    timer--;
    if (timer == 0) {
        Music_Play_Direct(track_id, MPM_TRACKED);
    }
    flags = (flags & 0xFF00) | (timer & 0xFF);
#endif

finish:
    Music_SetTrackFlags(track_id, flags);
}

void Room_ParseFloorData(const int16_t *floor_data)
{
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        for (int32_t j = 0; j < room->size.x * room->size.z; j++) {
            SECTOR *const sector = &room->sectors[j];
            Room_PopulateSectorData(
                sector, floor_data, sector->idx, M_NULL_INDEX);
        }
    }
}

void Room_PopulateSectorData(
    SECTOR *const sector, const int16_t *floor_data, const uint16_t start_index,
    const uint16_t null_index)
{
    sector->floor.type = SURFACE_FLOOR;
    sector->ceiling.type = SURFACE_CEILING;
    sector->floor.tilt = 0;
    sector->ceiling.tilt = 0;
    sector->floor.split.type = SPLIT_NONE;
    sector->ceiling.split.type = SPLIT_NONE;
    sector->floor.is_split = false;
    sector->ceiling.is_split = false;
    sector->portal_room.wall = NO_ROOM;
    sector->is_death_sector = false;
    sector->trigger = nullptr;
    sector->ladder = LADDER_NONE;

    if (start_index == null_index) {
        return;
    }

    const int16_t *data = &floor_data[start_index];
    int16_t fd_entry;
    do {
        fd_entry = *data++;

        switch (M_ENTRY_TYPE(fd_entry)) {
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

        case FT_TRIGGER:
            data = M_ReadTrigger(data, fd_entry, sector);
            break;

        case FT_CLIMB:
            sector->ladder = (LADDER_DIRECTION)M_LADDER_TYPE(fd_entry);
            break;

        case FT_FLOOR_NWSE_SOLID:
        case FT_FLOOR_NESW_SOLID:
        case FT_FLOOR_NWSE_PORTAL_SW:
        case FT_FLOOR_NWSE_PORTAL_NE:
        case FT_FLOOR_NESW_PORTAL_SE:
        case FT_FLOOR_NESW_PORTAL_NW:
            M_ReadTriangulation(&sector->floor, fd_entry, *data++);
            break;

        case FT_ROOF_NWSE_SOLID:
        case FT_ROOF_NESW_SOLID:
        case FT_ROOF_NWSE_PORTAL_SW:
        case FT_ROOF_NWSE_PORTAL_NE:
        case FT_ROOF_NESW_PORTAL_NW:
        case FT_ROOF_NESW_PORTAL_SE:
            M_ReadTriangulation(&sector->ceiling, fd_entry, *data++);
            break;

        default:
            break;
        }
    } while (!M_IS_DONE(fd_entry));
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
            if (dx == 0 && dz == 0) {
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

void Room_TestSectorTrigger(const ITEM *const item, const SECTOR *const sector)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    const bool is_heavy = item->object_id != O_LARA;
    if (!is_heavy) {
        if (sector->is_death_sector && M_TestLava(item)) {
            Lara_TouchLava();
        }

        const LADDER_DIRECTION direction = 1 << Math_GetDirection(item->rot.y);
        lara_info->climb_status = (sector->ladder & direction) == direction;
    }

    const TRIGGER *const trigger = sector->trigger;
    if (trigger == nullptr) {
        return;
    }

    if (g_Camera.type != CAM_HEAVY) {
        Camera_RefreshFromTrigger(trigger);
    }

    ITEM *camera_item = nullptr;
    bool switch_off = false;
    bool flip_map = false;
    bool flip_available = false;
    int32_t new_effect = -1;
    const bool flip_status = Room_GetFlipStatus();

    if (is_heavy) {
        if (trigger->type != TT_HEAVY) {
            return;
        }
    } else {
        switch (trigger->type) {
        case TT_PAD:
        case TT_ANTIPAD:
            if (item->pos.y != item->floor) {
                return;
            }
            break;

        case TT_SWITCH: {
            if (!Switch_Trigger(trigger->item_index, trigger->timer)) {
                return;
            }
            const ITEM *const switch_item = Item_Get(trigger->item_index);
            switch_off = switch_item->current_anim_state == SWITCH_STATE_OFF;
            break;
        }

        case TT_KEY: {
            if (!Keyhole_Trigger(trigger->item_index)) {
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
            if (lara_info->gun_status != LGS_READY) {
                return;
            }
            break;

        default:
            break;
        }
    }

    const TRIGGER_CMD *cmd = trigger->command;
    for (; cmd != nullptr; cmd = cmd->next_cmd) {
        switch (cmd->type) {
        case TO_OBJECT: {
            const int16_t item_num = (int16_t)(intptr_t)cmd->parameter;
            ITEM *const trig_item = Item_Get(item_num);
            if (trig_item->flags & IF_ONE_SHOT) {
                break;
            }

            trig_item->timer = trigger->timer;
            if (trig_item->timer != 1) {
                trig_item->timer *= LOGIC_FPS;
            }

            if (trigger->type == TT_SWITCH) {
                trig_item->flags ^= trigger->mask;
            } else if (
                trigger->type == TT_ANTIPAD
                || trigger->type == TT_ANTITRIGGER) {
                trig_item->flags &= ~trigger->mask;
                if (trigger->one_shot) {
                    trig_item->flags |= IF_ONE_SHOT;
                }
            } else {
                trig_item->flags |= trigger->mask;
            }

            if ((trig_item->flags & IF_CODE_BITS) != IF_CODE_BITS) {
                break;
            }

            if (trigger->one_shot) {
                trig_item->flags |= IF_ONE_SHOT;
            }

            if (trig_item->active) {
                break;
            }

            const OBJECT *const obj = Object_Get(trig_item->object_id);
            if (obj->activate_func != nullptr) {
                obj->activate_func(trig_item);
            } else if (obj->intelligent) {
                if (trig_item->status == IS_INACTIVE) {
                    trig_item->touch_bits = 0;
                    trig_item->status = IS_ACTIVE;
                    Item_AddActive(item_num);
                    LOT_EnableBaddieAI(item_num, true);
                } else if (trig_item->status == IS_INVISIBLE) {
                    trig_item->touch_bits = 0;
                    if (LOT_EnableBaddieAI(item_num, false)) {
                        trig_item->status = IS_ACTIVE;
                    } else {
                        trig_item->status = IS_INVISIBLE;
                    }
                    Item_AddActive(item_num);
                }
            } else {
                trig_item->touch_bits = 0;
                trig_item->status = IS_ACTIVE;
                Item_AddActive(item_num);
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

            g_Camera.timer = LOGIC_FPS * cam_data->timer;

            if (cam_data->one_shot) {
                camera->flags |= IF_ONE_SHOT;
            }

            g_Camera.speed = cam_data->glide + 1;
            g_Camera.type = is_heavy ? CAM_HEAVY : CAM_FIXED;
            break;
        }

        case TO_SINK: {
            const OBJECT_VECTOR *const sink =
                Camera_GetFixedObject((int16_t)(intptr_t)cmd->parameter);

            if (lara_info->lot.required_box != sink->flags) {
                lara_info->lot.target = sink->pos;
                lara_info->lot.required_box = sink->flags;
            }
            lara_info->current_active = sink->data * 6;
            break;
        }

        case TO_FLIPMAP: {
            const int16_t flip_slot = (int16_t)(intptr_t)cmd->parameter;
            int32_t slot_flags = Room_GetFlipSlotFlags(flip_slot);
            flip_available = true;

            if (slot_flags & IF_ONE_SHOT) {
                break;
            }

            if (trigger->type == TT_SWITCH) {
                slot_flags ^= trigger->mask;
            } else {
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
            flip_available = true;

            if ((slot_flags & IF_CODE_BITS) == IF_CODE_BITS && !flip_status) {
                flip_map = true;
            }
            break;
        }

        case TO_FLIPOFF: {
            const int16_t flip_slot = (int16_t)(intptr_t)cmd->parameter;
            const int32_t slot_flags = Room_GetFlipSlotFlags(flip_slot);
            flip_available = true;

            if ((slot_flags & IF_CODE_BITS) == IF_CODE_BITS && flip_status) {
                flip_map = true;
            }
            break;
        }

        case TO_TARGET: {
            const int16_t target_num = (int16_t)(intptr_t)cmd->parameter;
            camera_item = Item_Get(target_num);
            break;
        }

        case TO_FINISH:
            Game_SetIsLevelComplete(true);
            break;

        case TO_FLIPEFFECT:
            new_effect = (int16_t)(intptr_t)cmd->parameter;
            break;

        case TO_CD:
            M_TriggerMusicTrack((MUSIC_ID)(intptr_t)cmd->parameter, trigger);
            break;

        case TO_SECRET: {
            // TODO: implement support for TR1-style secrets as an option
            // in TR2, see #2047
            const int16_t secret_num = (int16_t)(intptr_t)cmd->parameter;
            if (Stats_AddSecret(secret_num)) {
                Music_Play(MX_SECRET, MPM_ALWAYS);
            }
            break;
        }

#if TR_VERSION == 2
        case TO_BODY_BAG:
            Item_ClearKilled();
            break;
#endif

        default:
            break;
        }
    }

    if (camera_item != nullptr
        && (g_Camera.type == CAM_FIXED || g_Camera.type == CAM_HEAVY)) {
        g_Camera.item = camera_item;
    }

    if (flip_map) {
        Room_FlipMap();
    }

    if (new_effect != -1 && (flip_map || !flip_available)) {
        Room_SetFlipEffect(new_effect);
        Room_SetFlipTimer(0);
    }
}
