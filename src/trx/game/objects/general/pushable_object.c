#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/input.h>
#include <trx/game/lara.h>
#include <trx/game/objects/traps/movable_block.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>

// clang-format off
#define M_COLL_ROOMS  22
#define M_COLL_REACH  (WALL_L * 2) // = 2048
#define M_COLL_RADIUS (STEP_L * 3 / 2) // = 384
#define M_WALL_GAP    (STEP_L * 5 / 2) // = 640
#define M_GRID_SNAP   (WALL_L / 2) // = 512
#define M_LF_READY    19
// clang-format on

typedef struct {
    XZ_32 origin;
    XZ_32 move_origin;
    int16_t interaction_rot;
    bool playing_move_sfx;
} M_PRIV;

static const OBJECT_BOUNDS m_Bounds = {
    .shift = {
        .min = { .x = -100, .y = -STEP_L, .z = -200, },
        .max = { .x = +100, .y = 0, .z = 0, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
    },
};

static const XYZ_32 m_Position = { .x = 0, .y = 0, .z = -35 };

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "interaction_rot", &p->interaction_rot));
    MUST(JSON_READ_OPT(io, "playing_move_sfx", &p->playing_move_sfx));

    MUST(JSON_PUSH(io, "origin"));
    MUST(JSON_READ_OPT(io, "x", &p->origin.x));
    MUST(JSON_READ_OPT(io, "z", &p->origin.z));
    MUST(JSON_POP(io));

    MUST(JSON_PUSH(io, "move_origin"));
    MUST(JSON_READ_OPT(io, "x", &p->move_origin.x));
    MUST(JSON_READ_OPT(io, "z", &p->move_origin.z));
    MUST(JSON_POP(io));

    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "interaction_rot", p->interaction_rot);
    JSONW_WRITE(io, "playing_move_sfx", p->playing_move_sfx);

    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE(io, "x", p->origin.x);
    JSONW_WRITE(io, "z", p->origin.z);
    JSONW_POP_AND_SET(io, "origin");

    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE(io, "x", p->move_origin.x);
    JSONW_WRITE(io, "z", p->move_origin.z);
    JSONW_POP_AND_SET(io, "move_origin");
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    MovableBlock_UpdateBox(item, true);
}

static bool M_CollideWithItems(const ITEM *const push_item, const XYZ_32 pos)
{
    int16_t coll_rooms[M_COLL_ROOMS];
    const int32_t room_count =
        Room_GetAdjoiningRooms(push_item->room_num, coll_rooms, M_COLL_ROOMS);
    const ITEM *const lara_item = Lara_GetItem();

    int16_t room_num = push_item->room_num;
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    const int32_t height = Room_GetHeight(sector, pos);
    const int32_t y_radius = height == pos.y ? 0 : M_COLL_RADIUS;

    for (int32_t i = 0; i < room_count; i++) {
        const ROOM *const room = Room_Get(coll_rooms[i]);
        int16_t item_num = room->item_num;
        while (item_num != NO_ITEM) {
            const ITEM *const item = Item_Get(item_num);
            item_num = item->next_item;

            if (item == push_item || item == lara_item) {
                continue;
            }

            const OBJECT *const obj = Object_Get(item->object_id);
            if (item->clear_body || !item->is_visible
                || obj->draw_func == nullptr) {
                continue;
            }

            if (!item->is_collidable || obj->collision_func == nullptr) {
                continue;
            }

            if (!XYZ_32_IsNearby(pos, item->pos, M_COLL_REACH)) {
                continue;
            }

            const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
            if (pos.y + y_radius < item->pos.y + bounds->min.y
                || pos.y - y_radius > item->pos.y + bounds->max.y) {
                continue;
            }

            const XYZ_32 delta = XYZ_32_Subtract(pos, item->pos);
            const XYZ_32 local = XYZ_32_RotateYaw(delta, -item->rot.y);
            if (local.x + M_COLL_RADIUS >= bounds->min.x
                && local.z + M_COLL_RADIUS >= bounds->min.z
                && local.x - M_COLL_RADIUS <= bounds->max.x
                && local.z - M_COLL_RADIUS <= bounds->max.z) {
                return true;
            }
        }
    }

    return false;
}

