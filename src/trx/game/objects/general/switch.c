#include <trx/game/objects/general/switch.h>

#include <trx/config.h>
#include <trx/game/const.h>
#include <trx/game/input.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/version.h>

typedef struct {
    XYZ_32 normal;
    XYZ_32 controlled;
} M_SWITCH_POS;

static const OBJECT_BOUNDS m_SwitchBounds = {
    .shift = {
        .min = { .x = -220, .y = +0, .z = +WALL_L / 2 - 220, },
        .max = { .x = +220, .y = +0, .z = +WALL_L / 2, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
    },
};

static const OBJECT_BOUNDS m_SwitchBoundsControlled = {
    .shift = {
        .min = { .x = -WALL_L / 2, .y = +0, .z = -200, },
        .max = { .x = +WALL_L / 2, .y = +0, .z = +200, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
    },
};

static const OBJECT_BOUNDS m_SwitchBoundsUW = {
    .shift = {
        .min = { .x = -WALL_L, .y = -WALL_L, .z = -WALL_L, },
        .max = { .x = +WALL_L, .y = +WALL_L, .z = +WALL_L / 2, },
    },
    .rot = {
        .min = { .x = -80 * DEG_1, .y = -80 * DEG_1, .z = -80 * DEG_1, },
        .max = { .x = +80 * DEG_1, .y = +80 * DEG_1, .z = +80 * DEG_1, },
    },
};

static const OBJECT_BOUNDS m_SwitchBoundsUWCeiling = {
    .shift = {
        .min = { .x = -STEP_L, .y = -STEP_L * 5, .z = -STEP_L * 2, },
        .max = { .x = +STEP_L, .y = -STEP_L * 2, .z = +0, },
    },
    .rot = {
        .min = { .x = -80 * DEG_1, .y = -80 * DEG_1, .z = -80 * DEG_1, },
        .max = { .x = +80 * DEG_1, .y = +80 * DEG_1, .z = +80 * DEG_1, },
    },
};

static const OBJECT_BOUNDS m_SwitchBoundsJump = {
    .shift = {
        .min = { .x = -STEP_L / 2, .y = -STEP_L, .z = +STEP_L * 3 / 2, },
        .max = { .x = +STEP_L / 2, .y = +STEP_L, .z = +STEP_L * 2, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
    },
};

static const XYZ_32 m_SwitchUWPosition = {
    .x = 0,
    .y = 0,
    .z = 108,
};
static const XYZ_32 m_SwitchUWPositionCeiling = {
    .x = 0,
    .y = -736,
    .z = -416,
};

static const M_SWITCH_POS m_SmallSwitchPosition = {
    .normal = { .x = 0, .y = 0, .z = 362 },
    .controlled = { .x = 0, .y = 0, .z = 80 },
};

static const M_SWITCH_POS m_PushSwitchPosition = {
    .normal = { .x = 0, .y = 0, .z = 292 },
    .controlled = { .x = 0, .y = 0, .z = 146 },
};

static const M_SWITCH_POS m_WallSwitchPosition = {
    .normal = { .x = 0, .y = 0, .z = 128 },
    .controlled = { .x = 0, .y = 0, .z = 64 },
};

static const M_SWITCH_POS m_AirlockPosition = {
    .normal = { .x = 0, .y = 0, .z = 212 },
    .controlled = { .x = 0, .y = 0, .z = 106 },
};

static const M_SWITCH_POS m_JumpSwitchPosition = {
    .normal = { .x = 0, .y = -208, .z = 256 },
    .controlled = {},
};

static const OBJECT_BOUNDS *M_Bounds(void)
{
    return g_Config.gameplay.enable_walk_to_items ? &m_SwitchBoundsControlled
                                                  : &m_SwitchBounds;
}

static const OBJECT_BOUNDS *M_BoundsUW(void)
{
    return &m_SwitchBoundsUW;
}

static const OBJECT_BOUNDS *M_BoundsUWCeiling(void)
{
    return &m_SwitchBoundsUWCeiling;
}

static const OBJECT_BOUNDS *M_BoundsJump(void)
{
    return &m_SwitchBoundsJump;
}

static int16_t M_TranslateState(
    const ITEM *const item, const SWITCH_STATE state)
{
    if (item->object_id != O_SWITCH_TYPE_JUMP || state == SWITCH_STATE_LINK) {
        return state;
    }
    return state == SWITCH_STATE_OFF ? SWITCH_STATE_ON : SWITCH_STATE_OFF;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->trigger.mask = TRIGGER_MASK_ALL;
    if (!Item_IsTriggerActive(item)) {
        item->goal_anim_state = M_TranslateState(item, SWITCH_STATE_OFF);
        item->timer = 0;
    }
    Item_Animate(item);

    if (g_TRVersion >= 3 && item->trigger.switch_spent) {
        item->trigger.switch_spent = false;
        item->trigger.spent = true;
    }
}

static void M_AlignLara(ITEM *const lara_item, ITEM *const switch_item)
{
    lara_item->rot.y = switch_item->rot.y;
    switch (switch_item->object_id) {
    case O_SWITCH_TYPE_AIRLOCK:
    case O_SWITCH_TYPE_WHEEL:
        Lara_AlignPosition(switch_item, &m_AirlockPosition.normal);
        break;

    case O_SWITCH_TYPE_SMALL:
        Lara_AlignPosition(switch_item, &m_SmallSwitchPosition.normal);
        break;

    case O_SWITCH_TYPE_BUTTON:
        Lara_AlignPosition(switch_item, &m_PushSwitchPosition.normal);
        break;

    default:
        break;
    }
}

static bool M_MoveLaraControlled(
    const ITEM *const item, const BOUNDS_16 *const bounds)
{
    XYZ_32 shift;
    switch (item->object_id) {
    case O_SWITCH_TYPE_AIRLOCK:
    case O_SWITCH_TYPE_WHEEL:
        shift = m_AirlockPosition.controlled;
        break;
    case O_SWITCH_TYPE_SMALL:
        shift = m_SmallSwitchPosition.controlled;
        break;
    case O_SWITCH_TYPE_BUTTON:
        shift = m_PushSwitchPosition.controlled;
        break;
    default:
        shift = m_WallSwitchPosition.controlled;
        break;
    }

    const XYZ_32 move_vector = {
        .x = 0,
        .y = 0,
        .z = bounds->min.z - shift.z,
    };
    return Lara_MovePosition(item, &move_vector);
}

static void M_TurnSwitchOn(ITEM *const switch_item, ITEM *const lara_item)
{
    switch (switch_item->object_id) {
    case O_SWITCH_TYPE_WHEEL:
        Lara_SwitchToExtraState(LS_EXTRA_AIRLOCK);
        break;
    case O_SWITCH_TYPE_SMALL:
        Item_SwitchToAnim(lara_item, LA(LA_SWITCH_SMALL_DOWN), 0);
        break;

    case O_SWITCH_TYPE_BUTTON:
        Item_SwitchToAnim(lara_item, LA(LA_BUTTON_PUSH), 0);
        break;

    default:
        Item_SwitchToAnim(lara_item, LA(LA_WALL_SWITCH_DOWN), 0);
        break;
    }

    if (!Lara_GetLaraInfo()->extra_anim) {
        lara_item->current_anim_state = LS(LS_SWITCH_ON);
    }
    switch_item->goal_anim_state = SWITCH_STATE_ON;
}

static void M_TurnSwitchOff(ITEM *const switch_item, ITEM *const lara_item)
{
    lara_item->current_anim_state = LS(LS_SWITCH_OFF);

    LARA_INFO *const lara = Lara_GetLaraInfo();
    switch (switch_item->object_id) {
    case O_SWITCH_TYPE_AIRLOCK:
    case O_SWITCH_TYPE_WHEEL:
        Lara_SwitchToExtraState(LS_EXTRA_AIRLOCK);
        break;

    case O_SWITCH_TYPE_SMALL:
        Item_SwitchToAnim(lara_item, LA(LA_SWITCH_SMALL_UP), 0);
        break;

    case O_SWITCH_TYPE_BUTTON:
        Item_SwitchToAnim(lara_item, LA(LA_BUTTON_PUSH), 0);
        break;

    default:
        Item_SwitchToAnim(lara_item, LA(LA_WALL_SWITCH_UP), 0);
        break;
    }

    switch_item->goal_anim_state = SWITCH_STATE_OFF;
}

static void M_CollisionControlled(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();

    if (Lara_Interact_CanControl(LARA_INTERACT_SWITCH, item_num)) {
        const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);

        OBJECT_BOUNDS col_bounds = *Object_Get(item->object_id)->bounds_func();
        col_bounds.shift.min.x += bounds->min.x;
        col_bounds.shift.max.x += bounds->max.x;
        col_bounds.shift.min.z += bounds->min.z;
        col_bounds.shift.max.z += bounds->max.z;

        if (Lara_TestPosition(item, &col_bounds)) {
            if (M_MoveLaraControlled(item, bounds)) {
                if (item->current_anim_state == SWITCH_STATE_OFF) {
                    M_TurnSwitchOn(item, lara_item);
                } else {
                    M_TurnSwitchOff(item, lara_item);
                }
                Lara_Interact_FinishControl(LARA_INTERACT_SWITCH);
                Item_AddSimulated(item_num);
                Item_Animate(item);
            } else {
                lara->interact_target.item_num = item_num;
            }
        } else if (Lara_Interact_HasActiveTarget(item_num)) {
            lara->interact_target.is_moving = false;
            lara->gun_status = LGS_ARMLESS;
        }
    } else if (
        lara_item->current_anim_state != LS(LS_SWITCH_ON)
        && lara_item->current_anim_state != LS(LS_SWITCH_OFF)) {
        Object_Collision(item_num, lara_item, coll);
    }
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    if (g_TRVersion >= 3 && item->trigger.spent) {
        return;
    }

    if (g_Config.gameplay.enable_walk_to_items) {
        M_CollisionControlled(item_num, lara_item, coll);
        return;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    const OBJECT *const obj = Object_Get(item->object_id);

    if (!g_Input.action || !Item_IsInactive(item)
        || lara->gun_status != LGS_ARMLESS || lara_item->gravity
        || !Lara_Interact_CanBegin(LARA_INTERACT_SWITCH)
        || !Lara_TestPosition(item, obj->bounds_func())) {
        return;
    }

    if (item->object_id == O_SWITCH_TYPE_AIRLOCK
        && item->current_anim_state == SWITCH_STATE_OFF) {
        return;
    }

    M_AlignLara(lara_item, item);

    if (item->current_anim_state == SWITCH_STATE_OFF) {
        M_TurnSwitchOn(item, lara_item);
    } else {
        M_TurnSwitchOff(item, lara_item);
    }

    if (!lara->extra_anim) {
        lara_item->goal_anim_state = LS(LS_STOP);
    }
    lara->gun_status = LGS_HANDS_BUSY;

    Item_AddSimulated(item_num);
    Item_Animate(item);
}

static void M_CollisionUW(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const OBJECT *const obj = Object_Get(item->object_id);

    if (!g_Input.action || !Item_IsInactive(item)
        || (lara->water_status != LWS_UNDERWATER
            && lara->water_status != LWS_CHEAT)
        || lara->gun_status != LGS_ARMLESS
        || lara_item->current_anim_state != LS(LS_TREAD)) {
        return;
    }

    if (!Lara_TestPosition(item, obj->bounds_func())) {
        return;
    }

    if (item->current_anim_state != SWITCH_STATE_ON
        && item->current_anim_state != SWITCH_STATE_OFF) {
        return;
    }

    if (!Lara_MovePosition(item, &m_SwitchUWPosition)) {
        return;
    }

    lara_item->fall_speed = 0;
    Lara_AnimateUntil(lara_item, LS(LS_SWITCH_ON));
    lara_item->goal_anim_state = LS(LS_TREAD);
    lara->gun_status = LGS_HANDS_BUSY;

    if (item->current_anim_state == SWITCH_STATE_OFF) {
        item->goal_anim_state = SWITCH_STATE_ON;
    } else {
        item->goal_anim_state = SWITCH_STATE_OFF;
    }
    Item_AddSimulated(item_num);
    Item_Animate(item);
}

static void M_CollisionUWCeiling(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const OBJECT *const obj = Object_Get(item->object_id);

    if (!g_Input.action || !Item_IsInactive(item)
        || item->current_anim_state != SWITCH_STATE_ON
        || (lara->water_status != LWS_UNDERWATER
            && lara->water_status != LWS_CHEAT)
        || lara->gun_status != LGS_ARMLESS
        || lara_item->current_anim_state != LS(LS_TREAD)) {
        return;
    }

    OBJECT_BOUNDS bounds = *obj->bounds_func();
    XYZ_32 position = m_SwitchUWPositionCeiling;
    if (Lara_TestPosition(item, &bounds)) {
        if (!Lara_MovePosition(item, &position)) {
            return;
        }
    } else {
        SWAP(bounds.shift.min.z, bounds.shift.max.z);
        bounds.shift.min.z *= -1;
        bounds.shift.max.z *= -1;
        position.z *= -1;
        lara_item->rot.y -= DEG_180;
        const bool flip_result = Lara_TestPosition(item, &bounds)
            && Lara_MovePosition(item, &position);
        lara_item->rot.y -= DEG_180;
        if (!flip_result) {
            return;
        }
    }

    Item_SwitchToAnim(lara_item, LA(LA_UNDERWATER_PULLEY), 0);
    lara_item->current_anim_state = LS(LS_SWITCH_ON);
    lara_item->fall_speed = 0;
    lara->gun_status = LGS_HANDS_BUSY;

    if (item->current_anim_state == SWITCH_STATE_OFF) {
        item->goal_anim_state = SWITCH_STATE_ON;
    } else {
        item->goal_anim_state = SWITCH_STATE_OFF;
    }
    Item_AddSimulated(item_num);
    Item_Animate(item);
}

static void M_CollisionJump(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    if (item->current_anim_state != M_TranslateState(item, SWITCH_STATE_OFF)) {
        return;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (!g_Input.action || lara->gun_status != LGS_ARMLESS
        || !lara_item->gravity || lara_item->fall_speed <= 0) {
        return;
    }

    if (lara_item->current_anim_state != LS(LS_REACH)
        && lara_item->current_anim_state != LS(LS_JUMP_UP)) {
        return;
    }

    const OBJECT *const obj = Object_Get(item->object_id);
    if (!Lara_TestPosition(item, obj->bounds_func())) {
        return;
    }

    Lara_AlignPosition(item, &m_JumpSwitchPosition.normal);
    Item_SwitchToAnim(lara_item, LA(LA_JUMPSWITCH), 0);
    lara_item->current_anim_state = LS(LS_SWITCH_ON);
    lara_item->fall_speed = 0;
    lara_item->gravity = false;
    lara->gun_status = LGS_HANDS_BUSY;

    item->goal_anim_state = M_TranslateState(item, SWITCH_STATE_ON);
    Item_AddSimulated(item_num);
}

static void M_SetupBase(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->save_flags = true;
    obj->save_anim = true;
}

static void M_SetupCommon(OBJECT *const obj)
{
    M_SetupBase(obj);
    obj->collision_func = M_Collision;
    obj->bounds_func = M_Bounds;
}

static void M_SetupPushButton(OBJECT *const obj)
{
    M_SetupCommon(obj);
    obj->enable_interpolation = false;
    obj->bounds_func = M_Bounds;
}

static void M_SetupUW(OBJECT *const obj)
{
    M_SetupBase(obj);
    obj->collision_func = M_CollisionUW;
    obj->bounds_func = M_BoundsUW;
}

static void M_SetupUWCeiling(OBJECT *const obj)
{
    M_SetupBase(obj);
    obj->collision_func = M_CollisionUWCeiling;
    obj->bounds_func = M_BoundsUWCeiling;
}

static void M_SetupAirlock(OBJECT *const obj)
{
    M_SetupCommon(obj);
    obj->draw_func = Object_DrawUnclippedItem;
}

static void M_SetupJump(OBJECT *const obj)
{
    M_SetupCommon(obj);
    obj->collision_func = M_CollisionJump;
    obj->bounds_func = M_BoundsJump;
}

bool Switch_Trigger(const int16_t item_num, const int16_t timer)
{
    ITEM *const item = Item_Get(item_num);

    if (item->object_id == O_SWITCH_TYPE_AIRLOCK) {
        if (item->is_finished) {
            Item_RemoveSimulated(item_num);
            Item_SetFinished(item, false);
            return false;
        } else if (
            item->trigger.spent
            || item->current_anim_state == SWITCH_STATE_ON) {
            return false;
        }

        item->trigger.spent = true;
        return true;
    }

    if (Object_Get(item->object_id)->intelligent) {
        // Custom levels often use switch triggers under enemies for events on
        // death; the following addition is a safer approach for such, rather
        // than altering item status and timer as though they were regular
        // switch objects.
        if (!item->is_finished && !item->is_destroyed) {
            return false;
        }
        if (item->trigger.switch_spent) {
            return false;
        }
        item->trigger.switch_spent = true;
        return true;
    }

    if (!item->is_finished) {
        return false;
    }

    if (item->current_anim_state == M_TranslateState(item, SWITCH_STATE_ON)
        && timer > 0) {
        item->timer = timer;
        if (timer != 1) {
            item->timer *= LOGIC_FPS;
        }
        Item_SetFinished(item, false);
    } else {
        Item_RemoveSimulated(item_num);
        Item_SetFinished(item, false);
    }
    return true;
}

REGISTER_OBJECT(O_SWITCH_TYPE_AIRLOCK, M_SetupAirlock)
REGISTER_OBJECT(O_SWITCH_TYPE_BUTTON, M_SetupPushButton)
REGISTER_OBJECT(O_SWITCH_TYPE_NORMAL, M_SetupCommon)
REGISTER_OBJECT(O_SWITCH_TYPE_SMALL, M_SetupCommon)
REGISTER_OBJECT(O_SWITCH_TYPE_UW, M_SetupUW)
REGISTER_OBJECT(O_SWITCH_TYPE_WHEEL, M_SetupCommon)
REGISTER_OBJECT(O_SWITCH_TYPE_JUMP, M_SetupJump)
REGISTER_OBJECT(O_SWITCH_TYPE_UW_CEILING, M_SetupUWCeiling)
