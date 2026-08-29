#include <trx/game/objects/general/generic_switch.h>

#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/version.h>

typedef enum {
    M_STATE_OFF,
    M_STATE_ON,
} M_STATE;

typedef struct {
    SWITCH_MODE switch_mode;
} M_PRIV;

static const OBJECT_BOUNDS m_Bounds = {
    .shift = {},
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
    },
};

static const char *M_CheckSwitchMode(const TRX_VALUE *const in)
{
    return in->as_int < 0 || in->as_int >= SWITCH_MODE_NUMBER_OF
        ? "no such switch mode"
        : nullptr;
}

static void M_TurnSwitchOn(ITEM *const switch_item, ITEM *const lara_item)
{
    const M_PRIV *const p = switch_item->priv;
    LARA_ANIMATION_ID anim;
    switch (p->switch_mode) {
    case SWITCH_MODE_NORMAL:
        anim = LA_WALL_SWITCH_UP;
        break;
    case SWITCH_MODE_SHOVE:
        anim = LA_BIG_BUTTON_PUSH;
        break;
    case SWITCH_MODE_HIDDEN_REACH:
        anim = LA_HOLE_GRAB;
        break;
    default:
        return;
    }

    Item_SwitchToAnim(lara_item, LA(anim), 0);
    lara_item->current_anim_state = Item_GetAnim(lara_item)->current_anim_state;
    switch_item->goal_anim_state = M_STATE_ON;

    if (p->switch_mode == SWITCH_MODE_SHOVE) {
        switch_item->trigger.spent = true;
    }
}

static void M_TurnSwitchOff(ITEM *const switch_item, ITEM *const lara_item)
{
    const M_PRIV *const p = switch_item->priv;
    LARA_ANIMATION_ID anim;
    switch (p->switch_mode) {
    case SWITCH_MODE_NORMAL:
        anim = LA_WALL_SWITCH_DOWN;
        break;
    case SWITCH_MODE_HIDDEN_REACH:
        anim = LA_HOLE_GRAB;
        break;
    default:
        return;
    }

    Item_SwitchToAnim(lara_item, LA(anim), 0);
    lara_item->current_anim_state = Item_GetAnim(lara_item)->current_anim_state;
    switch_item->goal_anim_state = M_STATE_OFF;
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    if (item->trigger.spent) {
        Object_Collision(item_num, lara_item, coll);
        return;
    }

    const M_PRIV *const p = item->priv;
    if (p->switch_mode == SWITCH_MODE_HIDDEN_PICKUP) {
        Object_Collision(item_num, lara_item, coll);
        return;
    }

    if (Lara_Interact_CanControl(LARA_INTERACT_SWITCH, item_num)) {
        const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);

        OBJECT_BOUNDS col_bounds = m_Bounds;
        col_bounds.shift.min.x = bounds->min.x - STEP_L;
        col_bounds.shift.max.x = bounds->max.x + STEP_L;

        XYZ_32 position = {};

        if (p->switch_mode != SWITCH_MODE_NORMAL) {
            col_bounds.shift.min.z = bounds->min.z - STEP_L * 2;
            col_bounds.shift.max.z = bounds->max.z + STEP_L * 2;

            if (p->switch_mode == SWITCH_MODE_SHOVE) {
                position.z = bounds->min.z - STEP_L;
            } else {
                position.z = bounds->min.z - STEP_L / 2;
            }
        } else {
            col_bounds.shift.min.z = bounds->min.z - 200;
            col_bounds.shift.max.z = bounds->max.z + 200;
            position.z = bounds->min.z - STEP_L / 4;
        }

        LARA_INFO *const lara = Lara_GetLaraInfo();
        if (Lara_TestPosition(item, &col_bounds)) {
            if (Lara_MovePosition(item, &position)) {
                if (item->current_anim_state == M_STATE_OFF) {
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

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->trigger.mask = TRIGGER_MASK_ALL;
    if (!Item_IsTriggerActive(item)) {
        item->goal_anim_state = M_STATE_OFF;
        item->timer = 0;
    }
    Item_Animate(item);

    if (g_TRVersion >= 3 && item->trigger.switch_spent) {
        item->trigger.switch_spent = false;
        item->trigger.spent = true;
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->collision_func = M_Collision;
    obj->control_func = M_Control;
    obj->priv_size = sizeof(M_PRIV);
    obj->save_flags = true;
    obj->save_anim = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_CHECKED(
            M_PRIV, switch_mode, SWITCH_MODE_NORMAL, M_CheckSwitchMode,
            "Switch animation mode - 0: normal; 1: hidden reach; 2: hidden "
            "pickup; 3: single-use shove"));
}

REGISTER_OBJECT(O_SWITCH_TYPE_GENERIC_1, M_Setup)
REGISTER_OBJECT(O_SWITCH_TYPE_GENERIC_2, M_Setup)
REGISTER_OBJECT(O_SWITCH_TYPE_GENERIC_3, M_Setup)
REGISTER_OBJECT(O_SWITCH_TYPE_GENERIC_4, M_Setup)
REGISTER_OBJECT(O_SWITCH_TYPE_GENERIC_5, M_Setup)
REGISTER_OBJECT(O_SWITCH_TYPE_GENERIC_6, M_Setup)
