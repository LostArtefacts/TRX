#include <trx/game/lara.h>
#include <trx/game/objects/traps/movable_block.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>

// clang-format off
#define M_NUM_SECTORS   4
#define M_CEILING_STEP  STEP_L
#define M_HEIGHT_STEP   (STEP_L / 8) // = 32
#define M_HALF_CLICK    (STEP_L / 2) // = 128
#define M_MAXIMUM_SPEED (STEP_L / 4) // = 64
#define M_DEFAULT_SPEED 4
#define M_DEFAULT_DIST  16
// clang-format on

typedef enum {
    M_DIR_UP,
    M_DIR_DOWN,
} M_DIRECTION;

typedef struct {
    bool is_pressure_plate;
    int32_t travel_distance;
    int32_t speed;
    int32_t origin;
    M_DIRECTION direction;
    XYZ_32 positions[M_NUM_SECTORS];
} M_PRIV;

static const char *M_CheckWhole(const TRX_VALUE *const in)
{
    return in->as_int < 1 ? "value is below one" : nullptr;
}

static const char *M_CheckSpeed(const TRX_VALUE *const in)
{
    if (in->as_int > M_MAXIMUM_SPEED) {
        return "speed is above what the floor can travel at";
    }
    return M_CheckWhole(in);
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->origin = item->pos.y;
    Walkable_AllocateNodes(item, M_NUM_SECTORS);

    XYZ_32 pos = item->pos;
    int16_t angle = item->rot.y;
    for (int32_t i = 0; i < M_NUM_SECTORS; i++) {
        p->positions[i] = pos;
        pos = XYZ_32_OffsetYaw(pos, angle, WALL_L);
        angle -= DEG_90;
    }
}

static void M_AddWalkable(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    for (int32_t i = 0; i < M_NUM_SECTORS; i++) {
        Walkable_Add(item_num, p->positions[i]);
    }
}

static bool M_IsWithinFootprint(const ITEM *const item, const XYZ_32 pos)
{
    const M_PRIV *const p = item->priv;
    const XZ_32 test_pos = {
        .x = pos.x >> WALL_SHIFT,
        .z = pos.z >> WALL_SHIFT,
    };

    for (int32_t i = 0; i < M_NUM_SECTORS; i++) {
        const XZ_32 sector_pos = {
            .x = p->positions[i].x >> WALL_SHIFT,
            .z = p->positions[i].z >> WALL_SHIFT,
        };
        if (sector_pos.x == test_pos.x && sector_pos.z == test_pos.z) {
            return true;
        }
    }

    return false;
}

static int32_t M_GetFloorHeight(
    const ITEM *const item, const XYZ_32 pos, const int32_t height)
{
    if (!M_IsWithinFootprint(item, pos)) {
        return height;
    }

    if (pos.y > item->pos.y + M_HEIGHT_STEP || item->pos.y >= height) {
        return height;
    }

    return item->pos.y;
}

static int32_t M_GetCeilingHeight(
    const ITEM *const item, const XYZ_32 pos, const int32_t height)
{
    if (!M_IsWithinFootprint(item, pos)) {
        return height;
    }

    if (pos.y <= item->pos.y + M_HEIGHT_STEP || item->pos.y <= height) {
        return height;
    }

    return item->pos.y + M_CEILING_STEP;
}

static bool M_IsLaraOnItem(const ITEM *const item, const ITEM *const lara_item)
{
    if (!M_IsWithinFootprint(item, lara_item->pos)) {
        return false;
    }
    const int32_t height =
        M_GetFloorHeight(item, lara_item->pos, lara_item->pos.y + 1);
    return height == item->pos.y;
}

static void M_ShiftStackableItems(const ITEM *const item, const int32_t old_y)
{
    M_PRIV *const p = item->priv;
    for (int32_t i = 0; i < M_NUM_SECTORS; i++) {
        MovableBlock_ShiftStackY(
            old_y, p->positions[i], item->pos.y, item->room_num, true);
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;

    if (!Item_IsTriggerActive(item)) {
        return;
    }

    const int32_t old_y = item->pos.y;

    if (p->is_pressure_plate) {
        const ITEM *const lara_item = Lara_GetItem();
        if (Item_TestAnimEqual(lara_item, LA(LA_SMALL_JUMP_BACK))
            || !M_IsLaraOnItem(item, lara_item)) {
            p->direction = M_DIR_UP;
        } else {
            p->direction = M_DIR_DOWN;
        }

        if (p->direction == M_DIR_DOWN) {
            if (item->pos.y
                >= p->origin + (p->travel_distance * M_HALF_CLICK)) {
                p->direction = M_DIR_UP;
            } else {
                Sound_Effect(SFX_LOWERING_BLOCK, &item->pos, SPM_NORMAL);
                item->pos.y += p->speed;
            }
        } else {
            if (item->pos.y <= p->origin) {
                p->direction = M_DIR_DOWN;
            } else {
                Sound_Effect(SFX_LOWERING_BLOCK, &item->pos, SPM_NORMAL);
                item->pos.y -= p->speed;
            }
        }
    } else {
        if (item->pos.y > p->origin - (p->travel_distance * M_HALF_CLICK)) {
            item->pos.y -= p->speed;
        }

        int16_t room_num = item->room_num;
        const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
        item->floor = Room_GetHeight(sector, item->pos);
        if (room_num != item->room_num) {
            Item_UpdateRoom(item_num, room_num);
        }
    }

    if (old_y != item->pos.y) {
        M_ShiftStackableItems(item, old_y);
    }
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;

    obj->add_walkable_func = M_AddWalkable;
    obj->floor_height_func = M_GetFloorHeight;
    obj->ceiling_height_func = M_GetCeilingHeight;

    obj->priv_size = sizeof(M_PRIV);
    obj->save_flags = true;
    obj->save_position = true;

    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY(
            M_PRIV, is_pressure_plate, false,
            "Whether the floor acts as a pressure plate, moving down when Lara "
            "is on top and back up when she leaves it."),
        OBJECT_PROPERTY_CHECKED(
            M_PRIV, travel_distance, M_DEFAULT_DIST, M_CheckWhole,
            "The vertical distance the floor will travel, in half-clicks. "
            "Value range: minimum 1."),
        OBJECT_PROPERTY_CHECKED(
            M_PRIV, speed, M_DEFAULT_SPEED, M_CheckSpeed,
            "The speed at which the floor moves, in world units. Value range: "
            "minimum 1; maximum 64."));
}

REGISTER_OBJECT(O_MOVING_FLOOR, M_Setup)
