#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/camera.h>
#include <trx/game/lara.h>
#include <trx/game/objects/traps/movable_block.h>
#include <trx/game/pathing.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>

// clang-format off
#define M_FIXED_SCALE (1 << W2V_SHIFT) // = 16384
#define M_MAX_SCALE   (WALL_L * 4) // = 4096
#define M_OFFSET      (STEP_L * 2) // = 512
#define M_SPEED       (STEP_L / 4) // = 64
#define M_BOUNCE_DIST (WALL_L * 10) // = 10240
#define M_MIN_SHAKE   (-32)
// clang-format on

typedef struct {
    XYZ_32 base_pos;
    int16_t scale;
    int16_t prev_scale;
    bool expanded;
    bool shake_camera;
} M_PRIV;

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "scale", &p->scale));
    MUST(JSON_READ_OPT(io, "expanded", &p->expanded));
    p->prev_scale = p->scale;
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "scale", p->scale);
    JSONW_WRITE(io, "expanded", p->expanded);
}

static void M_UpdateBox(const ITEM *const item, const bool blocked)
{
    int16_t room_num = item->room_num;
    const M_PRIV *const p = item->priv;
    const SECTOR *const sector = Room_GetSector(p->base_pos, &room_num);
    BOX_INFO *const box = Box_GetBox(sector->box);
    if (box != nullptr && (box->overlap_index & BOX_BLOCKABLE) != 0) {
        TOGGLE_BIT(box->overlap_index, BOX_BLOCKED, blocked);
    }
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->base_pos = item->pos;
    M_UpdateBox(item, false);
    Walkable_AllocateNodes(item, 1);

    if (item->object_id == O_EXPANDING_BLOCK) {
        p->base_pos.y += M_OFFSET;
        if (item->rot.y == 0) {
            item->pos.z += M_OFFSET;
        } else if (item->rot.y == DEG_90) {
            item->pos.x += M_OFFSET;
        } else if (item->rot.y == -DEG_180) {
            item->pos.z -= M_OFFSET;
        } else if (item->rot.y == -DEG_90) {
            item->pos.x -= M_OFFSET;
        }
    }
}

static void M_AddWalkable(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    Walkable_Add(item_num, p->base_pos);
}

static int32_t M_GetItemTop(const ITEM *const item)
{
    const int32_t item_height =
        WALL_L * (item->object_id == O_RAISING_BLOCK_2 ? 2 : 1);
    const M_PRIV *const p = item->priv;
    return p->base_pos.y - item_height;
}

static bool M_IsAgainstFloor(const ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(p->base_pos, &room_num);
    return !sector->floor.is_split && sector->floor.tilt.x == 0
        && sector->floor.tilt.z == 0 && sector->floor.height == p->base_pos.y;
}

static bool M_IsAgainstCeiling(const ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(p->base_pos, &room_num);
    const SECTOR *const sky_sector =
        Room_GetSkySector(sector, p->base_pos.x, p->base_pos.z);
    return !sector->ceiling.is_split && sky_sector->ceiling.tilt.x == 0
        && sky_sector->ceiling.tilt.z == 0
        && sky_sector->ceiling.height == M_GetItemTop(item);
}

static int32_t M_GetFloorHeight(
    const ITEM *const item, const XYZ_32 pos, const int32_t height)
{
    const M_PRIV *const p = item->priv;
    if (!p->expanded) {
        return height;
    }

    const bool on_floor = M_IsAgainstFloor(item);
    if (on_floor && M_IsAgainstCeiling(item)) {
        return NO_HEIGHT;
    }

    const int32_t item_top = M_GetItemTop(item);

    if (pos.y <= p->base_pos.y && pos.y > item_top) {
        const SECTOR *const sector = Room_GetWorldSector(
            Room_Get(item->room_num), p->base_pos.x, p->base_pos.z);
        if (p->base_pos.y < sector->floor.height) {
            return height;
        } else if (on_floor) {
            return item_top;
        }
    }

    if (pos.y > p->base_pos.y) {
        return height;
    }

    if (item_top >= height) {
        return height;
    }

    return item_top;
}

