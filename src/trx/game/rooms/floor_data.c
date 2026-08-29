#include <trx/game/rooms/floor_data.h>

#include <trx/config.h>
#include <trx/game/camera.h>
#include <trx/game/game_buf.h>
#include <trx/game/gym.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/level/settings.h>
#include <trx/game/objects/general/keyhole.h>
#include <trx/game/objects/general/pickup.h>
#include <trx/game/objects/general/switch.h>
#include <trx/game/rooms.h>
#include <trx/version.h>

#define M_NULL_INDEX 0
#define M_IS_DONE(t) ((t & 0x8000) == 0x8000)

#define M_ENTRY_TYPE(t) (t & 0x1F)
#define M_TRIG_TYPE(t) ((t & 0x7F00) >> 8)
#define M_TRIG_TIMER(t) (t & 0xFF)
#define M_TRIG_ONE_SHOT(t) ((t & 0x100) == 0x100)
#define M_TRIG_MASK(t) ((t & 0x3E00) >> TRIGGER_MASK_SHIFT)
#define M_TRIG_CMD_TYPE(t) ((t & 0x7C00) >> 10)
#define M_TRIG_CMD_ARG(t) (t & 0x3FF)
#define M_TRIG_CAM_GLIDE(t) ((t & 0x3E00) >> 6)
#define M_LADDER_TYPE(t) ((t & 0x7F00) >> 8)

static const LARA_STATE_ID m_MonkeyStates[] = {
    // clang-format off
    LS_MONKEY_IDLE,
    LS_MONKEY_FORWARD,
    LS_MONKEY_LEFT,
    LS_MONKEY_RIGHT,
    LS_MONKEY_ROLL,
    LS_MONKEY_TURN_LEFT,
    LS_MONKEY_TURN_RIGHT,
    NO_CATALOG_ID,
    // clang-format on
};

static const LARA_STATE_ID m_CrouchStates[] = {
    // clang-format off
    LS_CROUCH_IDLE,
    LS_CROUCH_ROLL,
    LS_CROUCH_TURN_LEFT,
    LS_CROUCH_TURN_RIGHT,
    LS_CRAWL_IDLE,
    LS_CRAWL_FORWARD,
    LS_CRAWL_TURN_LEFT,
    LS_CRAWL_TURN_RIGHT,
    NO_CATALOG_ID,
    // clang-format on
};

static const LARA_STATE_ID m_ClimbStates[] = {
    // clang-format off
    LS_HANG,
    LS_CLIMB_STANCE,
    LS_CLIMBING,
    LS_CLIMB_LEFT,
    LS_CLIMB_END,
    LS_CLIMB_RIGHT,
    LS_CLIMB_DOWN,
    LS_MONKEY_IDLE,
    NO_CATALOG_ID,
    // clang-format on
};

static void (*m_Handlers[TO_NUMBER_OF])(
    const TRIGGER *trigger, const TRIGGER_CMD *cmd,
    TRIGGER_STATUS *status) = {};

static const int16_t *M_ReadTrigger(
    const int16_t *data, const int16_t fd_entry, SECTOR *const sector)
{
    TRIGGER *const trigger = GameBuf_Alloc(sizeof(TRIGGER), GBUF_FLOOR_DATA);

    const int16_t trig_setup = *data++;
    trigger->enabled = true;
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

    if (sector->trigger != nullptr) {
        // Some old TRLEs have incorrectly formatted floor data, with multiple
        // trigger entries defined where regular triggers overlap dummies.
        // Ignore the data as dummy triggers are no longer essential.
        int16_t command;
        do {
            command = *data++;
            const TRIGGER_OBJECT type = M_TRIG_CMD_TYPE(command);
            if (type == TO_CAMERA || type == TO_FLYBY) {
                command = *data++;
            }
        } while (!M_IS_DONE(command));
        goto finish;
    }

    sector->trigger = trigger;
    sector->trigger->command =
        GameBuf_Alloc(sizeof(TRIGGER_CMD), GBUF_FLOOR_DATA);
    TRIGGER_CMD *cmd = sector->trigger->command;

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
        } else if (cmd->type == TO_FLYBY) {
            TRIGGER_FLYBY_DATA *const flyby_data =
                GameBuf_Alloc(sizeof(TRIGGER_FLYBY_DATA), GBUF_FLOOR_DATA);
            cmd->parameter = (void *)flyby_data;
            flyby_data->sequence_num = M_TRIG_CMD_ARG(command);
            command = *data++;
            flyby_data->one_shot = M_TRIG_ONE_SHOT(command);
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

finish:
    return data;
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
    const SECTOR *const sector = Room_GetSector(
        (XYZ_32) { item->pos.x, MAX_HEIGHT, item->pos.z }, &room_num);
    return sector->is_death_sector;
}

