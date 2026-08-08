#include <trx/config.h>
#include <trx/game/input.h>
#include <trx/game/inventory.h>
#include <trx/game/lara.h>
#include <trx/game/objects/common.h>

static XYZ_32 m_DefaultPosition = {};
static XYZ_32 m_ControlledPosition = { .x = WALL_L / 2,
                                       .y = 0,
                                       .z = -STEP_L * 3 };

static const OBJECT_BOUNDS m_DefaultBounds = {
    .shift = {
        .min = { .x = -WALL_L / 2, .y = -100, .z = -WALL_L / 2 - 300, },
        .max = { .x = +WALL_L, .y = +100, .z = -WALL_L / 2 + 100, },
    },
    .rot = {
        .min = { .x = -30 * DEG_1, .y = 0, .z = 0, },
        .max = { .x = +30 * DEG_1, .y = 0, .z = 0, },
    },
    .ignore_rot = true,
};

static const OBJECT_BOUNDS m_ControlledBounds = {
    .shift = {
        .min = { .x = -WALL_L / 2, .y = -100, .z = -WALL_L, },
        .max = { .x = +WALL_L, .y = +100, .z = -WALL_L / 2 + 100, },
    },
    .rot = {
        .min = { .x = -30 * DEG_1, .y = 0, .z = 0, },
        .max = { .x = +30 * DEG_1, .y = 0, .z = 0, },
    },
    .ignore_rot = true,
};

static void M_ConsumeKeyItem(ITEM *const receptacle_item)
{
    const OBJECT_ID key_object_id =
        Object_FindReceptacleKey(receptacle_item->object_id);
    if (key_object_id != NO_OBJECT) {
        Inv_RemoveItem(key_object_id);
    }
}

static void M_CreateGongBonger(ITEM *const lara_item)
{
    const int16_t item_gong_bonger_num = Item_Create();
    if (item_gong_bonger_num == NO_ITEM) {
        return;
    }

    ITEM *const item_gong_bonger = Item_Get(item_gong_bonger_num);
    item_gong_bonger->object_id = O_GONG_BONGER;
    item_gong_bonger->pos.x = lara_item->pos.x;
    item_gong_bonger->pos.y = lara_item->pos.y;
    item_gong_bonger->pos.z = lara_item->pos.z;
    item_gong_bonger->rot.x = 0;
    item_gong_bonger->rot.y = lara_item->rot.y;
    lara_item->rot.z = 0;
    item_gong_bonger->room_num = lara_item->room_num;

    Item_Initialise(item_gong_bonger_num);
    Item_AddSimulated(item_gong_bonger_num);
    item_gong_bonger->shade.value_1 = -1;
}

static void M_Use(ITEM *const lara_item, ITEM *const receptacle_item)
{
    Lara_AlignPosition(receptacle_item, &m_DefaultPosition);
    lara_item->rot.y += DEG_180;

    Lara_SwitchToExtraState(LS_EXTRA_GONG_BONG);
    if (Item_TestFrameEqual(lara_item, 0)) {
        M_ConsumeKeyItem(receptacle_item);
    }

    M_CreateGongBonger(lara_item);
    // A struck gong is spent. Both the collision path and M_IsUsable, which
    // gates the key selected from the inventory, read that off this axis.
    Item_SetFinished(receptacle_item, true);

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->interact_target.is_moving = false;
    lara->interact_target.item_num = NO_ITEM;
}

static const OBJECT_BOUNDS *M_Bounds(void)
{
    return g_Config.gameplay.enable_walk_to_items ? &m_ControlledBounds
                                                  : &m_DefaultBounds;
}

static bool M_ShowInventory(const ITEM *const item)
{
    if (!GF_ShowInventoryKeys(item->object_id)) {
        Lara_RefuseInteraction();
        return false;
    }

    return true;
}

static void M_CollisionControlled(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    if (!Lara_Interact_CanControl(LARA_INTERACT_RECEPTACLE, item_num)) {
        Object_Collision(item_num, lara_item, coll);
        return;
    }

    ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const OBJECT *const obj = Object_Get(item->object_id);

    const XYZ_16 old_rot = item->rot;
    item->rot.x = 0;
    item->rot.y += DEG_180;
    item->rot.z = 0;

    if (Lara_TestPosition(item, obj->bounds_func())) {
        item->rot = old_rot;
        if (!lara->interact_target.is_moving
            && (!M_ShowInventory(item)
                || !Lara_Interact_HasActiveTarget(Item_GetIndex(item)))) {
            return;
        }

        if (lara_item->current_anim_state == LS(LS_STOP)) {
            lara->interact_target.is_moving = false;
        }

        item->rot.y = old_rot.y + DEG_180;
        if (Lara_MovePosition(item, &m_ControlledPosition)) {
            Lara_Interact_FinishControl(LARA_INTERACT_RECEPTACLE);
            item->rot = old_rot;
            M_Use(lara_item, item);
        } else {
            lara->interact_target.item_num = item_num;
        }
    } else if (
        lara->interact_target.is_moving
        && lara->interact_target.item_num == item_num) {
        lara->interact_target.is_moving = false;
        lara->gun_status = LGS_ARMLESS;
    }

    item->rot = old_rot;
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->extra_anim) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    if (!Item_IsInactive(item)) {
        goto normal_collision;
    }

    if (g_Config.gameplay.enable_walk_to_items) {
        M_CollisionControlled(item_num, lara_item, coll);
        return;
    }

    if (Lara_Interact_HasActiveTarget(item_num)) {
        M_Use(lara_item, item);
        return;
    }

    if (!g_Input.action || lara->gun_status != LGS_ARMLESS || lara_item->gravity
        || !Lara_Interact_CanBegin(LARA_INTERACT_RECEPTACLE)) {
        goto normal_collision;
    }

    const XYZ_16 old_rot = item->rot;
    item->rot.x = 0;
    item->rot.y = lara_item->rot.y;
    item->rot.z = 0;

    const OBJECT *const obj = Object_Get(item->object_id);
    if (!Lara_TestPosition(item, obj->bounds_func())) {
        item->rot = old_rot;
        goto normal_collision;
    }

    item->rot = old_rot;

    M_ShowInventory(item);
    return;

normal_collision:
    Object_Collision(item_num, lara_item, coll);
}

static bool M_IsUsable(const int16_t item_num)
{
    return Item_IsInactive(Item_Get(item_num));
}

static void M_Setup(OBJECT *const obj)
{
    obj->collision_func = M_Collision;
    obj->bounds_func = M_Bounds;
    obj->is_usable_func = M_IsUsable;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_GONG, M_Setup)
