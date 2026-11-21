#include <trx/config.h>
#include <trx/game/const.h>
#include <trx/game/cutseq.h>
#include <trx/game/game_flow.h>
#include <trx/game/input.h>
#include <trx/game/inventory.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/objects/links.h>
#include <trx/game/objects/property.h>
#include <trx/game/sound.h>
#include <trx/version.h>

#define M_LF_USE_PUZZLE 80
#define M_LF_PUT_TRIDENT 180
// Marks a TR4 slot with an OCB above this value as one that has no
// collision, lets Lara use it from further away and starts the cutscene
// numbered by the OCB minus this value.
#define M_CUTSCENE_OCB 1024
// Marks a used TR4 slot with this OCB as one that Lara can walk through.
#define M_NO_COLLISION_OCB 999

// TR4 slot types, selected by the OCB. A negative OCB names the Lara
// animation that the slot plays in place of the usual one.
typedef enum {
    M_TYPE_PLAIN,
    M_TYPE_ANIM,
    M_TYPE_CUTSCENE,
} M_TYPE;

typedef struct {
    bool collidable_when_done;
} M_PRIV;

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

static int32_t M_GetOCB(const ITEM *const item)
{
    TRX_VALUE ocb = {};
    if (!ObjectProperty_GetItemValue(item, "ocb", &ocb)
        || ocb.type != TVT_S32) {
        return 0;
    }
    return ocb.as_int;
}

static M_TYPE M_GetType(const ITEM *const item)
{
    const int32_t ocb = M_GetOCB(item);
    if (ocb < 0) {
        return M_TYPE_ANIM;
    } else if (ocb > M_CUTSCENE_OCB) {
        return M_TYPE_CUTSCENE;
    }
    return M_TYPE_PLAIN;
}

static bool M_IsControlled(const ITEM *const item)
{
    return g_Config.gameplay.enable_walk_to_items
        || M_GetType(item) != M_TYPE_PLAIN;
}

static OBJECT_BOUNDS M_GetControlledBounds(const ITEM *const item)
{
    OBJECT_BOUNDS bounds = m_ControlledBounds;
    if (g_TRVersion == 4) {
        const BOUNDS_16 *const frame = Item_GetBoundsAccurate(item);
        bounds.shift.min.x = frame->min.x - STEP_L;
        bounds.shift.max.x = frame->max.x + STEP_L;
        bounds.shift.min.y = -STEP_L;
        bounds.shift.max.y = +STEP_L;
        bounds.shift.min.z = frame->min.z - STEP_L;
        bounds.shift.max.z = frame->max.z + STEP_L;
    }
    return bounds;
}

static OBJECT_BOUNDS M_GetUseBounds(const ITEM *const item)
{
    OBJECT_BOUNDS bounds = M_GetControlledBounds(item);
    if (M_GetType(item) == M_TYPE_CUTSCENE) {
        bounds.shift.min.x -= WALL_L;
        bounds.shift.max.x += WALL_L;
        bounds.shift.min.z -= WALL_L;
        bounds.shift.max.z += WALL_L;
        bounds.ignore_rot = true;
    }
    return bounds;
}

static OBJECT_BOUNDS M_Bounds(const ITEM *const item)
{
    return g_Config.gameplay.enable_walk_to_items || g_TRVersion == 4
        ? M_GetControlledBounds(item)
        : m_DefaultBounds;
}

static OBJECT_BOUNDS M_BoundsEmpty(const ITEM *const item)
{
    return M_IsControlled(item) ? M_GetUseBounds(item) : M_Bounds(item);
}

static XYZ_32 M_GetControlledPosition(const ITEM *const item)
{
    XYZ_32 position = m_ControlledPosition;
    if (g_TRVersion == 4) {
        position.z = Item_GetBoundsAccurate(item)->min.z - 100;
    }
    return position;
}

