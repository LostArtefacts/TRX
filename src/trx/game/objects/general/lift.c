#include <trx/config.h>
#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/math.h>
#include <trx/core/strings.h>
#include <trx/core/vector.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/objects/traps/movable_block.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_DEFAULT_WAIT_TIME   3
#define M_DEFAULT_TRAVEL_DIST 22
#define M_DEFAULT_SPEED       16
#define M_MAXIMUM_SPEED       64
#define M_HEIGHT              (STEP_L * 5) // = 1280
#define M_WALL_WIDTH          (STEP_L / 6) // = 42
#define M_WALL_COLOR          ((RGBA_8888) { 0, 255, 255, 255 })
#define M_NUM_FLOOR_SECTORS   4
#define M_NUM_SECTORS         (M_NUM_FLOOR_SECTORS * 2)
// clang-format on

typedef enum {
    M_STATE_DOOR_CLOSED,
    M_STATE_DOOR_OPEN,
} M_STATE;

typedef enum {
    M_ANIM_CLOSED = 0,
} M_ANIM;

typedef enum {
    M_LARA_ABOVE,
    M_LARA_INSIDE,
    M_LARA_BELOW,
} M_LARA_STATUS;

typedef struct {
    int32_t start_height;
    int32_t wait_timer;
    int32_t wait_time;
    int32_t travel_distance;
    int32_t speed;
    bool is_moving;
    GAME_VECTOR linked[M_NUM_SECTORS];
    BOUNDS_16 wall_bounds;
} M_PRIV;

static const LARA_TRX_STATE m_ClimbingStates[] = {
    // clang-format off
    LS_CLIMB_STANCE,
    LS_CLIMBING,
    LS_CLIMB_LEFT,
    LS_CLIMB_END,
    LS_CLIMB_RIGHT,
    LS_CLIMB_DOWN,
    LS_HANG,
    LS_SHIMMY_LEFT,
    LS_SHIMMY_RIGHT,
    LS_SHIMMY_OUTER_LEFT,
    LS_SHIMMY_OUTER_RIGHT,
    LS_SHIMMY_INNER_LEFT,
    LS_SHIMMY_INNER_RIGHT,
    LS_TRX_INVALID, // sentinel
    // clang-format on
};

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "start_height", &p->start_height));
    MUST(JSON_READ_OPT(io, "wait_time", &p->wait_timer));
    MUST(JSON_READ_OPT(io, "is_moving", &p->is_moving));
    for (int32_t i = 0; i < M_NUM_SECTORS; i++) {
        const char *const key = String_FormatStatic("linked_%d", i);
        if (SHOULD(JSON_PUSH(io, key))) {
            MUST(JSON_READ_OPT(io, "x", &p->linked[i].pos.x));
            MUST(JSON_READ_OPT(io, "y", &p->linked[i].pos.y));
            MUST(JSON_READ_OPT(io, "z", &p->linked[i].pos.z));
            MUST(JSON_POP(io));
        }
    }
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "start_height", p->start_height);
    JSONW_WRITE(io, "wait_time", p->wait_timer);
    JSONW_WRITE(io, "is_moving", p->is_moving);
    for (int32_t i = 0; i < M_NUM_SECTORS; i++) {
        const char *const key = String_FormatStatic("linked_%d", i);
        JSONW_PUSH_OBJECT(io);
        JSONW_WRITE(io, "x", p->linked[i].pos.x);
        JSONW_WRITE(io, "y", p->linked[i].pos.y);
        JSONW_WRITE(io, "z", p->linked[i].pos.z);
        JSONW_POP_AND_SET(io, key);
    }
}

static XZ_32 M_GetTile(const XYZ_32 pos)
{
    return (XZ_32) {
        .x = pos.x >> WALL_SHIFT,
        .z = pos.z >> WALL_SHIFT,
    };
}

static XZ_32 M_GetShaftOffset(const int16_t angle)
{
    const DIRECTION direction = Math_GetDirection(angle);
    switch (direction) {
        // clang-format off
    case DIR_NORTH: return (XZ_32) { -1, +1 };
    case DIR_EAST:  return (XZ_32) { +1, +1 };
    case DIR_SOUTH: return (XZ_32) { +1, -1 };
    case DIR_WEST:  return (XZ_32) { -1, -1 };
    default:        return (XZ_32) { +0, +0 };
        // clang-format on
    }
}