static int32_t M_GetCeilingHeight(
    const ITEM *const item, const XYZ_32 pos, const int32_t height)
{
    const M_PRIV *const p = item->priv;
    if (!p->expanded) {
        return height;
    }

    const bool on_floor = M_IsAgainstFloor(item);
    if (on_floor && M_IsAgainstCeiling(item)) {
        return NO_HEIGHT;
    }

    const int32_t item_top = M_GetItemTop(item);

    if (pos.y <= p->base_pos.y && pos.y > item_top) {
        return on_floor ? item_top : p->base_pos.y;
    }

    if (pos.y <= item_top) {
        return height;
    }

    if (p->base_pos.y <= height) {
        return height;
    }

    return p->base_pos.y;
}

static void M_ApplyShake(const ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    if (!p->shake_camera) {
        return;
    }

    const ITEM *const lara_item = Lara_GetItem();
    if (!Item_IsNearby(item, lara_item, M_BOUNCE_DIST)) {
        return;
    }

    if (p->scale == M_SPEED || p->scale == M_MAX_SCALE) {
        g_Camera.bounce = M_MIN_SHAKE;
    } else {
        g_Camera.bounce = M_MIN_SHAKE / 2;
    }
}

static bool M_IsBlocked(const ITEM *const item)
{
    int16_t room_num = item->room_num;
    const M_PRIV *const p = item->priv;
    const SECTOR *const sector = Room_GetSector(p->base_pos, &room_num);
    const int32_t height = Room_GetHeight(sector, p->base_pos);
    return height <= M_GetItemTop(item);
}

static void M_DropStack(const ITEM *const item)
{
    M_PRIV *const p = item->priv;
    const XYZ_32 pos = {
        .x = p->base_pos.x,
        .y = M_GetItemTop(item),
        .z = p->base_pos.z,
    };
    MovableBlock_DropStack(pos, item->room_num);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->prev_scale = p->scale;

    if (Item_IsTriggerActive(item)) {
        if (p->scale == 0 && M_IsBlocked(item)) {
            return;
        }

        if (!p->expanded) {
            M_UpdateBox(item, true);
            p->expanded = true;
        }

        if (p->scale < M_MAX_SCALE) {
            Sound_Effect(SFX_LOWERING_BLOCK, &p->base_pos, SPM_NORMAL);
            p->scale += M_SPEED;
            M_ApplyShake(item);
        }
    } else if (p->scale > 0) {
        Sound_Effect(SFX_LOWERING_BLOCK, &p->base_pos, SPM_NORMAL);
        M_ApplyShake(item);
        p->scale -= M_SPEED;
    } else if (p->expanded) {
        M_UpdateBox(item, false);
        p->expanded = false;
        M_DropStack(item);
    }
}

static inline XYZ_32 M_GetDrawScale(const OBJECT_ID obj_id, const int32_t value)
{
    const bool is_vertical = obj_id != O_EXPANDING_BLOCK;
    return (XYZ_32) {
        .x = M_FIXED_SCALE,
        .y = is_vertical ? (value << 2) : M_FIXED_SCALE,
        .z = is_vertical ? M_FIXED_SCALE : (value << 2),
    };
}

static bool M_Draw(const ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    const XYZ_32 scale = M_GetDrawScale(item->object_id, p->scale);
    const XYZ_32 prev_scale = M_GetDrawScale(item->object_id, p->prev_scale);
    return Object_DrawScaledItem(item, scale, prev_scale);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->draw_func = M_Draw;

    obj->add_walkable_func = M_AddWalkable;
    obj->floor_height_func = M_GetFloorHeight;
    obj->ceiling_height_func = M_GetCeilingHeight;

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_flags = true;

    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY(
            M_PRIV, shake_camera, false,
            "Whether or not the camera should shake when the block is "
            "expanding "
            "or contracting."));
}

REGISTER_OBJECT(O_RAISING_BLOCK_1, M_Setup)
REGISTER_OBJECT(O_RAISING_BLOCK_2, M_Setup)
REGISTER_OBJECT(O_EXPANDING_BLOCK, M_Setup)