static bool M_IsSolidWhenDone(const ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    return p->collidable_when_done
        && (g_TRVersion != 4 || M_GetOCB(item) != M_NO_COLLISION_OCB);
}

static bool M_CanShowInventory(const ITEM *const item)
{
    return g_TRVersion != 4
        || Inv_HasItem(Object_FindReceptacleKey(item->object_id));
}

static bool M_IsUsable(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    return Item_IsInactive(item);
}

static void M_Use(ITEM *const lara_item, ITEM *const receptacle_item)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const XYZ_32 position = g_TRVersion == 4
        ? M_GetControlledPosition(receptacle_item)
        : m_DefaultPosition;
    Lara_AlignPosition(receptacle_item, &position);
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
    const OBJECT_ID done_obj_id =
        ObjectLink_Get(receptacle_item->object_id, OBJ_LINK_RECEPTACLE_TO_DONE);
    if (done_obj_id != NO_OBJECT) {
        receptacle_item->object_id = done_obj_id;
    }
    if (Item_IsInPlay(receptacle_item)) {
        return;
    }

    Item_SwitchToObjAnim(receptacle_item, 0, 0, receptacle_item->object_id);
    const ANIM *const anim = Item_GetAnim(receptacle_item);
    receptacle_item->current_anim_state = anim->current_anim_state;
    receptacle_item->goal_anim_state = receptacle_item->current_anim_state;
    receptacle_item->required_anim_state = 0;
    receptacle_item->trigger =
        (ITEM_TRIGGER_STATE) { .mask = TRIGGER_MASK_ALL };
    // A save taken after the done hole triggered records it finished; re-arming
    // it on load has to clear that so Item_IsInPlay reads it as active again.
    Item_SetFinished(receptacle_item, false);
    Item_AddSimulated(Item_GetIndex(receptacle_item));
    Item_Animate(receptacle_item);
}

static void M_Finish(ITEM *const receptacle_item)
{
    M_ConsumeKeyItem(receptacle_item);
    M_MarkDone(receptacle_item);
}

static void M_StartUse(ITEM *const lara_item, ITEM *const receptacle_item)
{
    const int32_t ocb = M_GetOCB(receptacle_item);
    switch (M_GetType(receptacle_item)) {
    case M_TYPE_ANIM: {
        const int16_t anim = -ocb;
        Item_SwitchToAnim(lara_item, anim, 0);
        lara_item->current_anim_state = LS(LS_CONTROLLED);
        if (anim != LA(LA_PUT_TRIDENT)) {
            M_Finish(receptacle_item);
        }
        break;
    }

    case M_TYPE_CUTSCENE:
        M_Finish(receptacle_item);
        CutSeq_Request(ocb - M_CUTSCENE_OCB, true);
        if (!CutSeq_IsActive()) {
            Lara_GetLaraInfo()->gun_status = LGS_ARMLESS;
        }
        break;

    case M_TYPE_PLAIN:
        Item_SwitchToAnim(lara_item, LA(LA_USE_PUZZLE), 0);
        lara_item->current_anim_state = LS(LS_USE_PUZZLE);
        break;
    }
}

