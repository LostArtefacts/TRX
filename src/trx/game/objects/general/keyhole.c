#include <trx/config.h>
#include <trx/game/game_flow.h>
#include <trx/game/input.h>
#include <trx/game/inventory.h>
#include <trx/game/lara.h>
#include <trx/game/sound.h>

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

static const OBJECT_BOUNDS *M_Bounds(void)
{
    return g_Config.gameplay.enable_walk_to_items ? &m_ControlledBounds
                                                  : &m_DefaultBounds;
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
    receptacle_item->status = IS_ACTIVE;
}

static void M_CollisionControlled(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();

    if (Lara_Interact_CanControl(LARA_INTERACT_RECEPTACLE, item_num)) {
        const OBJECT *const obj = Object_Get(item->object_id);
        if (Lara_TestPosition(item, obj->bounds_func())) {
            if (g_Input.action && !lara->interact_target.is_moving
                && !GF_ShowInventoryKeys(item->object_id)) {
                Lara_RefuseInteraction();
                return;
            }

            if (lara->interact_target.item_num != item_num) {
                lara->interact_target.is_moving = false;
                return;
            }

            if (lara->interact_target.move_count == 0) {
                lara->interact_target.is_moving = false;
            }

            if (Lara_MovePosition(item, &m_ControlledPosition)) {
                Item_SwitchToAnim(lara_item, LA(LA_USE_KEY), 0);
                lara_item->current_anim_state = LS(LS_USE_KEY);
                Lara_Interact_FinishControl(LARA_INTERACT_RECEPTACLE);
            }
        } else if (
            lara->interact_target.is_moving
            && lara->interact_target.item_num == item_num) {
            lara->interact_target.is_moving = false;
            lara->gun_status = LGS_ARMLESS;
        }
    } else if (
        lara->interact_target.item_num == item_num
        && lara_item->current_anim_state == LS(LS_USE_KEY)
        && Item_TestFrameEqual(lara_item, M_LF_USE_KEYHOLE)) {
        M_ConsumeKeyItem(item);
        M_MarkDone(item);
    } else {
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
    const OBJECT *const obj = Object_Get(item->object_id);
    const LARA_INFO *const lara = Lara_GetLaraInfo();

    if (!Lara_Interact_CanBegin(LARA_INTERACT_RECEPTACLE)) {
        if (lara_item->current_anim_state == LS(LS_USE_KEY)
            && Lara_TestPosition(item, obj->bounds_func())
            && Item_TestFrameEqual(lara_item, M_LF_USE_KEYHOLE)) {
            M_ConsumeKeyItem(item);
            M_MarkDone(item);
        }
        return;
    }

    if (lara->interact_target.is_moving
        && lara->interact_target.item_num == item_num) {
        M_Use(lara_item, item);
    }

    if (!g_Input.action || lara->gun_status != LGS_ARMLESS || lara_item->gravity
        || lara_item->current_anim_state != LS(LS_STOP)) {
        return;
    }

    if (!Lara_TestPosition(item, obj->bounds_func())) {
        return;
    }

    if (item->status != IS_INACTIVE) {
        Lara_RefuseInteraction();
    } else if (!GF_ShowInventoryKeys(item->object_id)) {
        Lara_RefuseInteraction();
    }
}

static bool M_IsUsable(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    return item->status == IS_INACTIVE;
}

static void M_Setup(OBJECT *const obj)
{
    obj->collision_func = M_Collision;
    obj->bounds_func = M_Bounds;
    obj->save_flags = true;
    obj->is_usable_func = M_IsUsable;
}

bool Keyhole_Trigger(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (item->status != IS_ACTIVE || lara->gun_status == LGS_HANDS_BUSY) {
        return false;
    }
    item->status = IS_DEACTIVATED;
    return true;
}

REGISTER_OBJECT(O_KEY_HOLE_1, M_Setup)
REGISTER_OBJECT(O_KEY_HOLE_2, M_Setup)
REGISTER_OBJECT(O_KEY_HOLE_3, M_Setup)
REGISTER_OBJECT(O_KEY_HOLE_4, M_Setup)