static bool M_TestPush(
    const ITEM *const item, const int32_t block_height,
    const DIRECTION quadrant)
{
    XYZ_32 base_pos = item->pos;
    int16_t room_num = item->room_num;

    switch (quadrant) {
    case DIR_NORTH:
        base_pos.z += WALL_L;
        break;
    case DIR_EAST:
        base_pos.x += WALL_L;
        break;
    case DIR_SOUTH:
        base_pos.z -= WALL_L;
        break;
    case DIR_WEST:
        base_pos.x -= WALL_L;
        break;
    default:
        break;
    }

    base_pos.y -= STEP_L;
    const SECTOR *sector = Room_GetSector(base_pos, &room_num);
    if (sector->stopper) {
        return false;
    }

    if (Room_GetHeight(sector, base_pos) != item->pos.y) {
        return false;
    }

    base_pos.y = item->pos.y;
    sector = Room_GetSector(base_pos, &room_num);
    Room_GetHeight(sector, base_pos);
    if (Room_GetHeightType() != HT_WALL) {
        return false;
    }

    base_pos.y -= block_height - 100;
    sector = Room_GetSector(base_pos, &room_num);
    if (Room_GetCeiling(sector, base_pos) > base_pos.y) {
        return false;
    }

    base_pos.y = item->pos.y;
    COLL_INFO coll = {
        .quadrant = quadrant,
        .radius = M_COLL_RADIUS,
    };
    if (Collide_CollideStaticObjects(&coll, base_pos, room_num, 1000)) {
        return false;
    }
    return !M_CollideWithItems(item, base_pos);
}

static bool M_TestPull(
    const ITEM *const item, const int32_t block_height,
    const DIRECTION quadrant)
{
    XYZ_32 base_pos = item->pos;
    int16_t room_num = item->room_num;

    XZ_32 offset = {};
    switch (quadrant) {
    case DIR_NORTH:
        offset.z = -WALL_L;
        break;
    case DIR_EAST:
        offset.x = -WALL_L;
        break;
    case DIR_SOUTH:
        offset.z = WALL_L;
        break;
    case DIR_WEST:
        offset.x = WALL_L;
        break;
    default:
        break;
    }

    base_pos.x += offset.x;
    base_pos.z += offset.z;
    base_pos.y -= STEP_L;
    const SECTOR *sector = Room_GetSector(base_pos, &room_num);
    if (sector->stopper) {
        return false;
    }

    base_pos.y -= STEP_L;
    if (Room_GetHeight(sector, base_pos) != item->pos.y) {
        return false;
    }

    base_pos.y = item->pos.y - block_height;
    sector = Room_GetSector(base_pos, &room_num);
    if (sector->ceiling.height > base_pos.y) {
        return false;
    }

    base_pos.y = item->pos.y;
    COLL_INFO coll = {
        .quadrant = quadrant,
        .radius = M_COLL_RADIUS,
    };
    if (Collide_CollideStaticObjects(&coll, base_pos, room_num, 1000)) {
        return false;
    }
    if (M_CollideWithItems(item, base_pos)) {
        return false;
    }

    base_pos.x += offset.x;
    base_pos.z += offset.z;
    sector = Room_GetSector(base_pos, &room_num);
    if (Room_GetHeight(sector, base_pos) != item->pos.y) {
        // If Lara is pulling towards a wall or step-up, ensure there is
        // adequate space for her to stand at the destination.
        const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
        const XZ_32 size = {
            .x = ABS(bounds->max.x - bounds->min.x),
            .z = ABS(bounds->max.z - bounds->min.z),
        };
        if (size.x >= M_WALL_GAP || size.z >= M_WALL_GAP) {
            return false;
        }
    }

    const ITEM *const lara_item = Lara_GetItem();
    base_pos = lara_item->pos;
    base_pos.x += offset.x;
    base_pos.y -= STEP_L;
    base_pos.z += offset.z;
    room_num = lara_item->room_num;
    sector = Room_GetSector(base_pos, &room_num);
    if (Room_GetHeight(sector, base_pos) != lara_item->pos.y) {
        return false;
    }

    base_pos.y = lara_item->pos.y - LARA_HEIGHT;
    sector = Room_GetSector(base_pos, &room_num);
    if (Room_GetCeiling(sector, base_pos) > base_pos.y) {
        return false;
    }

    base_pos.y = lara_item->pos.y;
    coll.quadrant = (quadrant + 2) & 3;
    if (Collide_CollideStaticObjects(&coll, base_pos, room_num, LARA_HEIGHT)) {
        return false;
    }

    return !M_CollideWithItems(item, base_pos);
}