static void M_HandleSave(ITEM *const item, const SAVEGAME_STAGE stage)
{
    if (stage == SAVEGAME_STAGE_AFTER_LOAD) {
        if (item->is_finished || Item_IsInPlay(item)) {
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
    const M_TYPE type = M_GetType(item);

    if (Lara_Interact_CanControl(LARA_INTERACT_RECEPTACLE, item_num)) {
        const OBJECT_BOUNDS bounds = M_GetUseBounds(item);
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

            const XYZ_32 position = M_GetControlledPosition(item);
            if (type == M_TYPE_CUTSCENE || Lara_MovePosition(item, &position)) {
                Lara_Interact_FinishControl(LARA_INTERACT_RECEPTACLE);
                M_StartUse(lara_item, item);
            }
        } else {
            Lara_Interact_Release(item_num);
        }
    } else if (
        lara->interact_target.item_num == item_num
        && lara_item->current_anim_state == LS(LS_USE_PUZZLE)
        && Item_TestFrameEqual(lara_item, M_LF_USE_PUZZLE)) {
        M_Finish(item);
    } else if (
        lara->interact_target.item_num == item_num
        && lara_item->current_anim_state == LS(LS_CONTROLLED)
        && Item_TestAnimEqual(lara_item, LA(LA_PUT_TRIDENT))
        && Item_TestFrameEqual(lara_item, M_LF_PUT_TRIDENT)) {
        M_Finish(item);
    } else if (
        lara_item->current_anim_state != LS(LS_CONTROLLED)
        && type != M_TYPE_CUTSCENE) {
        Object_Collision(item_num, lara_item, coll);
    }
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    if (M_IsControlled(item)) {
        M_CollisionControlled(item_num, lara_item, coll);
        return;
    }

    const OBJECT_BOUNDS bounds = Object_Get(item->object_id)->bounds_func(item);
    const LARA_INFO *const lara = Lara_GetLaraInfo();

    if (g_TRVersion == 4) {
        Object_Collision(item_num, lara_item, coll);
    }

    if (!Lara_Interact_CanBegin(LARA_INTERACT_RECEPTACLE)) {
        if (lara_item->current_anim_state == LS(LS_USE_PUZZLE)
            && Lara_TestPosition(item, &bounds)
            && Item_TestFrameEqual(lara_item, M_LF_USE_PUZZLE)) {
            M_Finish(item);
        }
        return;
    }

    if (Lara_Interact_HasActiveTarget(item_num)) {
        M_Use(lara_item, item);
    }

    if (!g_Input.action || lara->gun_status != LGS_ARMLESS
        || lara_item->gravity) {
        return;
    }

    if (!Lara_TestPosition(item, &bounds)) {
        return;
    }

    if (M_CanShowInventory(item) && !GF_ShowInventoryKeys(item->object_id)) {
        Lara_RefuseInteraction();
    }
}

static void M_CollisionDone(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    const OBJECT_BOUNDS bounds = Object_Get(item->object_id)->bounds_func(item);
    const LARA_INFO *const lara = Lara_GetLaraInfo();

    if ((g_Config.gameplay.enable_walk_to_items || g_TRVersion == 4)
        && M_IsSolidWhenDone(item)) {
        Object_Collision(item_num, lara_item, coll);
    }

    if (g_TRVersion == 4 || !g_Input.action || lara->gun_status != LGS_ARMLESS
        || lara_item->gravity
        || !Lara_Interact_CanBegin(LARA_INTERACT_RECEPTACLE)
        || !Lara_TestPosition(item, &bounds)) {
        return;
    }

    // Trying to interact with a complete puzzle hole
    Lara_RefuseInteraction();
}

static void M_DeclareProperties(OBJECT *const obj)
{
    obj->priv_size = sizeof(M_PRIV);
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY(
            M_PRIV, collidable_when_done, true,
            "Whether or not Lara can collide with the slot after using it."));
}

static void M_SetupEmpty(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = M_Collision;
    obj->handle_save_func = M_HandleSave;
    obj->is_usable_func = M_IsUsable;
    obj->bounds_func = M_BoundsEmpty;
    obj->save_flags = true;
    obj->save_anim = true;
    M_DeclareProperties(obj);
}

static void M_SetupDone(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = M_CollisionDone;
    obj->bounds_func = M_Bounds;
    obj->save_flags = true;
    obj->save_anim = true;
    M_DeclareProperties(obj);
}

#define X_PICKUP_PUZZLE(n)                                                     \
    REGISTER_OBJECT(O_PUZZLE_HOLE_##n, M_SetupEmpty)                           \
    REGISTER_OBJECT(O_PUZZLE_DONE_##n, M_SetupDone)
#include <trx/game/objects/pickups.def>
#undef X_PICKUP_PUZZLE