static bool M_IsTileInShaft(
    const XZ_32 tile, const XZ_32 shaft_tile, const XZ_32 offset)
{
    return (tile.x == shaft_tile.x || tile.x + offset.x == shaft_tile.x)
        && (tile.z == shaft_tile.z || tile.z + offset.z == shaft_tile.z);
}

static M_LARA_STATUS M_GetLaraStatus(
    const ITEM *const item, const ITEM *const lara_item)
{
    const int32_t lift_bottom = item->pos.y + STEP_L;
    const int32_t lift_ceiling = item->pos.y - M_HEIGHT + STEP_L;

    if (lara_item->pos.y < lift_bottom && lara_item->pos.y > lift_ceiling) {
        return M_LARA_INSIDE;
    }

    if (lara_item->pos.y <= lift_ceiling) {
        return M_LARA_ABOVE;
    }

    return M_LARA_BELOW;
}

static void M_FloorCeiling(
    const ITEM *const item, const XYZ_32 pos, int32_t *const out_floor,
    int32_t *const out_ceiling)
{
    ITEM *const lara_item = Lara_GetItem();
    const XZ_32 lift_tile = M_GetTile(item->pos);
    const XZ_32 lara_tile = M_GetTile(lara_item->pos);
    const XZ_32 test_tile = M_GetTile(pos);
    const XZ_32 offset = M_GetShaftOffset(item->rot.y);

    const bool point_in_shaft = M_IsTileInShaft(test_tile, lift_tile, offset);
    const bool lara_in_shaft = M_IsTileInShaft(lara_tile, lift_tile, offset);

    const int32_t lift_bottom = item->pos.y + STEP_L;
    const int32_t lift_floor = item->pos.y;
    const int32_t lift_ceiling = item->pos.y - M_HEIGHT + STEP_L;
    const int32_t lift_top = item->pos.y - M_HEIGHT;

    const bool lara_inside_lift =
        M_GetLaraStatus(item, lara_item) == M_LARA_INSIDE;

    *out_floor = -UNDEFINED_HEIGHT;
    *out_ceiling = UNDEFINED_HEIGHT;

    if (lara_in_shaft) {
        if (item->current_anim_state == M_STATE_DOOR_CLOSED
            && lara_inside_lift) {
            if (point_in_shaft) {
                *out_floor = lift_floor;
                *out_ceiling = lift_ceiling;
            } else {
                *out_floor = NO_HEIGHT;
                *out_ceiling = -UNDEFINED_HEIGHT;
            }
        } else if (point_in_shaft) {
            if (lara_item->pos.y <= lift_ceiling) {
                *out_floor = lift_top;
            } else if (lara_item->pos.y <= lift_bottom) {
                *out_floor = lift_floor;
                *out_ceiling = lift_ceiling;
            } else {
                *out_ceiling = lift_bottom;
            }
        }
    } else if (point_in_shaft) {
        if (pos.y <= lift_top) {
            *out_floor = lift_top;
        } else if (pos.y >= lift_bottom) {
            *out_ceiling = lift_bottom;
        } else if (item->current_anim_state == M_STATE_DOOR_OPEN) {
            *out_floor = lift_floor;
            *out_ceiling = lift_ceiling;
        } else {
            *out_floor = NO_HEIGHT;
            *out_ceiling = -UNDEFINED_HEIGHT;
        }
    }
}

static int32_t M_GetFloorHeight(
    const ITEM *const item, const XYZ_32 pos, const int32_t height)
{
    int32_t new_floor;
    int32_t new_ceiling;
    M_FloorCeiling(item, pos, &new_floor, &new_ceiling);
    if (new_floor >= height) {
        return height;
    }
    return new_floor;
}

static int32_t M_GetCeilingHeight(
    const ITEM *const item, const XYZ_32 pos, const int32_t height)
{
    int32_t new_floor;
    int32_t new_ceiling;
    M_FloorCeiling(item, pos, &new_floor, &new_ceiling);
    if (new_ceiling <= height) {
        return height;
    }
    return new_ceiling;
}