static void M_UpdateStoppers(const ITEM *const item, const bool enabled)
{
    const M_PRIV *const p = item->priv;
    int16_t dir = p->interaction_rot;
    if (!enabled) {
        dir += DEG_180;
    }
    const ROOM *room = Room_Get(item->room_num);
    SECTOR *sector = Room_GetWorldSector(room, item->pos.x, item->pos.z);
    sector->stopper = enabled;

    XYZ_32 pos = XYZ_32_OffsetYaw(item->pos, dir, WALL_L);
    pos.y -= STEP_L / 2;
    int16_t room_num = item->room_num;
    Room_GetSector(pos, &room_num);
    room = Room_Get(room_num);
    sector = Room_GetWorldSector(room, pos.x, pos.z);
    sector->stopper = enabled;
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;

    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(
        (XYZ_32) { item->pos.x, item->pos.y - STEP_L, item->pos.z }, &room_num);
    item->pos.y = Room_GetHeight(sector, item->pos);
    if (item->room_num != room_num) {
        Item_UpdateRoom(item_num, room_num);
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (Lara_Interact_CanControl(LARA_INTERACT_PUSHABLE, item_num)) {
        room_num = lara_item->room_num;
        Room_GetSector(
            (XYZ_32) { item->pos.x, item->pos.y - STEP_L, item->pos.z },
            &room_num);
        if (room_num != item->room_num) {
            return;
        }

        const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
        OBJECT_BOUNDS move_bounds = m_Bounds;
        move_bounds.shift.min.x += bounds->min.x;
        move_bounds.shift.max.x += bounds->max.x;
        move_bounds.shift.min.z += bounds->min.z;

        const DIRECTION lara_dir = Math_GetDirection(lara_item->rot.y);
        const DIRECTION item_dir = Math_GetDirection(item->rot.y);
        const XYZ_16 old_rot = item->rot;
        item->rot.y = Math_DirectionToAngle(lara_dir);

        if (Lara_TestPosition(item, &move_bounds)) {
            XYZ_32 move_pos = m_Position;
            const bool lara_is_ns =
                lara_dir == DIR_NORTH || lara_dir == DIR_SOUTH;
            const bool item_is_ns =
                item_dir == DIR_NORTH || item_dir == DIR_SOUTH;
            if (lara_is_ns != item_is_ns) {
                move_pos.z += bounds->min.x;
            } else {
                move_pos.z += bounds->min.z;
            }

            if (Lara_MovePosition(item, &move_pos)) {
                Item_SwitchToAnim(lara_item, LA(LA_PUSHABLE_GRAB), 0);
                lara_item->current_anim_state = LS(LS_PP_READY);
                lara_item->goal_anim_state = LS(LS_PP_READY);
                lara->interact_target.is_moving = false;
                lara->gun_status = LGS_HANDS_BUSY;
            }
            lara->interact_target.item_num = item_num;
        } else if (Lara_Interact_HasActiveTarget(item_num)) {
            lara->interact_target.is_moving = false;
            lara->gun_status = LGS_ARMLESS;
        }

        item->rot = old_rot;
        return;
    }

    if (!Item_TestAnimEqual(lara_item, LA(LA_PUSHABLE_GRAB))
        || !Item_TestFrameEqual(lara_item, M_LF_READY)
        || lara->interact_target.item_num != item_num) {
        Object_Collision(item_num, lara_item, coll);
        return;
    }

    const int16_t quadrant = Math_GetDirection(lara_item->rot.y);
    if (g_Input.forward && M_TestPush(item, WALL_L, quadrant)) {
        p->interaction_rot = lara_item->rot.y;
        lara_item->goal_anim_state = LS(LS_FAST_PUSH_BLOCK);
    } else if (g_Input.back && M_TestPull(item, WALL_L, quadrant)) {
        p->interaction_rot = lara_item->rot.y + DEG_180;
        lara_item->goal_anim_state = LS(LS_FAST_PULL_BLOCK);
    } else {
        return;
    }

    M_UpdateStoppers(item, true);
    MovableBlock_UpdateBox(item, false);
    Item_AddSimulated(item_num);

    lara->head_rot.x = 0;
    lara->head_rot.y = 0;
    lara->torso_rot.x = 0;
    lara->torso_rot.y = 0;
    XYZ_32 lara_pos = {};
    Collide_GetJointAbsPosition(lara_item, &lara_pos, LM_HAND_L);

    p->origin.x = item->pos.x;
    p->origin.z = item->pos.z;
    p->move_origin.x = lara_pos.x;
    p->move_origin.z = lara_pos.z;
}

static void M_SnapToLara(
    ITEM *const item, const XYZ_32 lara_pos, const DIRECTION dir,
    const bool pulling)
{
    int32_t *coord;
    int32_t offset;
    bool passed_target;
    const M_PRIV *const p = item->priv;

    switch (dir) {
    case DIR_NORTH:
        coord = &item->pos.z;
        offset = lara_pos.z + p->origin.z - p->move_origin.z;
        passed_target = pulling ? *coord > offset : *coord < offset;
        break;

    case DIR_EAST:
        coord = &item->pos.x;
        offset = lara_pos.x + p->origin.x - p->move_origin.x;
        passed_target = pulling ? *coord > offset : *coord < offset;
        break;

    case DIR_SOUTH:
        coord = &item->pos.z;
        offset = lara_pos.z + p->origin.z - p->move_origin.z;
        passed_target = pulling ? *coord < offset : *coord > offset;
        break;

    case DIR_WEST:
        coord = &item->pos.x;
        offset = lara_pos.x + p->origin.x - p->move_origin.x;
        passed_target = pulling ? *coord < offset : *coord > offset;
        break;

    default:
        return;
    }

    if (ABS(*coord - offset) < M_GRID_SNAP && passed_target) {
        *coord = offset;
    }
}

static void M_PlaySoundEffects(
    ITEM *const item, const ITEM *const lara_item, const bool pulling)
{
    M_PRIV *const p = item->priv;
    const bool is_move_frame = pulling
        ? (Item_TestFrameRange(lara_item, 30, 67)
           || Item_TestFrameRange(lara_item, 78, 125)
           || Item_TestFrameRange(lara_item, 140, 160))
        : (Item_TestFrameRange(lara_item, 40, 122)
           || Item_TestFrameRange(lara_item, 130, 170));

    if (is_move_frame) {
        Sound_Effect(SFX_PUSHABLE_MOVE, &item->pos, SPM_NORMAL);
        p->playing_move_sfx = true;
    } else if (p->playing_move_sfx) {
        Sound_Effect(SFX_PUSHABLE_STOP, &item->pos, SPM_NORMAL);
        p->playing_move_sfx = false;
    }
}

static void M_Control(const int16_t item_num)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->interact_target.item_num != item_num) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;

    ITEM *const lara_item = Lara_GetItem();
    const LARA_ANIMATION_ID lara_anim = LA_U(Item_GetRelativeAnim(lara_item));

    switch (lara_anim) {
    case LA_FAST_PUSHABLE_PULL:
    case LA_FAST_PUSHABLE_PUSH:
        XYZ_32 lara_pos = {};
        Collide_GetJointAbsPosition(lara_item, &lara_pos, LM_HAND_L);
        const DIRECTION dir = Math_GetDirection(lara_item->rot.y);
        const bool pulling = lara_anim == LA_FAST_PUSHABLE_PULL;
        const int16_t frame = Item_GetRelativeFrame(lara_item);

        M_PlaySoundEffects(item, lara_item, pulling);
        M_SnapToLara(item, lara_pos, dir, pulling);

        lara_item->goal_anim_state = lara_item->current_anim_state;
        if (Item_TestFrameEqual(lara_item, -2)) {
            const bool can_continue = pulling ? M_TestPull(item, WALL_L, dir)
                                              : M_TestPush(item, WALL_L, dir);
            if (lara_item->hit_points <= 0 || !g_Input.action
                || !can_continue) {
                lara_item->goal_anim_state = LS(LS_STOP);
            } else {
                M_UpdateStoppers(item, true);
                M_UpdateStoppers(item, false);
            }
        }
        break;

    case LA_FAST_PUSHABLE_PULL_STOP:
    case LA_FAST_PUSHABLE_PUSH_STOP:
        if (Item_TestFrameEqual(lara_item, 0)) {
            item->pos.x = (item->pos.x & -M_GRID_SNAP) | M_GRID_SNAP;
            item->pos.z = (item->pos.z & -M_GRID_SNAP) | M_GRID_SNAP;
        } else if (Item_TestFrameEqual(lara_item, -1)) {
            Item_SetFinished(item, false);
            Item_RemoveSimulated(item_num);
            Room_TestTriggers(item);
            M_UpdateStoppers(item, false);
            MovableBlock_UpdateBox(item, true);
            lara->interact_target.item_num = NO_ITEM;
        }
        break;

    default:
        break;
    }
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_Initialise;
    obj->collision_func = M_Collision;
    obj->control_func = M_Control;

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;

    obj->save_flags = true;
    obj->save_position = true;
}

REGISTER_OBJECT(O_PUSHABLE_OBJECT_1, M_Setup)
REGISTER_OBJECT(O_PUSHABLE_OBJECT_2, M_Setup)
REGISTER_OBJECT(O_PUSHABLE_OBJECT_3, M_Setup)
REGISTER_OBJECT(O_PUSHABLE_OBJECT_4, M_Setup)
REGISTER_OBJECT(O_PUSHABLE_OBJECT_5, M_Setup)
