#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/camera.h>
#include <trx/game/const.h>
#include <trx/game/input.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/objects/traps/movable_block.h>
#include <trx/version.h>

#define M_DEFAULT_BLOCKS_PORTAL ((bool)(g_TRVersion >= 4))

typedef enum {
    M_STATE_CLOSED,
    M_STATE_OPEN,
} M_STATE;

typedef enum {
    M_ANIM_CLOSED = 0,
} M_ANIM;

typedef struct {
    bool auto_open;
    bool blocks_portal;
    struct {
        bool initialised;
        bool on_floor;
        bool on_ceiling;
        int16_t sky;
        int16_t pit;
    } portal;
} M_PRIV;

static const OBJECT_BOUNDS m_FloorTrapdoorBounds = {
    .shift = {
        .min = { .x = -STEP_L, .y = +0, .z = -WALL_L, },
        .max = { .x = +STEP_L, .y = +0, .z = -STEP_L, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
    },
};

static const OBJECT_BOUNDS m_CeilingTrapdoorBounds = {
    .shift = {
        .min = { .x = -STEP_L, .y = +0, .z = -WALL_L * 3 / 4, },
        .max = { .x = +STEP_L, .y = +900, .z = -STEP_L, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
    },
};

static const XYZ_32 m_FloorTrapdoorPosition = { .x = 0, .y = 0, .z = -655 };
static const XYZ_32 m_CeilingTrapdoorPosition = { .x = 0,
                                                  .y = 1056,
                                                  .z = -480 };

static bool M_IsItemOnTop(
    const ITEM *const item, const int32_t x, const int32_t z)
{
    const BOUNDS_16 *const orig_bounds = &Item_GetBestFrame(item)->bounds;
    if (orig_bounds == nullptr) {
        return false;
    }

    BOUNDS_16 fixed_bounds = {};

    // Bounds need to change in order to account for 2 sector trapdoors
    // and the trapdoor angle.
    if (item->rot.y == 0) {
        fixed_bounds.min.x = orig_bounds->min.x;
        fixed_bounds.max.x = orig_bounds->max.x;
        fixed_bounds.min.z = orig_bounds->min.z;
        fixed_bounds.max.z = orig_bounds->max.z;
    } else if (item->rot.y == DEG_90) {
        fixed_bounds.min.x = orig_bounds->min.z;
        fixed_bounds.max.x = orig_bounds->max.z;
        fixed_bounds.min.z = -orig_bounds->max.x;
        fixed_bounds.max.z = -orig_bounds->min.x;
    } else if (item->rot.y == -DEG_180) {
        fixed_bounds.min.x = -orig_bounds->max.x;
        fixed_bounds.max.x = -orig_bounds->min.x;
        fixed_bounds.min.z = -orig_bounds->max.z;
        fixed_bounds.max.z = -orig_bounds->min.z;
    } else if (item->rot.y == -DEG_90) {
        fixed_bounds.min.x = -orig_bounds->max.z;
        fixed_bounds.max.x = -orig_bounds->min.z;
        fixed_bounds.min.z = orig_bounds->min.x;
        fixed_bounds.max.z = orig_bounds->max.x;
    }

    if (x <= item->pos.x + fixed_bounds.max.x
        && x >= item->pos.x + fixed_bounds.min.x
        && z <= item->pos.z + fixed_bounds.max.z
        && z >= item->pos.z + fixed_bounds.min.z) {
        return true;
    }

    return false;
}

static int32_t M_GetFloorHeight(
    const ITEM *const item, const XYZ_32 pos, const int32_t height)
{
    const M_PRIV *const p = item->priv;
    if (p->blocks_portal || !M_IsItemOnTop(item, pos.x, pos.z)) {
        return height;
    } else if (item->current_anim_state != M_STATE_CLOSED) {
        return height;
    } else if (pos.y > item->pos.y || item->pos.y > height) {
        return height;
    } else {
        return item->pos.y;
    }
}

static int32_t M_GetCeilingHeight(
    const ITEM *const item, const XYZ_32 pos, const int32_t height)
{
    const M_PRIV *const p = item->priv;
    if (p->blocks_portal || !M_IsItemOnTop(item, pos.x, pos.z)) {
        return height;
    } else if (item->current_anim_state != M_STATE_CLOSED) {
        return height;
    } else if (pos.y <= item->pos.y || item->pos.y <= height) {
        return height;
    } else {
        return item->pos.y + STEP_L;
    }
}

static BOUNDS_16 M_RotateBounds(const BOUNDS_16 bounds, const int16_t rot_y)
{
    BOUNDS_16 rot_bounds = {};

    switch (rot_y) {
    case 0:
    default:
        rot_bounds = bounds;
        break;
    case DEG_90:
        rot_bounds.min.x = bounds.min.z;
        rot_bounds.max.x = bounds.max.z;
        rot_bounds.min.z = -bounds.max.x;
        rot_bounds.max.z = -bounds.min.x;
        break;
    case -DEG_180:
        rot_bounds.min.x = -bounds.max.x;
        rot_bounds.max.x = -bounds.min.x;
        rot_bounds.min.z = -bounds.max.z;
        rot_bounds.max.z = -bounds.min.z;
        break;
    case -DEG_90:
        rot_bounds.min.x = -bounds.max.z;
        rot_bounds.max.x = -bounds.min.z;
        rot_bounds.min.z = bounds.min.x;
        rot_bounds.max.z = bounds.max.x;
        break;
    }
    return rot_bounds;
}

static void M_GetSectorPositions(const ITEM *const item, VECTOR *sector_pos)
{
    const OBJECT *const obj = Object_Get(item->object_id);
    const ANIM_FRAME *const frame =
        Object_GetAnim(obj, M_ANIM_CLOSED)->frame_ptr;
    const BOUNDS_16 rot_bounds = M_RotateBounds(frame->bounds, item->rot.y);

    const int32_t x0 = item->pos.x + rot_bounds.min.x;
    const int32_t x1 = item->pos.x + rot_bounds.max.x - 1;
    const int32_t z0 = item->pos.z + rot_bounds.min.z;
    const int32_t z1 = item->pos.z + rot_bounds.max.z - 1;

    const int32_t sx0 = Math_FloorDiv(x0, WALL_L);
    const int32_t sx1 = Math_FloorDiv(x1, WALL_L);
    const int32_t sz0 = Math_FloorDiv(z0, WALL_L);
    const int32_t sz1 = Math_FloorDiv(z1, WALL_L);

    for (int32_t sx = sx0; sx <= sx1; ++sx) {
        for (int32_t sz = sz0; sz <= sz1; ++sz) {
            XYZ_32 pos = {
                .x = sx * WALL_L + WALL_L / 2,
                .y = item->pos.y,
                .z = sz * WALL_L + WALL_L / 2,
            };
            Vector_Add(sector_pos, &pos);
        }
    }
}

static void M_EnsurePortals(const ITEM *const item)
{
    M_PRIV *const p = item->priv;
    if (p->portal.initialised) {
        return;
    }

    p->portal.pit = NO_ROOM;
    p->portal.sky = NO_ROOM;

    const ROOM *room = Room_Get(item->room_num);
    const SECTOR *sector = Room_GetWorldSector(room, item->pos.x, item->pos.z);
    if (item->pos.y == room->min_floor && sector->portal_room.pit != NO_ROOM) {
        p->portal.on_floor = true;
        p->portal.pit = sector->portal_room.pit;
        room = Room_Get(p->portal.pit);
        sector = Room_GetWorldSector(room, item->pos.x, item->pos.z);
        p->portal.sky = sector->portal_room.sky;
    } else if (
        item->pos.y == room->max_ceiling
        && sector->portal_room.sky != NO_ROOM) {
        p->portal.on_ceiling = true;
        p->portal.sky = sector->portal_room.sky;
        room = Room_Get(p->portal.sky);
        sector = Room_GetWorldSector(room, item->pos.x, item->pos.z);
        p->portal.pit = sector->portal_room.pit;
    }

    p->portal.initialised = true;
}

static void M_SetPortalsOpen(const ITEM *const item, const bool open)
{
    M_EnsurePortals(item);

    const M_PRIV *const p = item->priv;
    if (!p->blocks_portal) {
        return;
    }

    const int16_t pit = open ? p->portal.pit : NO_ROOM;
    const int16_t sky = open ? p->portal.sky : NO_ROOM;

    const ROOM *room = Room_Get(item->room_num);
    SECTOR *sector = Room_GetWorldSector(room, item->pos.x, item->pos.z);
    if (p->portal.on_floor) {
        sector->portal_room.pit = pit;
        room = Room_Get(p->portal.pit);
        sector = Room_GetWorldSector(room, item->pos.x, item->pos.z);
        sector->portal_room.sky = sky;
    } else if (p->portal.on_ceiling) {
        sector->portal_room.sky = sky;
        room = Room_Get(p->portal.sky);
        sector = Room_GetWorldSector(room, item->pos.x, item->pos.z);
        sector->portal_room.pit = pit;
    }
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_EnsurePortals(item);

    VECTOR *const positions = Vector_Create(sizeof(XYZ_32));
    M_GetSectorPositions(item, positions);
    Walkable_AllocateNodes(item, positions->count);
    Vector_Free(positions);
}

static void M_SetBlocksPortal(ITEM *const item, const TRX_VALUE *const in)
{
    M_PRIV *const p = item->priv;
    if (p->blocks_portal == in->as_bool) {
        return;
    }

    M_SetPortalsOpen(item, true);

    p->blocks_portal = in->as_bool;
    if (p->blocks_portal && item->current_anim_state == M_STATE_CLOSED) {
        M_SetPortalsOpen(item, false);
    }
}

static void M_HandleSave(ITEM *const item, const SAVEGAME_STAGE stage)
{
    if (stage == SAVEGAME_STAGE_AFTER_LOAD
        && item->current_anim_state == M_STATE_OPEN) {
        M_SetPortalsOpen(item, true);
    }
}

static void M_DropStack(const ITEM *const item)
{
    VECTOR *const positions = Vector_Create(sizeof(XYZ_32));
    M_GetSectorPositions(item, positions);
    for (int32_t i = 0; i < positions->count; i++) {
        MovableBlock_DropStack(
            *(const XYZ_32 *)Vector_Get(positions, i), item->room_num);
    }
    Vector_Free(positions);
}

static void M_AddWalkable(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    VECTOR *const positions = Vector_Create(sizeof(XYZ_32));
    M_GetSectorPositions(item, positions);
    for (int32_t i = 0; i < positions->count; i++) {
        Walkable_Add(item_num, *(const XYZ_32 *)Vector_Get(positions, i));
    }
    Vector_Free(positions);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    if (Item_IsTriggerActive(item)) {
        if (item->current_anim_state == M_STATE_CLOSED
            && (p->auto_open || item->goal_anim_state == M_STATE_OPEN)) {
            item->goal_anim_state = M_STATE_OPEN;
            M_SetPortalsOpen(item, true);
            M_DropStack(item);
        }
    } else {
        if (item->current_anim_state == M_STATE_OPEN) {
            item->goal_anim_state = M_STATE_CLOSED;
            M_SetPortalsOpen(item, false);
        }
    }
    Item_Animate(item);
    Item_UpdateRoom(item_num, item->room_num);
}

static void M_OpenManually(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    Item_AddSimulated(item_num);
    item->trigger.mask = TRIGGER_MASK_ALL;
    item->goal_anim_state = M_STATE_OPEN;
    M_SetPortalsOpen(item, true);
}

static void M_AssertCamera(
    const ITEM *const item, const int32_t distance, const int32_t height_shift,
    const bool clamp_to_ceiling)
{
    XYZ_32 cam_pos = XYZ_32_OffsetYaw(item->pos, item->rot.y, -distance);
    cam_pos.y = item->pos.y + height_shift;
    if (clamp_to_ceiling) {
        CLAMPL(cam_pos.y, Room_Get(item->room_num)->max_ceiling);
    }
    Camera_UpdateDynamicFixedObject(cam_pos, item->room_num);
    g_Camera.num = Camera_GetDynamicFixedObjectIdx();
    g_Camera.type = CAM_FIXED;
}

static void M_FloorCollision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();

    if (Lara_Interact_CanControl(LARA_INTERACT_DOOR, item_num)
        && !Item_IsInPlay(item)) {
        if (Lara_TestPosition(item, &m_FloorTrapdoorBounds)) {
            if (Lara_MovePosition(item, &m_FloorTrapdoorPosition)) {
                Item_SwitchToAnim(lara_item, LA(LA_FLOOR_TRAPDOOR_OPEN), 0);
                lara_item->current_anim_state = LS(LS_LIFT_TRAPDOOR);
                Lara_Interact_FinishControl(LARA_INTERACT_DOOR);
                M_OpenManually(item_num);
            } else {
                lara->interact_target.item_num = item_num;
            }
        } else if (Lara_Interact_HasActiveTarget(item_num)) {
            lara->interact_target.is_moving = false;
            lara->gun_status = LGS_ARMLESS;
        }
    }

    if (lara_item->current_anim_state == LS(LS_LIFT_TRAPDOOR)
        && Item_IsInPlay(item) && item->current_anim_state != M_STATE_OPEN) {
        M_AssertCamera(item, WALL_L * 2, -WALL_L * 2, true);
    }
}

static void M_CeilingCollision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();

    if (g_Input.action && !Item_IsInPlay(item)
        && lara_item->current_anim_state == LS(LS_JUMP_UP) && lara_item->gravity
        && lara->gun_status == LGS_ARMLESS
        && Lara_TestPosition(item, &m_CeilingTrapdoorBounds)) {
        Lara_AlignPosition(item, &m_CeilingTrapdoorPosition);
        lara_item->gravity = false;
        lara_item->fall_speed = 0;
        Item_SwitchToAnim(lara_item, LA(LA_CEILING_TRAPDOOR_OPEN), 0);
        lara_item->current_anim_state = LS(LS_PULL_TRAPDOOR);
        Lara_Interact_FinishControl(LARA_INTERACT_DOOR);
        M_OpenManually(item_num);
    }

    if (lara_item->current_anim_state == LS(LS_PULL_TRAPDOOR)
        && Item_IsInPlay(item) && item->current_anim_state != M_STATE_OPEN) {
        M_AssertCamera(item, WALL_L, WALL_L, false);
    }
}