static void M_GetSectorPositions(
    const ITEM *const item, VECTOR *const sector_pos)
{
    const XZ_32 lift_tile = {
        .x = item->pos.x >> WALL_SHIFT,
        .z = item->pos.z >> WALL_SHIFT,
    };

    // Orient.
    const XZ_32 offset = M_GetShaftOffset(item->rot.y);

    // Collect a 2×2 footprint that lines up with the shaft tiles.
    for (int32_t ix = 0; ix < 2; ix++) {
        for (int32_t iz = 0; iz < 2; iz++) {
            const int32_t sx = lift_tile.x - offset.x * ix;
            const int32_t sz = lift_tile.z - offset.z * iz;

            const XYZ_32 pos = {
                .x = sx * WALL_L + WALL_L / 2,
                .y = item->pos.y,
                .z = sz * WALL_L + WALL_L / 2,
            };
            Vector_Add(sector_pos, &pos);
        }
    }

    // Collect a 2×2 footprint that lines up with the shaft ceiling tiles.
    for (int32_t ix = 0; ix < 2; ix++) {
        for (int32_t iz = 0; iz < 2; iz++) {
            const int32_t sx = lift_tile.x - offset.x * ix;
            const int32_t sz = lift_tile.z - offset.z * iz;

            const XYZ_32 pos = {
                .x = sx * WALL_L + WALL_L / 2,
                .y = item->pos.y - M_HEIGHT,
                .z = sz * WALL_L + WALL_L / 2,
            };
            Vector_Add(sector_pos, &pos);
        }
    }
}

static void M_SetupInternalWall(ITEM *const item)
{
    M_PRIV *const p = item->priv;
    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
    p->wall_bounds = *bounds;
    p->wall_bounds.max.x = bounds->min.x + M_WALL_WIDTH;
    p->wall_bounds.min.y += STEP_L / 2;
    p->wall_bounds.max.y -= STEP_L / 2;
}

// Each of these is counted in whole units, and none of them can be nothing: a
// lift that waits, travels or moves for zero never arrives.
static const char *M_CheckWhole(const TRX_VALUE *const in)
{
    return in->as_int < 1 ? "value is below one" : nullptr;
}

static const char *M_CheckSpeed(const TRX_VALUE *const in)
{
    if (in->as_int > M_MAXIMUM_SPEED) {
        return "speed is above what the lift can travel at";
    }
    return M_CheckWhole(in);
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->start_height = item->pos.y;
    p->wait_timer = 0;
    p->is_moving = false;

    VECTOR *const positions = Vector_Create(sizeof(XYZ_32));
    M_GetSectorPositions(item, positions);
    for (int32_t i = 0; i < positions->count; i++) {
        const GAME_VECTOR linked = {
            .pos = *(const XYZ_32 *)Vector_Get(positions, i),
            .room_num = item->room_num,
        };
        p->linked[i] = linked;
    }
    Walkable_AllocateNodes(item, positions->count);
    Vector_Free(positions);

    M_SetupInternalWall(item);
}

static void M_KillLara(ITEM *const lara)
{
    if (lara->hit_points <= 0) {
        return;
    }

    Lara_Kill();
    lara->speed = 0;
    lara->fall_speed = 0;
    lara->gravity = false;
    lara->rot.x = 0;
    lara->rot.z = 0;
    lara->current_anim_state = LS(LS_LIFT_DEATH);
    lara->goal_anim_state = LS(LS_LIFT_DEATH);
    Item_SwitchToAnim(lara, LA(LA_BOULDER_DEATH), 0);

    for (int32_t i = 0; i < 15; i++) {
        const int32_t x = lara->pos.x + (Random_GetControl() - 0x4000) / 256;
        const int32_t z = lara->pos.z + (Random_GetControl() - 0x4000) / 256;
        const int32_t y = lara->pos.y - Random_GetControl() / 64;
        const int32_t d = lara->rot.y + (Random_GetControl() - 0x4000) / 8;
        Spawn_Blood(x, y, z, M_DEFAULT_SPEED * 2, d, lara->room_num);
    }
}

static void M_InternalCollision(const ITEM *const item, COLL_INFO *const coll)
{
    const M_PRIV *const p = item->priv;
    COLL_ITEM wall = {
        .bounds = p->wall_bounds,
        .pos = item->pos,
        .rot = item->rot,
    };
    for (int32_t i = 0; i < M_NUM_FLOOR_SECTORS; i++) {
        wall.rot.y += DEG_90;
        wall.pos = XYZ_32_OffsetYaw(wall.pos, wall.rot.y, WALL_L);
        if (i > 0 || p->is_moving) {
            Lara_Col_Push(&wall, coll, false, true);
        }
    }
}

