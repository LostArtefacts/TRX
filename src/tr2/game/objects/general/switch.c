#include "game/objects/general/switch.h"

#include "game/input.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/lara/misc.h"
#include "global/vars.h"

typedef enum {
    SWITCH_STATE_OFF = 0,
    SWITCH_STATE_ON = 1,
    SWITCH_STATE_LINK = 2,
} SWITCH_STATE;

static XYZ_32 g_SmallSwitchPosition = { .x = 0, .y = 0, .z = 362 };
static XYZ_32 g_PushSwitchPosition = { .x = 0, .y = 0, .z = 292 };
static XYZ_32 m_AirlockPosition = { .x = 0, .y = 0, .z = 212 };
static XYZ_32 m_SwitchUWPosition = { .x = 0, .y = 0, .z = 108 };

static int16_t m_SwitchBounds[12] = {
    // clang-format off
    -220,
    +220,
    +0,
    +0,
    +WALL_L / 2 - 220,
    +WALL_L / 2,
    -10 * DEG_1,
    +10 * DEG_1,
    -30 * DEG_1,
    +30 * DEG_1,
    -10 * DEG_1,
    +10 * DEG_1,
    // clang-format on
};

static int16_t m_SwitchBoundsUW[12] = {
    // clang-format off
    -WALL_L,
    +WALL_L,
    -WALL_L,
    +WALL_L,
    -WALL_L,
    +WALL_L / 2,
    -80 * DEG_1,
    +80 * DEG_1,
    -80 * DEG_1,
    +80 * DEG_1,
    -80 * DEG_1,
    +80 * DEG_1,
    // clang-format on
};

static void M_AlignLara(ITEM *lara_item, ITEM *switch_item);
static void M_SwitchOn(ITEM *switch_item, ITEM *lara_item);
static void M_SwitchOff(ITEM *switch_item, ITEM *lara_item);
static void M_SetupBase(OBJECT *obj);
static void M_Setup(OBJECT *obj);
static void M_SetupPushButton(OBJECT *obj);
static void M_SetupUW(OBJECT *obj);
static void M_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
static void M_CollisionUW(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
static void M_Control(int16_t item_num);

static void M_AlignLara(ITEM *const lara_item, ITEM *const switch_item)
{
    switch (switch_item->object_id) {
    case O_SWITCH_TYPE_AIRLOCK:
        Item_AlignPosition(&m_AirlockPosition, switch_item, lara_item);
        break;

    case O_SWITCH_TYPE_SMALL:
        Item_AlignPosition(&g_SmallSwitchPosition, switch_item, lara_item);
        break;

    case O_SWITCH_TYPE_BUTTON:
        Item_AlignPosition(&g_PushSwitchPosition, switch_item, lara_item);
        break;
    }
}

static void M_SwitchOn(ITEM *const switch_item, ITEM *const lara_item)
{
    switch (switch_item->object_id) {
    case O_SWITCH_TYPE_SMALL:
        Item_SwitchToAnim(lara_item, LA_SWITCH_SMALL_DOWN, 0);
        break;

    case O_SWITCH_TYPE_BUTTON:
        Item_SwitchToAnim(lara_item, LA_BUTTON_PUSH, 0);
        break;

    default:
        Item_SwitchToAnim(lara_item, LA_WALL_SWITCH_DOWN, 0);
        break;
    }

    lara_item->current_anim_state = LS_SWITCH_ON;
    switch_item->goal_anim_state = SWITCH_STATE_OFF;
}

static void M_SwitchOff(ITEM *const switch_item, ITEM *const lara_item)
{
    lara_item->current_anim_state = LS_SWITCH_OFF;

    switch (switch_item->object_id) {
    case O_SWITCH_TYPE_AIRLOCK:
        Item_SwitchToObjAnim(lara_item, LA_EXTRA_BREATH, 0, O_LARA_EXTRA);
        lara_item->current_anim_state = LA_EXTRA_BREATH;
        lara_item->goal_anim_state = LA_EXTRA_AIRLOCK;
        Item_Animate(lara_item);
        g_Lara.extra_anim = 1;
        break;

    case O_SWITCH_TYPE_SMALL:
        Item_SwitchToAnim(lara_item, LA_SWITCH_SMALL_UP, 0);
        break;

    case O_SWITCH_TYPE_BUTTON:
        Item_SwitchToAnim(lara_item, LA_BUTTON_PUSH, 0);
        break;

    default:
        Item_SwitchToAnim(lara_item, LA_WALL_SWITCH_UP, 0);
        break;
    }

    switch_item->goal_anim_state = SWITCH_STATE_ON;
}

static void M_SetupBase(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

static void M_Setup(OBJECT *const obj)
{
    M_SetupBase(obj);
    obj->collision_func = M_Collision;
}

static void M_SetupPushButton(OBJECT *const obj)
{
    M_Setup(obj);
    obj->enable_interpolation = false;
}

static void M_SetupUW(OBJECT *const obj)
{
    M_SetupBase(obj);
    obj->collision_func = M_CollisionUW;
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    if (!g_Input.action || item->status != IS_INACTIVE
        || g_Lara.gun_status != LGS_ARMLESS || lara_item->gravity
        || lara_item->current_anim_state != LS_STOP
        || !Item_TestPosition(m_SwitchBounds, item, lara_item)) {
        return;
    }

    lara_item->rot.y = item->rot.y;

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

    if (!g_Lara.extra_anim) {
        lara_item->goal_anim_state = LS_STOP;
    }
    g_Lara.gun_status = LGS_HANDS_BUSY;

    item->status = IS_ACTIVE;
    Item_AddActive(item_num);
    Item_Animate(item);
}

static void M_CollisionUW(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);

    if (!g_Input.action || item->status != IS_INACTIVE
        || g_Lara.water_status != LWS_UNDERWATER
        || g_Lara.gun_status != LGS_ARMLESS
        || lara_item->current_anim_state != LS_TREAD) {
        return;
    }

    if (!Item_TestPosition(m_SwitchBoundsUW, item, lara_item)) {
        return;
    }

    if (item->current_anim_state != SWITCH_STATE_OFF
        && item->current_anim_state != SWITCH_STATE_ON) {
        return;
    }

    if (!Lara_MovePosition(&m_SwitchUWPosition, item, lara_item)) {
        return;
    }

    lara_item->fall_speed = 0;
    lara_item->goal_anim_state = LS_SWITCH_ON;
    do {
        Lara_Animate(lara_item);
    } while (lara_item->current_anim_state != LS_SWITCH_ON);
    lara_item->goal_anim_state = LS_TREAD;
    g_Lara.gun_status = LGS_HANDS_BUSY;

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
            item->timer = timer * FRAMES_PER_SECOND;
        }
        item->status = IS_ACTIVE;
    } else {
        Item_RemoveActive(item_num);
        item->status = IS_INACTIVE;
    }
    return true;
}

REGISTER_OBJECT(O_SWITCH_TYPE_AIRLOCK, M_Setup)
REGISTER_OBJECT(O_SWITCH_TYPE_BUTTON, M_SetupPushButton)
REGISTER_OBJECT(O_SWITCH_TYPE_NORMAL, M_Setup)
REGISTER_OBJECT(O_SWITCH_TYPE_SMALL, M_Setup)
REGISTER_OBJECT(O_SWITCH_TYPE_UW, M_SetupUW)
