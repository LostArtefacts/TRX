#include "game/room.h"

#include "game/box.h"
#include "game/camera.h"
#include "game/items.h"
#include "game/lara/misc.h"
#include "game/lot.h"
#include "game/music.h"
#include "game/objects/general/keyhole.h"
#include "game/objects/general/pickup.h"
#include "game/objects/general/switch.h"
#include "game/shell.h"
#include "global/const.h"
#include "global/utils.h"
#include "global/vars.h"

#include <libtrx/debug.h>
#include <libtrx/game/game_buf.h>
#include <libtrx/game/math.h>
#include <libtrx/utils.h>

void Room_MarkToBeDrawn(int16_t room_num);
static int16_t M_GetFloorTiltHeight(const SECTOR *sector, int32_t x, int32_t z);
static int16_t M_GetCeilingTiltHeight(
    const SECTOR *sector, int32_t x, int32_t z);

static void M_TriggerMusicTrack(int16_t track, const TRIGGER *trigger);
static bool M_TestLava(const ITEM *item);

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

static void M_TriggerMusicTrack(
    const int16_t track, const TRIGGER *const trigger)
{
    if (track < MX_CUTSCENE_THE_GREAT_WALL || track >= MX_TITLE_THEME) {
        return;
    }

    uint16_t flags = Music_GetTrackFlags(track);
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
        Music_Play(track, MPM_TRACKED);
        goto finish;
    }

    if (track != Music_GetDelayedTrack()) {
        Music_Play(track, MPM_DELAYED);
        flags =
            (flags & 0xFF00) | ((FRAMES_PER_SECOND * trigger->timer) & 0xFF);
        goto finish;
    }

    int32_t timer = flags & 0xFF;
    if (timer == 0) {
        goto finish;
    }

    timer--;
    if (timer == 0) {
        Music_Play(track, MPM_TRACKED);
    }
    flags = (flags & 0xFF00) | (timer & 0xFF);