static bool M_ShouldPreventClimbing(
    const ITEM *const item, const ITEM *const lara_item,
    const M_LARA_STATUS lara_status)
{
    if (!Lara_TestBoundsCollide(item, 0)) {
        return false;
    }

    if (Lara_HasState(m_ClimbingStates)) {
        return true;
    }

    if (lara_status == M_LARA_BELOW) {
        return false;
    }

    // Allow vaulting only if the lift is descending to avoid embedding. This
    // avoids doing item->pos.y += coll->side_mid.floor in the regular routines
    // as that would impact vaulting off collapsible tiles and suchlike.
    if (Item_IsTriggerActiveRO(item)) {
        return false;
    }

    return Item_TestAnimEqual(lara_item, LA(LA_CLIMB_2CLICK))
        || Item_TestAnimEqual(lara_item, LA(LA_CLIMB_3CLICK))
        || Item_TestAnimEqual(lara_item, LA(LA_STAND_TO_JUMP_UP));
}

static void M_PreventClimbing(ITEM *const lara_item)
{
    lara_item->goal_anim_state = LS(LS_FAST_FALL);
    lara_item->current_anim_state = LS(LS_FAST_FALL);
    Item_SwitchToAnim(lara_item, LA(LA_SMASH_JUMP), 0);
    Lara_GetLaraInfo()->gun_status = LGS_ARMLESS;
}

static bool M_ShouldKillLara(
    const ITEM *const item, const ITEM *const lara_item)
{
    int16_t room_num = lara_item->room_num;
    const SECTOR *const sector = Room_GetSector(lara_item->pos, &room_num);
    const int32_t height = Room_GetHeight(sector, lara_item->pos);
    const int32_t ceiling = Room_GetCeiling(sector, lara_item->pos);
    if (height == NO_HEIGHT || ceiling == NO_HEIGHT) {
        return false;
    }

    const int32_t clearance = ABS(height - ceiling);
    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(lara_item);
    const int32_t lara_height = ABS(bounds->max.y - bounds->min.y);
    return clearance < lara_height;
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    if (!g_Config.gameplay.fix_lift_collision) {
        return;
    }

    const ITEM *const item = Item_Get(item_num);
    const XZ_32 lift_tile = M_GetTile(item->pos);
    const XZ_32 lara_tile = M_GetTile(lara_item->pos);
    const XZ_32 offset = M_GetShaftOffset(item->rot.y);
    if (!M_IsTileInShaft(lara_tile, lift_tile, offset)) {
        return;
    }

    const M_LARA_STATUS lara_status = M_GetLaraStatus(item, lara_item);
    if (lara_status == M_LARA_INSIDE) {
        M_InternalCollision(item, coll);
        return;
    }

    const M_PRIV *const p = item->priv;
    if (!p->is_moving) {
        return;
    }

    if (M_ShouldPreventClimbing(item, lara_item, lara_status)) {
        M_PreventClimbing(lara_item);
        return;
    }

    if (!M_ShouldKillLara(item, lara_item)) {
        return;
    }

    if (g_Config.debug.enable_invulnerability) {
        lara_item->pos.y = lara_status == M_LARA_BELOW
            ? item->pos.y
            : item->pos.y - M_HEIGHT + STEP_L;
        Lara_UpdateRoomToHeight(0);
    } else {
        M_KillLara(lara_item);
    }
}

static void M_ShiftStackableItems(
    const ITEM *const lift_item, const bool reposition)
{
    M_PRIV *const p = lift_item->priv;
    for (int32_t i = 0; i < M_NUM_FLOOR_SECTORS; i++) {
        MovableBlock_ShiftStackY(
            p->linked[i].pos.y, p->linked[i].pos, lift_item->pos.y,
            lift_item->room_num, reposition);
        if (reposition) {
            p->linked[i].pos.y = lift_item->pos.y;
        }
    }
    for (int32_t i = M_NUM_FLOOR_SECTORS; i < M_NUM_SECTORS; i++) {
        MovableBlock_ShiftStackY(
            p->linked[i].pos.y, p->linked[i].pos, lift_item->pos.y - M_HEIGHT,
            lift_item->room_num, reposition);
        if (reposition) {
            p->linked[i].pos.y = lift_item->pos.y - M_HEIGHT;
        }
    }
}

static bool M_IsItemInStack(
    const ITEM *const lift_item, const ITEM *const target_item)
{
    int16_t room_num = target_item->room_num;
    const SECTOR *sector = Room_GetSector(target_item->pos, &room_num);
    sector = Room_GetPitSector(sector, target_item->pos.x, target_item->pos.z);

    for (WALKABLE *w = sector->walkable; w != nullptr; w = w->next) {
        if (lift_item == Item_Get(w->item_num)) {
            return true;
        }
    }
    return false;
}

