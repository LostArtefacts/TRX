#include <trx/config.h>
#include <trx/game/game_flow.h>
#include <trx/game/input.h>
#include <trx/game/inventory.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/sound.h>
#include <trx/version.h>

#define M_LF_USE_KEYHOLE 104

static XYZ_32 m_DefaultPosition = {
    .x = 0,
    .y = 0,
    .z = WALL_L / 2 - LARA_RADIUS - 50,
};

static XYZ_32 m_ControlledPosition = {
    .x = 0,
    .y = 0,
    .z = WALL_L / 2 - LARA_RADIUS * 2,
};

static XYZ_32 m_ControlledPositionTR4 = {
    .x = 0,
    .y = 0,
    .z = WALL_L / 2 - LARA_RADIUS - 50,
};

static const OBJECT_BOUNDS m_DefaultBounds = {
    .shift = {
        .min = { .x = -200, .y = +0, .z = +WALL_L / 2 - 200, },
        .max = { .x = +200, .y = +0, .z = +WALL_L / 2, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
    },
};

static const OBJECT_BOUNDS m_ControlledBounds = {
    .shift = {
        .min = { .x = -STEP_L, .y = +0, .z = +0, },
        .max = { .x = +STEP_L, .y = +0, .z = +412, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
    },
};

static OBJECT_BOUNDS M_Bounds(const ITEM *const item)
{
    return g_Config.gameplay.enable_walk_to_items || g_TRVersion == 4
        ? m_ControlledBounds
        : m_DefaultBounds;
}

static bool M_CanShowInventory(const ITEM *const item)
{
    return g_TRVersion != 4
        || Inv_HasItem(Object_FindReceptacleKey(item->object_id));
}

static bool M_IsSolid(const ITEM *const item)
{
    if (g_TRVersion != 4) {
        return true;
    }
    // Keyholes 7 to 12 have no collision in TR4, as in the original game.
    switch (item->object_id) {
    case O_KEY_HOLE_7:
    case O_KEY_HOLE_8:
    case O_KEY_HOLE_9:
    case O_KEY_HOLE_10:
    case O_KEY_HOLE_11:
    case O_KEY_HOLE_12:
        return false;
    default:
        return true;
    }
}

static void M_Use(ITEM *const lara_item, ITEM *const receptacle_item)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    Lara_AlignPosition(receptacle_item, &m_DefaultPosition);
    Lara_AnimateUntil(lara_item, LS(LS_USE_KEY));
    lara_item->goal_anim_state = LS(LS_STOP);
    lara->gun_status = LGS_HANDS_BUSY;
    lara->interact_target.is_moving = false;
}

static void M_ConsumeKeyItem(ITEM *const receptacle_item)
{
    const OBJECT_ID key_object_id =
        Object_FindReceptacleKey(receptacle_item->object_id);
    if (key_object_id != NO_OBJECT) {
        Inv_RemoveItem(key_object_id);
    }
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->interact_target.item_num = NO_ITEM;
}

static void M_MarkDone(ITEM *const receptacle_item)
{
    // The inserted key is the receptacle entering the simulation, the same
    // done-state the puzzle hole uses. Item_IsInPlay then reads as "armed".
    // A new save carries this in the active field; a released save recorded it
    // as IS_ACTIVE off the active list, re-armed on load in M_ReadItem.
    Item_AddSimulated(Item_GetIndex(receptacle_item));
}

static void M_Control(const int16_t item_num)
{
    // A keyhole has nothing to animate; it holds a control function only so a
    // filled hole can join the simulation, since Item_AddSimulated skips a
    // control-less item. Being simulated is what lets Keyhole_Trigger, shared
    // with the puzzle hole, gate on Item_IsInPlay.
}

static void M_CollisionControlled(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();

    if (Lara_Interact_CanControl(LARA_INTERACT_RECEPTACLE, item_num)) {
        const OBJECT *const obj = Object_Get(item->object_id);
        const OBJECT_BOUNDS bounds = obj->bounds_func(item);
        if (Lara_TestPosition(item, &bounds)) {
            if (g_Input.action && !lara->interact_target.is_moving
                && M_CanShowInventory(item)
                && !GF_ShowInventoryKeys(item->object_id)) {
                Lara_RefuseInteraction();
                return;
            }

            if (lara->interact_target.item_num != item_num) {
                lara->interact_target.is_moving = false;
                return;
            }

            const XYZ_32 *const position = g_TRVersion == 4
                ? &m_ControlledPositionTR4
                : &m_ControlledPosition;
            if (Lara_MovePosition(item, position)) {
                Item_SwitchToAnim(lara_item, LA(LA_USE_KEY), 0);
                lara_item->current_anim_state = LS(LS_USE_KEY);
                Lara_Interact_FinishControl(LARA_INTERACT_RECEPTACLE);
            }
        } else {
            Lara_Interact_Release(item_num);
        }
    } else if (
        lara->interact_target.item_num == item_num
        && lara_item->current_anim_state == LS(LS_USE_KEY)
        && Item_TestFrameEqual(lara_item, M_LF_USE_KEYHOLE)) {
        M_ConsumeKeyItem(item);
        M_MarkDone(item);
    } else if (M_IsSolid(item)) {
        Object_Collision(item_num, lara_item, coll);
    }
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    if (g_Config.gameplay.enable_walk_to_items) {
        M_CollisionControlled(item_num, lara_item, coll);
        return;
    }

    ITEM *const item = Item_Get(item_num);
    const OBJECT_BOUNDS bounds = Object_Get(item->object_id)->bounds_func(item);
    const LARA_INFO *const lara = Lara_GetLaraInfo();

    if (g_TRVersion == 4 && M_IsSolid(item)) {
        Object_Collision(item_num, lara_item, coll);
    }

    if (!Lara_Interact_CanBegin(LARA_INTERACT_RECEPTACLE)) {
        if (lara_item->current_anim_state == LS(LS_USE_KEY)
            && Lara_TestPosition(item, &bounds)
            && Item_TestFrameEqual(lara_item, M_LF_USE_KEYHOLE)) {
            M_ConsumeKeyItem(item);
            M_MarkDone(item);
        }
        return;
    }

    if (Lara_Interact_HasActiveTarget(item_num)) {
        M_Use(lara_item, item);
    }

    if (!g_Input.action || lara->gun_status != LGS_ARMLESS || lara_item->gravity
        || lara_item->current_anim_state != LS(LS_STOP)) {
        return;
    }

    if (!Lara_TestPosition(item, &bounds)) {
        return;
    }

    if (!Item_IsInactive(item)) {
        if (g_TRVersion != 4) {
            Lara_RefuseInteraction();
        }
    } else if (
        M_CanShowInventory(item) && !GF_ShowInventoryKeys(item->object_id)) {
        Lara_RefuseInteraction();
    }
}

static bool M_IsUsable(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    return Item_IsInactive(item);
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = M_Collision;
    obj->bounds_func = M_Bounds;
    obj->save_flags = true;
    obj->is_usable_func = M_IsUsable;
}

bool Keyhole_Trigger(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (!Item_IsInPlay(item) || lara->gun_status == LGS_HANDS_BUSY) {
        return false;
    }
    Item_SetFinished(item, true);
    return true;
}

#define X_PICKUP_KEY(n) REGISTER_OBJECT(O_KEY_HOLE_##n, M_Setup)
#include <trx/game/objects/pickups.def>
#undef X_PICKUP_KEY