static void M_SetupBase(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->floor_height_func = M_GetFloorHeight;
    obj->ceiling_height_func = M_GetCeilingHeight;
    obj->handle_save_func = M_HandleSave;
    obj->priv_size = sizeof(M_PRIV);
    obj->save_flags = true;
    obj->save_anim = true;
    obj->add_walkable_func = M_AddWalkable;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY(
            M_PRIV, auto_open, true,
            "Whether the trapdoor opens automatically when triggered."),
        OBJECT_PROPERTY_SETTER(
            M_PRIV, blocks_portal, M_DEFAULT_BLOCKS_PORTAL, nullptr,
            M_SetBlocksPortal,
            "Whether the trapdoor blocks any portals it sits on."));
}

static void M_SetupFloor(OBJECT *const obj)
{
    M_SetupBase(obj);
    obj->collision_func = M_FloorCollision;
}

static void M_SetupCeiling(OBJECT *const obj)
{
    M_SetupBase(obj);
    obj->collision_func = M_CeilingCollision;
}

REGISTER_OBJECT(O_TRAPDOOR_1, M_SetupBase)
REGISTER_OBJECT(O_TRAPDOOR_2, M_SetupBase)
REGISTER_OBJECT(O_TRAPDOOR_3, M_SetupBase)
REGISTER_OBJECT(O_FLOOR_TRAPDOOR_1, M_SetupFloor)
REGISTER_OBJECT(O_FLOOR_TRAPDOOR_2, M_SetupFloor)
REGISTER_OBJECT(O_CEILING_TRAPDOOR_1, M_SetupCeiling)
REGISTER_OBJECT(O_CEILING_TRAPDOOR_2, M_SetupCeiling)