static void M_ShiftTravellingItems(
    const ITEM *const lift_item, const int32_t delta)
{
    // TODO: rather than passing delta to listeners, refactor Room_GetHeight to
    // be more aware of what's calling it, so that such checks in M_FloorCeiling
    // that expect Lara only can become more generic.
    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        ITEM *const item = Item_Get(i);
        if (item == lift_item) {
            continue;
        }

        const OBJECT *const obj = Object_Get(item->object_id);
        if (obj->event_func == nullptr) {
            continue;
        }

        if (!M_IsItemInStack(lift_item, item)) {
            continue;
        }

        obj->event_func(
            item, OBJECT_EVENT_FLOOR_MOVED, (void *)(intptr_t)delta);
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    const int32_t bottom = p->start_height;
    const int32_t top = bottom + p->travel_distance * STEP_L;
    const int32_t target = Item_IsTriggerActive(item) ? top : bottom;

    if (item->pos.y == target) {
        item->goal_anim_state = M_STATE_DOOR_OPEN;
        p->wait_timer = 0;
        if (p->is_moving) {
            M_ShiftStackableItems(item, true);
        }
        p->is_moving = false;
    } else if (p->wait_timer < p->wait_time * LOGIC_FPS) {
        item->goal_anim_state = M_STATE_DOOR_OPEN;
        p->wait_timer++;
        // Prevent Lara from interacting with blocks about to move.
        M_ShiftStackableItems(item, false);
    } else {
        item->goal_anim_state = M_STATE_DOOR_CLOSED;
        p->is_moving = true;
        const int32_t delta = target - item->pos.y;
        const int32_t step = (delta > 0)
            ? (delta < p->speed ? delta : p->speed)
            : (delta > -p->speed ? delta : -p->speed);
        item->pos.y += step;
        // Raise/lower possible movable blocks on top and check positions on
        // save vs load.
        M_ShiftStackableItems(item, false);
        M_ShiftTravellingItems(item, step);
    }

    Item_Animate(item);

    // Update room number one click up to avoid lift on a room portal.
    int16_t room_num = item->room_num;
    Room_GetSector(
        (XYZ_32) { item->pos.x, item->pos.y - STEP_L, item->pos.z }, &room_num);
    Item_UpdateRoom(item_num, room_num);
}

static void M_AddWalkable(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    VECTOR *positions = Vector_Create(sizeof(XYZ_32));
    M_GetSectorPositions(item, positions);
    for (int32_t i = 0; i < positions->count; i++) {
        Walkable_Add(item_num, *(const XYZ_32 *)Vector_Get(positions, i));
    }
    Vector_Free(positions);
}

static bool M_Draw(const ITEM *const item)
{
    Object_DrawAnimatingItem(item);
    if (!g_Config.debug.enable_debug_bounding_boxes
        || !g_Config.gameplay.fix_lift_collision) {
        return true;
    }

    const M_PRIV *const p = item->priv;

    Matrix_Push();
    Matrix_TranslateAbs32(item->interp.result.pos);
    Matrix_Rot16(item->interp.result.rot);

    for (int32_t i = 0; i < M_NUM_FLOOR_SECTORS; i++) {
        Matrix_TranslateRel(WALL_L, 0, 0);
        Matrix_RotY(DEG_90);
        if (i > 0 || p->is_moving) {
            Output_DrawCuboidEx(&p->wall_bounds, M_WALL_COLOR);
        }
    }

    Matrix_Pop();
    return true;
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->collision_func = M_Collision;
    obj->control_func = M_Control;
    obj->floor_height_func = M_GetFloorHeight;
    obj->ceiling_height_func = M_GetCeilingHeight;
    obj->add_walkable_func = M_AddWalkable;
    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->draw_func = M_Draw;
    obj->save_position = true;
    obj->save_flags = true;
    obj->save_anim = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_CHECKED(
            M_PRIV, wait_time, M_DEFAULT_WAIT_TIME, M_CheckWhole,
            "The time to wait before the lift begins moving, in seconds. Value "
            "range: minimum 1."),
        OBJECT_PROPERTY_CHECKED(
            M_PRIV, travel_distance, M_DEFAULT_TRAVEL_DIST, M_CheckWhole,
            "The vertical distance the lift will travel, in clicks. Value "
            "range: minimum 1."),
        OBJECT_PROPERTY_CHECKED(
            M_PRIV, speed, M_DEFAULT_SPEED, M_CheckSpeed,
            "The speed at which the lift moves, in world units. Value range: "
            "minimum 1; maximum 64."));
}

REGISTER_OBJECT(O_LIFT, M_Setup)