static int32_t M_DecodeSplit(int32_t height)
{
    // Triangles can be 15 clicks high at most; bit 0x10 is used to wrap to
    // negative via manual sign extension. OG used 0xFFF0 for 16-bit.
    if ((height & 0x10) != 0) {
        height |= 0xFFFFFFF0;
    }
    return height << 8;
}

// Anything other than the player trips triggers as a heavy object.
static bool M_IsHeavy(const ITEM *const item)
{
    return item->object_id != O_LARA;
}

static bool M_TestSectorTrigger(
    const ITEM *const item, const SECTOR *const sector, const bool is_heavy)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    // The non-heavy pass runs for the flyby camera as well, and a level it
    // plays in need not hold a player for these to read.
    const ITEM *const lara_item = Lara_GetItem();
    if (!is_heavy && lara_item != nullptr) {
        if (sector->is_death_sector && M_TestLava(item)) {
            Lara_TouchDeathSector(Level_GetDeathTile());
        }

        const LADDER_DIRECTION direction = 1 << Math_GetDirection(item->rot.y);
        lara_info->climb_status = (sector->ladder & direction) == direction;
    }

    const TRIGGER *const trigger = sector->trigger;
    if (trigger == nullptr || !trigger->enabled) {
        return false;
    }

    if (g_Camera.type != CAM_HEAVY) {
        Camera_RefreshFromTrigger(trigger);
    }

    TRIGGER_STATUS status = {
        .is_heavy = is_heavy,
        .heavy_mask = trigger->mask,
        .camera_item = nullptr,
        .switch_off = false,
        .flip_group = -1,
        .flip_available = false,
        .new_effect = -1,
    };

    if (is_heavy) {
        switch (trigger->type) {
        case TT_HEAVY:
        case TT_HEAVY_ANTITRIGGER:
            break;
        case TT_HEAVY_SWITCH:
            const int32_t item_mask = item->trigger.mask;
            if (item_mask == 0) {
                return false;
            }
            if (item_mask != status.heavy_mask) {
                return false;
            }
            break;
        default:
            return false;
        }
    } else {
        switch (trigger->type) {
        case TT_PAD:
        case TT_ANTIPAD:
            if (!Gym_TrackManager_OnPadContact(GYM_TRACK_ASSAULT, false)) {
                return false;
            }
            // A pad answers to the player standing on it, whoever asked.
            if (lara_item == nullptr || lara_item->pos.y != lara_item->floor) {
                return false;
            }
            if (item->object_id == O_LARA
                && !Gym_TrackManager_OnPadContact(GYM_TRACK_ASSAULT, true)) {
                return false;
            }
            break;

        case TT_SWITCH: {
            const bool switch_result =
                Switch_Trigger(trigger->item_index, trigger->timer);
            ITEM *const switch_item = Item_Get(trigger->item_index);
            const bool intelligent =
                Object_Get(switch_item->object_id)->intelligent;
            if (!intelligent && g_TRVersion >= 3 && trigger->one_shot) {
                switch_item->trigger.switch_spent = true;
            }
            if (!switch_result) {
                return false;
            }
            status.switch_off = !intelligent
                && switch_item->current_anim_state == SWITCH_STATE_OFF;
            break;
        }

        case TT_KEY: {
            if (!Keyhole_Trigger(trigger->item_index)) {
                return false;
            }
            break;
        }

        case TT_PICKUP: {
            if (!Pickup_Trigger(trigger->item_index)) {
                return false;
            }
            break;
        }

        case TT_HEAVY:
        case TT_DUMMY:
        case TT_HEAVY_SWITCH:
        case TT_HEAVY_ANTITRIGGER:
            return false;

        case TT_COMBAT:
            if (lara_info->gun_status != LGS_READY) {
                return false;
            }
            break;

        case TT_MONKEY:
            if (!Lara_HasState(m_MonkeyStates)) {
                return false;
            }
            break;

        case TT_CROUCH:
            if (!Lara_HasState(m_CrouchStates)) {
                return false;
            }
            break;

        case TT_CLIMB:
            if (!Lara_HasState(m_ClimbStates)) {
                return false;
            }
            break;

        default:
            break;
        }
    }

    for (const TRIGGER_CMD *cmd = trigger->command; cmd != nullptr;
         cmd = cmd->next_cmd) {
        // The command type is a 5-bit field, so it can name a type this
        // engine has no handler for.
        if (cmd->type < TO_NUMBER_OF && m_Handlers[cmd->type] != nullptr) {
            m_Handlers[cmd->type](trigger, cmd, &status);
        }
    }

    if (status.camera_item != nullptr
        && (g_Camera.type == CAM_FIXED || g_Camera.type == CAM_HEAVY)) {
        g_Camera.item = status.camera_item;
    }

    if (status.flip_group >= 0) {
        Room_FlipMap(status.flip_group);
    }

    // A flip and an effect on the one trigger: the effect runs only if the
    // flip happened, or if nothing on the trigger could flip at all.
    if (status.new_effect != -1
        && (status.flip_group >= 0 || !status.flip_available)) {
        if (!ItemAction_Intercept(
                status.new_effect, trigger->timer, Item_GetIndex(item))) {
            Room_SetFlipEffect(status.new_effect);
            Room_SetFlipTimer(0);
        }
    }

    return true;
}

