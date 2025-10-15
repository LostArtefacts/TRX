#include "game/lara.h"

#include <libtrx/game/input.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/objects/general/switch.h>

static XYZ_32 g_SmallSwitchPosition = { .x = 0, .y = 0, .z = 362 };
static XYZ_32 g_PushSwitchPosition = { .x = 0, .y = 0, .z = 292 };
static XYZ_32 m_AirlockPosition = { .x = 0, .y = 0, .z = 212 };
static XYZ_32 m_SwitchUWPosition = { .x = 0, .y = 0, .z = 108 };

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

static const OBJECT_BOUNDS *M_Bounds(void)
{
    return &m_SwitchBounds;
}

static const OBJECT_BOUNDS *M_BoundsUW(void)
{
    return &m_SwitchBoundsUW;
}

static void M_AlignLara(ITEM *const lara_item, ITEM *const switch_item)
{
    lara_item->rot.y = switch_item->rot.y;
    switch (switch_item->object_id) {
    case O_SWITCH_TYPE_AIRLOCK:
        Lara_AlignPosition(switch_item, &m_AirlockPosition);
        break;

    case O_SWITCH_TYPE_SMALL:
        Lara_AlignPosition(switch_item, &g_SmallSwitchPosition);
        break;

    case O_SWITCH_TYPE_BUTTON:
        Lara_AlignPosition(switch_item, &g_PushSwitchPosition);
        break;

    default:
        break;
    }
}

static void M_SwitchOn(ITEM *const switch_item, ITEM *const lara_item)
{
    switch (switch_item->object_id) {
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

    lara_item->current_anim_state = LS(LS_SWITCH_ON);
    switch_item->goal_anim_state = SWITCH_STATE_OFF;
}

static void M_SwitchOff(ITEM *const switch_item, ITEM *const lara_item)
{
    lara_item->current_anim_state = LS(LS_SWITCH_OFF);

    LARA_INFO *const lara = Lara_GetLaraInfo();
    switch (switch_item->object_id) {
    case O_SWITCH_TYPE_AIRLOCK:
        Item_SwitchToObjAnim(lara_item, LS_EXTRA_BREATH, 0, O_LARA_EXTRA);
        lara_item->current_anim_state = LS_EXTRA_BREATH;
        lara_item->goal_anim_state = LS_EXTRA_AIRLOCK;
        Item_Animate(lara_item);
        lara->extra_anim = true;
        lara->hit_direction = -1;
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

    switch_item->goal_anim_state = SWITCH_STATE_ON;
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const OBJECT *const obj = Object_Get(item->object_id);

    if (!g_Input.action || item->status != IS_INACTIVE
        || lara->gun_status != LGS_ARMLESS || lara_item->gravity
        || lara_item->current_anim_state != LS(LS_STOP)
        || !Lara_TestPosition(item, obj->bounds_func())) {
        return;
    }

    if (item->object_id == O_SWITCH_TYPE_AIRLOCK
        && item->current_anim_state == SWITCH_STATE_ON) {
        return;
    }

    M_AlignLara(lara_item, item);

    if (item->current_anim_state == SWITCH_STATE_ON) {
        M_SwitchOn(item, lara_item);
    } else {
        M_SwitchOff(item, lara_item);
    }

    if (!lara->extra_anim) {
        lara_item->goal_anim_state = LS(LS_STOP);
    }
    lara->gun_status = LGS_HANDS_BUSY;

    item->status = IS_ACTIVE;
    Item_AddActive(item_num);
    Item_Animate(item);
}

static void M_CollisionUW(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const OBJECT *const obj = Object_Get(item->object_id);

    if (!g_Input.action || item->status != IS_INACTIVE
        || (lara->water_status != LWS_UNDERWATER
            && lara->water_status != LWS_CHEAT)
        || lara->gun_status != LGS_ARMLESS
        || lara_item->current_anim_state != LS(LS_TREAD)) {
        return;
    }

    if (!Lara_TestPosition(item, obj->bounds_func())) {
        return;
    }

    if (item->current_anim_state != SWITCH_STATE_OFF
        && item->current_anim_state != SWITCH_STATE_ON) {
        return;
    }

    if (!Lara_MovePosition(item, &m_SwitchUWPosition)) {
        return;
    }

    lara_item->fall_speed = 0;
    lara_item->goal_anim_state = LS(LS_SWITCH_ON);
    do {
        Lara_Animate(lara_item);
    } while (lara_item->current_anim_state != LS(LS_SWITCH_ON));
    lara_item->goal_anim_state = LS(LS_TREAD);
    lara->gun_status = LGS_HANDS_BUSY;

    if (item->current_anim_state == SWITCH_STATE_ON) {
        item->goal_anim_state = SWITCH_STATE_OFF;
    } else {
        item->goal_anim_state = SWITCH_STATE_ON;
    }
    item->status = IS_ACTIVE;
    Item_AddActive(item_num);
    Item_Animate(item);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->flags |= IF_CODE_BITS;
    if (!Item_IsTriggerActive(item)) {
        item->goal_anim_state = SWITCH_STATE_ON;
        item->timer = 0;
    }
    Item_Animate(item);
}

bool Switch_Trigger(const int16_t item_num, const int16_t timer)
{
    ITEM *const item = Item_Get(item_num);

    if (item->object_id == O_SWITCH_TYPE_AIRLOCK) {
        if (item->status == IS_DEACTIVATED) {
            Item_RemoveActive(item_num);
            item->status = IS_INACTIVE;
            return false;
        } else if (
            (item->flags & IF_ONE_SHOT)
            || item->current_anim_state == SWITCH_STATE_OFF) {
            return false;
        }

        item->flags |= IF_ONE_SHOT;
        return true;
    }

    if (item->status != IS_DEACTIVATED) {
        return false;
    }

    if (item->current_anim_state == SWITCH_STATE_OFF && timer > 0) {
        item->timer = timer;
        if (timer != 1) {
            item->timer = timer * LOGIC_FPS;
        }
        item->status = IS_ACTIVE;
    } else {
        Item_RemoveActive(item_num);
        item->status = IS_INACTIVE;
    }
    return true;
}

static void M_SetupBase(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->save_flags = true;
    obj->save_anim = true;
}

static void M_Setup(OBJECT *const obj)
{
    M_SetupBase(obj);
    obj->collision_func = M_Collision;
    obj->bounds_func = M_Bounds;
}

static void M_SetupPushButton(OBJECT *const obj)
{
    M_Setup(obj);
    obj->enable_interpolation = false;
    obj->bounds_func = M_Bounds;
}

static void M_SetupUW(OBJECT *const obj)
{
    M_SetupBase(obj);
    obj->collision_func = M_CollisionUW;
    obj->bounds_func = M_BoundsUW;
}

REGISTER_OBJECT(O_SWITCH_TYPE_AIRLOCK, M_Setup)
REGISTER_OBJECT(O_SWITCH_TYPE_BUTTON, M_SetupPushButton)
REGISTER_OBJECT(O_SWITCH_TYPE_NORMAL, M_Setup)
REGISTER_OBJECT(O_SWITCH_TYPE_SMALL, M_Setup)
REGISTER_OBJECT(O_SWITCH_TYPE_UW, M_SetupUW)
