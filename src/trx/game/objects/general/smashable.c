#include <trx/game/objects/general/smashable.h>

#include <trx/config.h>
#include <trx/core/math.h>
#include <trx/core/utils.h>
#include <trx/game/collision/common.h>
#include <trx/game/lara.h>
#include <trx/game/objects/common.h>
#include <trx/game/pathing.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>

static void M_SetBoxBlocked(const ITEM *const item, const bool blocked)
{
    const ROOM *const room = Room_Get(item->room_num);
    const SECTOR *const sector =
        Room_GetWorldSector(room, item->pos.x, item->pos.z);
    BOX_INFO *const box = Box_GetBox(sector->box);
    if (box == nullptr) {
        return;
    }

    if (blocked && (box->overlap_index & BOX_BLOCKABLE) != 0) {
        box->overlap_index |= BOX_BLOCKED;
    } else if (!blocked && (box->overlap_index & BOX_BLOCKED) != 0) {
        box->overlap_index &= ~BOX_BLOCKED;
    }
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->trigger = (ITEM_TRIGGER_STATE) { 0 };
    item->mesh_bits = 1;
    M_SetBoxBlocked(item, true);
}

static void M_HandleSave(ITEM *const item, const SAVEGAME_STAGE stage)
{
    if (stage == SAVEGAME_STAGE_AFTER_LOAD) {
        if (item->trigger.spent) {
            item->mesh_bits = 0x100;
            M_SetBoxBlocked(item, false);
        }
    }
}

static void M_Control1(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->trigger.spent) {
        return;
    }

    if (Lara_Vehicle_IsMounted()) {
        if (Lara_IsNearItem(&item->pos, 512)) {
            Smashable_Smash(item_num);
        }
    } else if (item->touch_bits) {
        item->touch_bits = 0;
        const ITEM *const lara_item = Lara_GetItem();
        const int32_t speed =
            ABS((lara_item->speed * Math_Cos(lara_item->rot.y - item->rot.y))
                >> W2V_SHIFT);
        if (speed >= 50) {
            Smashable_Smash(item_num);
        }
    }
}

static void M_Control2(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->trigger.spent) {
        return;
    }

    M_SetBoxBlocked(item, false);

    item->mesh_bits = ~1;
    item->is_collidable = false;
    Item_Shatter(item_num, 65278, 0);

    if (item->object_id == O_SMASHABLE_2) {
        Sound_Effect(SFX_BRITTLE_GROUND_BREAK, &item->pos, SPM_NORMAL);
    } else if (item->object_id == O_SMASHABLE_3) {
        Sound_Effect(SFX_EXPLOSION_1, &item->pos, SPM_NORMAL);
        Sound_Effect(SFX_EXPLOSION_2, &item->pos, SPM_NORMAL);
    }

    item->trigger.spent = true;
    Item_SetFinished(item, true);
    Item_RemoveSimulated(item_num);
}

// The wall is a pane standing on the edge of a sector, so the floor data
// describes the ground it covers as open. Whoever is about to move through
// that ground asks here instead.
static bool M_Block(
    const ITEM *const item, const XYZ_32 from, const XYZ_32 to,
    const int32_t height, const int32_t reach)
{
    if (!g_Config.gameplay.fix_breakable_wall_clip || item->trigger.spent
        || !item->is_collidable) {
        return false;
    }

    const ANIM_FRAME *const frame = Item_GetBestFrame(item);
    if (frame == nullptr) {
        return false;
    }

    const BOUNDS_16 *const bounds = &frame->bounds;
    const int32_t min_y = MIN(from.y, to.y) - height;
    const int32_t max_y = MAX(from.y, to.y);
    if (max_y <= item->pos.y + bounds->min.y
        || min_y >= item->pos.y + bounds->max.y) {
        return false;
    }

    const XYZ_32 from_r =
        XYZ_32_UnrotateYaw(XYZ_32_Subtract(from, item->pos), item->rot.y);
    const XYZ_32 to_r =
        XYZ_32_UnrotateYaw(XYZ_32_Subtract(to, item->pos), item->rot.y);

    const bool is_thin_in_z =
        bounds->max.z - bounds->min.z <= bounds->max.x - bounds->min.x;
    const int32_t along = is_thin_in_z ? to_r.x : to_r.z;
    const int32_t along_min = is_thin_in_z ? bounds->min.x : bounds->min.z;
    const int32_t along_max = is_thin_in_z ? bounds->max.x : bounds->max.z;
    if (along < along_min || along > along_max) {
        return false;
    }

    const int32_t from_across = is_thin_in_z ? from_r.z : from_r.x;
    const int32_t to_across = is_thin_in_z ? to_r.z : to_r.x;
    const int32_t middle = is_thin_in_z ? (bounds->min.z + bounds->max.z) / 2
                                        : (bounds->min.x + bounds->max.x) / 2;
    return (from_across < middle) != (to_across < middle)
        || ABS(to_across - middle) <= reach;
}

static void M_SetupBase(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->handle_save_func = M_HandleSave;
    obj->collision_func = Object_Collision;
    obj->save_flags = true;
    obj->save_anim = true;
}

static void M_Setup1(OBJECT *const obj)
{
    M_SetupBase(obj);
    obj->control_func = M_Control1;
    obj->block_func = M_Block;
}

static void M_Setup2(OBJECT *const obj)
{
    M_SetupBase(obj);
    obj->control_func = M_Control2;
}

void Smashable_Smash(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_SetBoxBlocked(item, false);

    item->is_collidable = false;
    item->mesh_bits = ~1;
    Item_Shatter(item_num, 0b11111110'11111110, 0);

    if (item->object_id == O_SMASHABLE_1) {
        Sound_Effect(SFX_GLASS_BREAK, &item->pos, SPM_NORMAL);
    } else if (item->object_id == O_SMASHABLE_4) {
        Sound_Effect(SFX_SHUTTERS_BREAK, &item->pos, SPM_NORMAL);
    }

    item->trigger.spent = true;
    if (Item_IsInPlay(item)) {
        Item_RemoveSimulated(item_num);
    }
    Item_SetFinished(item, true);
}

REGISTER_OBJECT(O_SMASHABLE_1, M_Setup1)
REGISTER_OBJECT(O_SMASHABLE_2, M_Setup2)
REGISTER_OBJECT(O_SMASHABLE_3, M_Setup2)
REGISTER_OBJECT(O_SMASHABLE_4, M_Setup1)