void Room_ParseFloorData(
    const int16_t *floor_data, const int32_t floor_data_size)
{
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        for (int32_t j = 0; j < room->size.x * room->size.z; j++) {
            SECTOR *const sector = &room->sectors[j];
            Room_PopulateSectorData(
                sector, floor_data, floor_data_size, sector->idx, M_NULL_INDEX);
        }
    }
}

void Room_PopulateSectorData(
    SECTOR *const sector, const int16_t *floor_data,
    const int32_t floor_data_size, const uint16_t start_index,
    const uint16_t null_index)
{
    sector->floor.type = SURFACE_FLOOR;
    sector->ceiling.type = SURFACE_CEILING;
    sector->floor.tilt = (XZ_16) {};
    sector->ceiling.tilt = (XZ_16) {};
    sector->floor.split.type = SPLIT_NONE;
    sector->ceiling.split.type = SPLIT_NONE;
    sector->floor.is_split = false;
    sector->ceiling.is_split = false;
    sector->portal_room.wall = NO_ROOM;
    sector->is_death_sector = false;
    sector->trigger = nullptr;
    sector->ladder = LADDER_NONE;
    sector->mine_cart_type = MINE_CART_NONE;

    if (start_index == null_index || floor_data == nullptr
        || start_index >= floor_data_size) {
        return;
    }

#define L_TILT(tilt)                                                           \
    do {                                                                       \
        const int16_t tilt_value = *data++;                                    \
        tilt.x = (int8_t)tilt_value;                                           \
        tilt.z = tilt_value >> 8;                                              \
    } while (false)

    const int16_t *data = &floor_data[start_index];
    int16_t fd_entry;
    do {
        fd_entry = *data++;

        switch (M_ENTRY_TYPE(fd_entry)) {
        case FT_TILT:
            L_TILT(sector->floor.tilt);
            break;

        case FT_ROOF:
            L_TILT(sector->ceiling.tilt);
            break;

        case FT_DOOR:
            const int16_t portal_room = *data++;
            if (sector->portal_room.wall == NO_ROOM) {
                sector->portal_room.wall = portal_room;
            }
            break;

        case FT_LAVA:
            sector->is_death_sector = true;
            break;

        case FT_TRIGGER:
            data = M_ReadTrigger(data, fd_entry, sector);
            break;

        case FT_CLIMB:
            sector->ladder |= (LADDER_DIRECTION)M_LADDER_TYPE(fd_entry);
            break;

        case FT_MONKEY:
            sector->ladder |= LADDER_CEILING;
            break;

        case FT_FLOOR_NWSE_SOLID:
        case FT_FLOOR_NESW_SOLID:
        case FT_FLOOR_NWSE_PORTAL_SW:
        case FT_FLOOR_NWSE_PORTAL_NE:
        case FT_FLOOR_NESW_PORTAL_SE:
        case FT_FLOOR_NESW_PORTAL_NW:
            Room_ReadTriangulation(&sector->floor, fd_entry, *data++);
            break;

        case FT_ROOF_NWSE_SOLID:
        case FT_ROOF_NESW_SOLID:
        case FT_ROOF_NWSE_PORTAL_SW:
        case FT_ROOF_NWSE_PORTAL_NE:
        case FT_ROOF_NESW_PORTAL_NW:
        case FT_ROOF_NESW_PORTAL_SE:
            Room_ReadTriangulation(&sector->ceiling, fd_entry, *data++);
            break;

        case FT_MINE_CART_LEFT:
            sector->mine_cart_type = MINE_CART_LEFT;
            break;

        case FT_MINE_CART_RIGHT:
            sector->mine_cart_type = sector->mine_cart_type == MINE_CART_LEFT
                ? MINE_CART_STOP
                : MINE_CART_RIGHT;
            break;

        default:
            break;
        }
    } while (!M_IS_DONE(fd_entry));

#undef L_TILT
}

