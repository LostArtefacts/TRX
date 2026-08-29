#include <trx/game/const.h>
#include <trx/game/input.h>
#include <trx/game/lara.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/general/door.h>

typedef enum {
    M_STATE_CLOSED = 0,
    M_STATE_OPEN = 1,
    M_STATE_PUSH_OPEN = 2,
    M_STATE_PULL_OPEN = 3,
} M_STATE;

static const OBJECT_BOUNDS m_DoorBounds = {
    .shift = {
        .min = { .x = -384, .y = +0, .z = -WALL_L, },
        .max = { .x = +384, .y = +0, .z = +WALL_L / 2, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
    },
};

static const OBJECT_BOUNDS m_UWDoorBounds = {
    .shift = {
        .min = { .x = -256, .y = -WALL_L, .z = -WALL_L, },
        .max = { .x = +256, .y = +0, .z = +0, },
    },
    .rot = {
        .min = { .x = -80 * DEG_1, .y = -80 * DEG_1, .z = -80 * DEG_1, },
        .max = { .x = +80 * DEG_1, .y = +80 * DEG_1, .z = +80 * DEG_1, },
    },
};

static const XYZ_32 m_PullPosition = { .x = -201, .y = 0, .z = 322 };
static const XYZ_32 m_PushPosition = { .x = 201, .y = 0, .z = -702 };
static const XYZ_32 m_KickPosition = { .x = 0, .y = 0, .z = -917 };
static const XYZ_32 m_DoubleDoorsPosition = { .x = 0, .y = 0, .z = 220 };
static const XYZ_32 m_UWDoorPosition = { .x = -251, .y = -760, .z = -46 };

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    Door_OpenPortals(item);
    Item_Animate(item);
}

static bool M_Open(
    ITEM *const lara_item, const int16_t item_num,
    const LARA_ANIMATION_ID lara_anim, const M_STATE door_goal_state)
{
    ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();

    Item_SwitchToAnim(lara_item, LA(lara_anim), 0);
    item->goal_anim_state = door_goal_state;
    Item_AddSimulated(item_num);
    lara->interact_target.is_moving = false;
    lara->gun_status = LGS_HANDS_BUSY;
    return true;
}

static void M_PushPullKickCollision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();

    if (Lara_Interact_CanControl(LARA_INTERACT_DOOR, item_num)) {
        const bool pull = lara_item->room_num == item->room_num;
        if (pull) {
            item->rot.y += DEG_180;
        }

        if (Lara_TestPosition(item, &m_DoorBounds)) {
            bool going = false;
            if (pull) {
                if (Lara_MovePosition(item, &m_PullPosition)) {
                    going = M_Open(
                        lara_item, item_num, LA_DOOR_OPEN_BACK,
                        M_STATE_PULL_OPEN);
                }
            } else if (
                item->object_id == O_KICK_DOOR_1
                || item->object_id == O_KICK_DOOR_2) {
                if (Lara_MovePosition(item, &m_KickPosition)) {
                    going = M_Open(
                        lara_item, item_num, LA_DOOR_KICK, M_STATE_PUSH_OPEN);
                }
            } else {
                if (Lara_MovePosition(item, &m_PushPosition)) {
                    going = M_Open(
                        lara_item, item_num, LA_DOOR_OPEN_FORWARD,
                        M_STATE_PUSH_OPEN);
                }
            }

            if (going) {
                lara_item->current_anim_state = LS(LS_CONTROLLED);
                lara_item->goal_anim_state = LS(LS_STOP);
            } else {
                lara->interact_target.item_num = item_num;
            }
        } else if (Lara_Interact_HasActiveTarget(item_num)) {
            lara->interact_target.is_moving = false;
            lara->gun_status = LGS_ARMLESS;
        }

        if (pull) {
            item->rot.y += DEG_180;
        }
    } else if (item->current_anim_state == M_STATE_CLOSED) {
        Door_Collision(item_num, lara_item, coll);
    }
}

