// Tomb of Qualopec and Sanctuary Scion pickup.
// Triggers O_LARA_EXTRA pedestal pickup animation.
// TODO: merge into pickup.c as PICKUP_MODE_SCION.

#include <trx/config.h>
#include <trx/game/camera.h>
#include <trx/game/input.h>
#include <trx/game/lara.h>
#include <trx/game/objects/common.h>
#include <trx/game/rooms.h>

static XYZ_32 m_DefaultPosition = { 0, 640, -310 };
static XYZ_32 m_ControlledPosition = { 0, 0, -380 };

static const OBJECT_BOUNDS m_DefaultBounds = {
    .shift = {
        .min = { .x = -256, .y = +640 - 100, .z = -350, },
        .max = { .x = +256, .y = +640 + 100, .z = -200, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = 0, .z = 0, },
        .max = { .x = +10 * DEG_1, .y = 0, .z = 0, },
    },
};

static const OBJECT_BOUNDS m_ControlledBounds = {
    .shift = {
        .min = { .x = -256, .y = -640 - 100, .z = -511, },
        .max = { .x = +256, .y = +640 + 100, .z = 320, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = 0, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = 0, },
    },
};

static const OBJECT_BOUNDS *M_Bounds(void)
{
    return g_Config.gameplay.enable_walk_to_items ? &m_ControlledBounds
                                                  : &m_DefaultBounds;
}

static void M_HandleSave(ITEM *const item, const SAVEGAME_STAGE stage)
{
    if (stage == SAVEGAME_STAGE_AFTER_LOAD) {
        if (item->is_finished) {
            const int16_t item_num = Item_GetIndex(item);
            Item_DetachFromRoom(item_num);
        }
    }
}

static XYZ_16 M_PrepareAndCacheRot(
    ITEM *const item, const ITEM *const lara_item)
{
    const XYZ_16 old_rot = item->rot;
    item->rot.y = lara_item->rot.y;
    item->rot.x = 0;
    item->rot.z = 0;
    return old_rot;
}

static void M_Collect(const ITEM *const item, const ITEM *const lara_item)
{
    Lara_GetLaraInfo()->interact_target.item_num = Item_GetIndex(item);
    Lara_AlignPosition(item, &m_DefaultPosition);
    Lara_SwitchToExtraState(LS_EXTRA_SCION_PICKUP_1);
    Camera_InvokeCinematic(lara_item, 0, 0);
}

static const BOUNDS_16 *M_FindPlinthBounds(const ITEM *const item)
{
    const ROOM *const room = Room_Get(item->room_num);
    const BOUNDS_16 *const item_bounds = Item_GetBoundsAccurate(item);
    for (int32_t i = 0; i < room->num_static_meshes; i++) {
        const STATIC_MESH *const mesh = &room->static_meshes[i];
        const STATIC_OBJECT_3D *const obj =
            Object_Get3DStatic(mesh->static_num);
        if (!obj->visible || mesh->pos.x != item->pos.x
            || mesh->pos.z != item->pos.z) {
            continue;
        }

        const BOUNDS_16 *const obj_bounds = &obj->collision_bounds;
        if (item_bounds->min.x <= obj_bounds->max.x
            && item_bounds->max.x >= obj_bounds->min.x
            && item_bounds->min.z <= obj_bounds->max.z
            && item_bounds->max.z >= obj_bounds->min.z
            && (obj_bounds->min.x != 0 || obj_bounds->max.x != 0)) {
            return obj_bounds;
        }
    }

    return nullptr;
}

static void M_CollisionControlled(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    const XYZ_16 old_rot = M_PrepareAndCacheRot(item, lara_item);
    LARA_INFO *const lara = Lara_GetLaraInfo();

    if (Lara_Interact_CanControl(LARA_INTERACT_PICKUP, item_num)) {
        const OBJECT *const obj = Object_Get(item->object_id);
        OBJECT_BOUNDS bounds = *obj->bounds_func();
        bounds.shift.max.y = lara_item->pos.y - item->pos.y + 100;
        const BOUNDS_16 *const plinth_bounds = M_FindPlinthBounds(item);
        if (plinth_bounds != nullptr) {
            bounds.shift.min.x = plinth_bounds->min.x;
            bounds.shift.max.x = plinth_bounds->max.x;
            bounds.shift.max.z = plinth_bounds->max.z;
        }

        if (Lara_TestPosition(item, &bounds)) {
            XYZ_32 pos = m_ControlledPosition;
            pos.y = Lara_GetItem()->pos.y - item->pos.y;
            if (plinth_bounds != nullptr) {
                pos.z = -200 - plinth_bounds->max.z;
            }

            if (Lara_MovePosition(item, &pos)) {
                Lara_Interact_FinishControl(LARA_INTERACT_PICKUP);
                M_Collect(item, lara_item);
            }
            lara->interact_target.item_num = item_num;
        } else if (Lara_Interact_HasActiveTarget(item_num)) {
            lara->interact_target.is_moving = false;
            lara->interact_target.item_num = NO_ITEM;
            lara->gun_status = LGS_ARMLESS;
        }
    } else if (
        !lara->interact_target.is_moving
        && lara->interact_target.item_num == item_num) {
        lara->interact_target.item_num = NO_ITEM;
    }

cleanup:
    item->rot = old_rot;
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->extra_anim) {
        return;
    }

    if (g_Config.gameplay.enable_walk_to_items) {
        M_CollisionControlled(item_num, lara_item, coll);
        return;
    }

    ITEM *const item = Item_Get(item_num);
    const XYZ_16 old_rot = M_PrepareAndCacheRot(item, lara_item);

    const OBJECT *const obj = Object_Get(item->object_id);
    if (!Lara_TestPosition(item, obj->bounds_func())) {
        goto cleanup;
    }

    if (g_Input.action && lara->gun_status == LGS_ARMLESS && !lara_item->gravity
        && Lara_Interact_CanBegin(LARA_INTERACT_RECEPTACLE)) {
        M_Collect(item, lara_item);
    }

cleanup:
    item->rot = old_rot;
}

static void M_Setup(OBJECT *const obj)
{
    obj->handle_save_func = M_HandleSave;
    obj->draw_func = Object_DrawPickupItem;
    obj->collision_func = M_Collision;
    obj->save_flags = true;
    obj->bounds_func = M_Bounds;
}

REGISTER_OBJECT(O_SCION_ITEM_1, M_Setup)