void Room_ReadTriangulation(
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
    const int32_t split_1 = (func_data & 0x03E0) >> 5;
    const int32_t split_2 = (func_data & 0x7C00) >> 10;
    surface->split.h1 = M_DecodeSplit(split_1);
    surface->split.h2 = M_DecodeSplit(split_2);

    for (int32_t i = 0; i < 4; i++) {
        surface->split.tilts[i] = (tilt_data >> (i * 4)) & 0xF;
        if (surface->type == SURFACE_CEILING) {
            surface->split.tilts[i] *= -1;
        }
    }
}

void Room_RegisterTriggerHandler(
    const TRIGGER_OBJECT type,
    void (*const handle_func)(
        const TRIGGER *trigger, const TRIGGER_CMD *cmd, TRIGGER_STATUS *status))
{
    m_Handlers[type] = handle_func;
}

bool Room_TestTriggers(const ITEM *const item)
{
    return Room_TestTriggersEx(item, M_IsHeavy(item));
}

bool Room_TestTriggersEx(const ITEM *const item, const bool is_heavy)
{
    int16_t room_num = item->room_num;
    const SECTOR *sector = Room_GetSector(
        (XYZ_32) { item->pos.x, MAX_HEIGHT, item->pos.z }, &room_num);

    bool result = M_TestSectorTrigger(item, sector, is_heavy);
    if (item->object_id != O_TORSO) {
        return result;
    }

    for (int32_t dx = -1; dx < 2; dx++) {
        for (int32_t dz = -1; dz < 2; dz++) {
            if (dx == 0 && dz == 0) {
                continue;
            }

            room_num = item->room_num;
            sector = Room_GetSector(
                (XYZ_32) {
                    item->pos.x + dx * WALL_L,
                    MAX_HEIGHT,
                    item->pos.z + dz * WALL_L,
                },
                &room_num);
            result |= M_TestSectorTrigger(item, sector, is_heavy);
        }
    }

    return result;
}

bool Room_TestSectorTrigger(const ITEM *const item, const SECTOR *const sector)
{
    return M_TestSectorTrigger(item, sector, M_IsHeavy(item));
}

bool Room_IsAntiTrigger(const TRIGGER_TYPE type)
{
    return type == TT_ANTIPAD || type == TT_ANTITRIGGER
        || type == TT_HEAVY_ANTITRIGGER;
}