static void M_DoubleDoorsCollision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();

    if (!Lara_Interact_CanControl(LARA_INTERACT_DOOR, item_num)) {
        return;
    }

    item->rot.y += DEG_180;

    if (Lara_TestPosition(item, &m_DoorBounds)) {
        if (Lara_MovePosition(item, &m_DoubleDoorsPosition)) {
            Item_SwitchToAnim(lara_item, LA(LA_DOUBLEDOORS_PUSH), 0);
            lara_item->current_anim_state = LS(LS_PUSH_DOORS);
            Item_AddSimulated(item_num);
            Lara_Interact_FinishControl(LARA_INTERACT_DOOR);
        } else {
            lara->interact_target.item_num = item_num;
        }
    } else if (Lara_Interact_HasActiveTarget(item_num)) {
        lara->interact_target.is_moving = false;
        lara->gun_status = LGS_ARMLESS;
    }

    item->rot.y += DEG_180;
}

static void M_UWDoorCollision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();

    const bool can_interact = (g_Input.action && !Item_IsInPlay(item)
                               && (lara->water_status == LWS_UNDERWATER
                                   || lara->water_status == LWS_CHEAT)
                               && lara->gun_status == LGS_ARMLESS
                               && lara_item->current_anim_state == LS(LS_TREAD))
        || Lara_Interact_HasActiveTarget(item_num);

    if (can_interact) {
        lara_item->rot.y += DEG_180;

        if (Lara_TestPosition(item, &m_UWDoorBounds)) {
            if (Lara_MovePosition(item, &m_UWDoorPosition)) {
                Item_SwitchToAnim(lara_item, LA(LA_UNDERWATER_DOOR_OPEN), 0);
                lara_item->current_anim_state = LS(LS_CONTROLLED);
                lara_item->fall_speed = 0;
                item->goal_anim_state = M_STATE_OPEN;
                Item_AddSimulated(item_num);
                Item_Animate(item);
                lara->interact_target.is_moving = false;
                lara->gun_status = LGS_HANDS_BUSY;
            } else {
                lara->interact_target.item_num = item_num;
            }
        } else if (Lara_Interact_HasActiveTarget(item_num)) {
            lara->interact_target.is_moving = false;
            lara->gun_status = LGS_ARMLESS;
        }

        lara_item->rot.y += DEG_180;
    } else if (Item_IsInPlay(item)) {
        Object_Collision(item_num, lara_item, coll);
    }
}

static void M_SetupBase(OBJECT *const obj)
{
    obj->initialise_func = Door_Initialise;
    obj->control_func = M_Control;
    obj->draw_func = Object_DrawUnclippedItem;
    obj->priv_size = Door_GetPrivSize();
    obj->save_flags = true;
    obj->save_anim = true;
}

static void M_SetupPushPullKick(OBJECT *const obj)
{
    M_SetupBase(obj);
    obj->collision_func = M_PushPullKickCollision;
}

static void M_SetupDoubleDoors(OBJECT *const obj)
{
    M_SetupBase(obj);
    obj->collision_func = M_DoubleDoorsCollision;
}

static void M_SetupUWDoor(OBJECT *const obj)
{
    M_SetupBase(obj);
    obj->collision_func = M_UWDoorCollision;
}

REGISTER_OBJECT(O_PUSHPULL_DOOR_1, M_SetupPushPullKick)
REGISTER_OBJECT(O_PUSHPULL_DOOR_2, M_SetupPushPullKick)
REGISTER_OBJECT(O_KICK_DOOR_1, M_SetupPushPullKick)
REGISTER_OBJECT(O_KICK_DOOR_2, M_SetupPushPullKick)
REGISTER_OBJECT(O_DOUBLE_DOORS, M_SetupDoubleDoors)
REGISTER_OBJECT(O_UNDERWATER_DOOR, M_SetupUWDoor)
