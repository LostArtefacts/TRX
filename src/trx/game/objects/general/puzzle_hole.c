#include <trx/config.h>
#include <trx/game/game_flow.h>
#include <trx/game/input.h>
#include <trx/game/inventory.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/objects/vars.h>
#include <trx/game/sound.h>

#define M_LF_USE_PUZZLE 80

static XYZ_32 m_DefaultPosition = {
    .x = 0,
    .y = 0,
    .z = WALL_L / 2 - LARA_RADIUS - 85,
};

static XYZ_32 m_ControlledPosition = {
    .x = 0,
    .y = 0,
    .z = WALL_L / 2 - LARA_RADIUS * 2,
};

static const OBJECT_BOUNDS m_DefaultBounds = {
    .shift = {
        .min = { .x = -200, .y = 0, .z = WALL_L / 2 - 200, },
        .max = { .x = +200, .y = 0, .z = WALL_L / 2, },
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

static bool M_IsUsable(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    return item->status == IS_INACTIVE;
}

static void M_Use(ITEM *const lara_item, ITEM *const receptacle_item)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    Lara_AlignPosition(receptacle_item, &m_DefaultPosition);
    Lara_AnimateUntil(lara_item, LS(LS_USE_PUZZLE));
    lara_item->goal_anim_state = LS(LS_STOP);
    lara->gun_status = LGS_HANDS_BUSY;
    lara->interact_target.is_moving = false;
}

static void M_ConsumeKeyItem(ITEM *const receptacle_item)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const OBJECT_ID key_object_id =
        Object_FindReceptacleKey(receptacle_item->object_id);
    if (key_object_id != NO_OBJECT) {
        Inv_RemoveItem(key_object_id);
    }
    lara->interact_target.item_num = NO_ITEM;
}

static void M_MarkDone(ITEM *const receptacle_item)
{
    const OBJECT_ID done_obj_id = Object_GetCognate(
        receptacle_item->object_id, g_ReceptacleToReceptacleDoneMap);
    if (done_obj_id != NO_OBJECT) {
        receptacle_item->object_id = done_obj_id;
    }
    if (receptacle_item->status == IS_ACTIVE) {
        return;
    }

    Item_SwitchToObjAnim(receptacle_item, 0, 0, receptacle_item->object_id);
    const ANIM *const anim = Item_GetAnim(receptacle_item);
    receptacle_item->current_anim_state = anim->current_anim_state;
    receptacle_item->goal_anim_state = receptacle_item->current_anim_state;
    receptacle_item->required_anim_state = 0;
    receptacle_item->flags = IF_CODE_BITS;
    receptacle_item->status = IS_ACTIVE;
    Item_AddActive(Item_GetIndex(receptacle_item));
    Item_Animate(receptacle_item);
}

static void M_HandleSave(ITEM *const item, const SAVEGAME_STAGE stage)
{
    if (stage == SAVEGAME_STAGE_AFTER_LOAD) {
        if (item->status == IS_DEACTIVATED || item->status == IS_ACTIVE) {
            M_MarkDone(item);
        }
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (Item_IsTriggerActive(item)) {
        Item_Animate(item);
    }
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
                Item_SwitchToAnim(lara_item, LA(LA_USE_PUZZLE), 0);
                lara_item->current_anim_state = LS(LS_USE_PUZZLE);
                lara->head_rot.y = 0;
                lara->head_rot.x = 0;
                lara->torso_rot.y = 0;
                lara->torso_rot.x = 0;
                lara->interact_target.is_moving = false;
                lara->gun_status = LGS_HANDS_BUSY;
            }
        } else if (
            lara->interact_target.is_moving
            && lara->interact_target.item_num == item_num) {
            lara->interact_target.is_moving = false;
            lara->gun_status = LGS_ARMLESS;
        }
    } else if (
        lara->interact_target.item_num == item_num
        && lara_item->current_anim_state == LS(LS_USE_PUZZLE)
        && Item_TestFrameEqual(lara_item, M_LF_USE_PUZZLE)) {
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
        if (lara_item->current_anim_state == LS(LS_USE_PUZZLE)
            && Lara_TestPosition(item, obj->bounds_func())
            && Item_TestFrameEqual(lara_item, M_LF_USE_PUZZLE)) {
            M_ConsumeKeyItem(item);
            M_MarkDone(item);
        }
        return;
    }

    if (lara->interact_target.is_moving
        && lara->interact_target.item_num == item_num) {
        M_Use(lara_item, item);
    }

    if (!g_Input.action || lara->gun_status != LGS_ARMLESS
        || lara_item->gravity) {
        return;
    }

    if (!Lara_TestPosition(item, obj->bounds_func())) {
        return;
    }

    if (!GF_ShowInventoryKeys(item->object_id)) {
        Lara_RefuseInteraction();
    }
}

static void M_CollisionDone(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    const OBJECT *const obj = Object_Get(item->object_id);
    const LARA_INFO *const lara = Lara_GetLaraInfo();

    if (g_Config.gameplay.enable_walk_to_items) {
        Object_Collision(item_num, lara_item, coll);
    }

    if (!g_Input.action || lara->gun_status != LGS_ARMLESS || lara_item->gravity
        || !Lara_Interact_CanBegin(LARA_INTERACT_RECEPTACLE)
        || !Lara_TestPosition(item, obj->bounds_func())) {
        return;
    }

    // Trying to interact with a complete puzzle hole
    Lara_RefuseInteraction();
}

static void M_SetupEmpty(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = M_Collision;
    obj->handle_save_func = M_HandleSave;
    obj->is_usable_func = M_IsUsable;
    obj->bounds_func = M_Bounds;
    obj->save_flags = true;
    obj->save_anim = true;
}

static void M_SetupDone(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = M_CollisionDone;
    obj->bounds_func = M_Bounds;
    obj->save_flags = true;
    obj->save_anim = true;
}

REGISTER_OBJECT(O_PUZZLE_HOLE_1, M_SetupEmpty)
REGISTER_OBJECT(O_PUZZLE_HOLE_2, M_SetupEmpty)
REGISTER_OBJECT(O_PUZZLE_HOLE_3, M_SetupEmpty)
REGISTER_OBJECT(O_PUZZLE_HOLE_4, M_SetupEmpty)
REGISTER_OBJECT(O_PUZZLE_DONE_1, M_SetupDone)
REGISTER_OBJECT(O_PUZZLE_DONE_2, M_SetupDone)
REGISTER_OBJECT(O_PUZZLE_DONE_3, M_SetupDone)
REGISTER_OBJECT(O_PUZZLE_DONE_4, M_SetupDone)