finish:
    Music_SetTrackFlags(track, flags);
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
    if (!is_heavy) {
        if (sector->is_death_sector && M_TestLava(item)) {
            Lara_TouchLava((ITEM *)item);
        }

        const LADDER_DIRECTION direction = 1 << Math_GetDirection(item->rot.y);
        g_Lara.climb_status = (sector->ladder & direction) == direction;
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
            switch_off = switch_item->current_anim_state == LS_RUN;
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
            if (g_Lara.gun_status != LGS_READY) {
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
                trig_item->timer *= FRAMES_PER_SECOND;
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
            if (obj->intelligent) {
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

            g_Camera.timer = FRAMES_PER_SECOND * cam_data->timer;

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

            if (!g_Lara.creature) {
                LOT_EnableBaddieAI(g_Lara.item_num, true);
            }

            g_Lara.creature->lot.target = sink->pos;
            g_Lara.creature->lot.required_box = sink->flags;
            g_Lara.current_active = sink->data * 6;
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
            g_LevelComplete = true;
            break;

        case TO_FLIPEFFECT:
            new_effect = (int16_t)(intptr_t)cmd->parameter;
            break;

        case TO_CD:
            M_TriggerMusicTrack((int16_t)(intptr_t)cmd->parameter, trigger);
            break;

        case TO_BODY_BAG:
            Item_ClearKilled();
            break;

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

int32_t Room_FindGridShift(int32_t src, const int32_t dst)
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

void Room_GetNearbyRooms(
    const int32_t x, const int32_t y, const int32_t z, const int32_t r,
    const int32_t h, const int16_t room_num)
{
    Room_DrawReset();
    Room_MarkToBeDrawn(room_num);

    Room_GetNewRoom(r + x, y, r + z, room_num);
    Room_GetNewRoom(x - r, y, r + z, room_num);
    Room_GetNewRoom(r + x, y, z - r, room_num);
    Room_GetNewRoom(x - r, y, z - r, room_num);
    Room_GetNewRoom(r + x, y - h, r + z, room_num);
    Room_GetNewRoom(x - r, y - h, r + z, room_num);
    Room_GetNewRoom(r + x, y - h, z - r, room_num);
    Room_GetNewRoom(x - r, y - h, z - r, room_num);
}

void Room_GetNewRoom(
    const int32_t x, const int32_t y, const int32_t z, int16_t room_num)
{
    Room_GetSector(x, y, z, &room_num);
    Room_MarkToBeDrawn(room_num);
}

int16_t Room_GetTiltType(
    const SECTOR *sector, const int32_t x, const int32_t y, const int32_t z)
{
    sector = Room_GetPitSector(sector, x, z);

    if ((y + STEP_L * 2) < sector->floor.height) {
        return 0;
    }

    return sector->floor.tilt;
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

SECTOR *Room_GetSkySector(
    const SECTOR *sector, const int32_t x, const int32_t z)
{
    while (sector->portal_room.sky != NO_ROOM) {
        const ROOM *const room = Room_Get(sector->portal_room.sky);
        sector = Room_GetWorldSector(room, x, z);
    }

    return (SECTOR *)sector;
}

SECTOR *Room_GetSector(
    const int32_t x, const int32_t y, const int32_t z, int16_t *const room_num)
{
    SECTOR *sector = nullptr;

    while (true) {
        const ROOM *room = Room_Get(*room_num);
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
        *room_num = sector->portal_room.wall;
    }

    ASSERT(sector != nullptr);

    if (y >= sector->floor.height) {
        while (sector->portal_room.pit != NO_ROOM) {
            *room_num = sector->portal_room.pit;
            const ROOM *const room = Room_Get(*room_num);
            sector = Room_GetWorldSector(room, x, z);
            if (y < sector->floor.height) {
                break;
            }
        }
    } else if (y < sector->ceiling.height) {
        while (sector->portal_room.sky != NO_ROOM) {
            *room_num = sector->portal_room.sky;
            const ROOM *const room = Room_Get(sector->portal_room.sky);
            sector = Room_GetWorldSector(room, x, z);
            if (y >= sector->ceiling.height) {
                break;
            }
        }
    }

    return sector;
}

int32_t Room_GetWaterHeight(
    const int32_t x, const int32_t y, const int32_t z, int16_t room_num)
{
    const SECTOR *sector = nullptr;
    const ROOM *room = nullptr;

    do {
        room = Room_Get(room_num);
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
        room_num = sector->portal_room.wall;
    } while (room_num != NO_ROOM);

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

int32_t Room_GetHeight(
    const SECTOR *sector, const int32_t x, const int32_t y, const int32_t z)
{
    g_HeightType = 0;

    const SECTOR *const pit_sector = Room_GetPitSector(sector, x, z);
    int32_t height = pit_sector->floor.height;

    if (g_GF_NoFloor && g_GF_NoFloor == height) {
        height = 0x4000;
    } else {
        height = M_GetFloorTiltHeight(pit_sector, x, z);
    }

    if (pit_sector->trigger == nullptr) {
        return height;
    }

    const TRIGGER_CMD *cmd = pit_sector->trigger->command;
    for (; cmd != nullptr; cmd = cmd->next_cmd) {
        if (cmd->type != TO_OBJECT) {
            continue;
        }

        const ITEM *const item = Item_Get((int16_t)(intptr_t)cmd->parameter);
        const OBJECT *const obj = Object_Get(item->object_id);
        if (obj->floor) {
            obj->floor(item, x, y, z, &height);
        }
    }

    return height;
}

void Room_Legacy_TestTriggers(const int16_t *fd, bool heavy)
{
    ASSERT_FAIL();
}

void Room_TestTriggers(const ITEM *const item)
{
    int16_t room_num = item->room_num;
    const SECTOR *sector =
        Room_GetSector(item->pos.x, MAX_HEIGHT, item->pos.z, &room_num);

    Room_TestSectorTrigger(item, sector);
}

int32_t Room_GetCeiling(
    const SECTOR *const sector, const int32_t x, const int32_t y,
    const int32_t z)
{
    const SECTOR *const sky_sector = Room_GetSkySector(sector, x, z);
    int32_t height = M_GetCeilingTiltHeight(sky_sector, x, z);

    const SECTOR *const pit_sector = Room_GetPitSector(sector, x, z);
    if (pit_sector->trigger == nullptr) {
        return height;
    }

    const TRIGGER_CMD *cmd = pit_sector->trigger->command;
    for (; cmd != nullptr; cmd = cmd->next_cmd) {
        if (cmd->type != TO_OBJECT) {
            continue;
        }

        const ITEM *const item = Item_Get((int16_t)(intptr_t)cmd->parameter);
        const OBJECT *const obj = Object_Get(item->object_id);
        if (obj->ceiling) {
            obj->ceiling(item, x, y, z, &height);
        }
    }

    return height;
}

int16_t Room_Legacy_GetDoor(const SECTOR *const sector)
{
    ASSERT_FAIL();
    return NO_ROOM;
}

void Room_AlterFloorHeight(const ITEM *const item, const int32_t height)
{
    int16_t room_num = item->room_num;

    SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    const SECTOR *ceiling = Room_GetSector(
        item->pos.x, item->pos.y + height - WALL_L, item->pos.z, &room_num);

    if (sector->floor.height == NO_HEIGHT) {
        sector->floor.height = ceiling->ceiling.height + ROUND_TO_CLICK(height);
    } else {
        sector->floor.height += ROUND_TO_CLICK(height);
        if (sector->floor.height == ceiling->ceiling.height) {
            sector->floor.height = NO_HEIGHT;
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

void Room_FlipMap(void)
{
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        ROOM *const room = Room_Get(i);
        if (room->flipped_room == NO_ROOM_NEG) {
            continue;
        }

        Room_RemoveFlipItems(room);

        ROOM *const flipped = Room_Get(room->flipped_room);
        const ROOM temp = *room;
        *room = *flipped;
        *flipped = temp;

        room->flipped_room = flipped->flipped_room;
        flipped->flipped_room = NO_ROOM_NEG;
        room->flip_status = RFS_UNFLIPPED;
        flipped->flip_status = RFS_FLIPPED;

        // TODO: is this really necessary given the assignments above?
        room->item_num = flipped->item_num;
        room->effect_num = flipped->effect_num;

        Room_AddFlipItems(room);
    }

    Room_ToggleFlipStatus();
}

void Room_RemoveFlipItems(const ROOM *const room)
{
    int16_t item_num = room->item_num;

    while (item_num != NO_ITEM) {
        ITEM *const item = Item_Get(item_num);

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

        if (item->flags & IF_ONE_SHOT
            && Object_Get(item->object_id)->intelligent
            && item->hit_points <= 0) {
            Item_RemoveDrawn(item_num);
            item->flags |= IF_KILLED;
        }

        item_num = item->next_item;
    }
}

void Room_AddFlipItems(const ROOM *const room)
{
    int16_t item_num = room->item_num;
    while (item_num != NO_ITEM) {
        const ITEM *const item = Item_Get(item_num);

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

void Room_Legacy_TriggerMusicTrack(
    const int16_t track, const int16_t flags, const int16_t type)
{
    ASSERT_FAIL();
}

void Room_InitCinematic(void)
{
    const int32_t room_count = Room_GetCount();
    for (int32_t i = 0; i < room_count; i++) {
        ROOM *const room = Room_Get(i);
        if (room->flipped_room != NO_ROOM_NEG) {
            Room_Get(room->flipped_room)->bound_active = 1;
        }
        room->flags |= RF_OUTSIDE;
    }

    Room_DrawReset();
    for (int32_t i = 0; i < room_count; i++) {
        if (!Room_Get(i)->bound_active) {
            Room_MarkToBeDrawn(i);
        }
    }
}
