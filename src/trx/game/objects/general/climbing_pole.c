#include <trx/game/const.h>
#include <trx/game/input.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>

#define M_GRAB_RADIUS 100
#define M_REACH_GRAB_OFFSET 10
#define M_JUMP_GRAB_OFFSET 66

static const XYZ_32 m_PolePosition = { .x = 0, .y = 0, .z = -208 };

static const OBJECT_BOUNDS m_PoleBounds = {
    .shift = {
        .min = { .x = -WALL_L / 4, .y = +0, .z = -WALL_L / 2, },
        .max = { .x = +WALL_L / 4, .y = +0, .z = +WALL_L / 2, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
    },
};

static const OBJECT_BOUNDS *M_Bounds(void)
{
    return &m_PoleBounds;
}

static void M_CollisionStanding(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const OBJECT *const obj = Object_Get(item->object_id);

    const int16_t old_rot_y = item->rot.y;
    item->rot.y = lara_item->rot.y;

    if (Lara_TestPosition(item, obj->bounds_func())) {
        if (Lara_MovePosition(item, &m_PolePosition)) {
            Item_SwitchToAnim(lara_item, LA(LA_STAND_TO_POLE_GRAB), 0);
            lara_item->current_anim_state = LS(LS_POLE_IDLE);
            lara_item->goal_anim_state = LS(LS_POLE_IDLE);
            lara->interact_target.is_moving = false;
            lara->gun_status = LGS_HANDS_BUSY;
        } else {
            lara->interact_target.item_num = item_num;
        }
    } else if (
        lara->interact_target.is_moving
        && lara->interact_target.item_num == item_num) {
        lara->interact_target.is_moving = false;
        lara->gun_status = LGS_ARMLESS;
    }

    item->rot.y = old_rot_y;
}

static void M_CollisionAirborne(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();

    if (!Lara_TestBoundsCollide(item, M_GRAB_RADIUS)
        || !Collide_TestCollision(item, lara_item)) {
        return;
    }

    const int16_t old_rot_y = item->rot.y;
    item->rot.y = lara_item->rot.y;

    if (lara_item->current_anim_state == LS(LS_REACH)) {
        lara_item->pos.y += M_REACH_GRAB_OFFSET;
        Item_SwitchToAnim(lara_item, LA(LA_REACH_TO_POLE_GRAB), 0);
    } else {
        lara_item->pos.y += M_JUMP_GRAB_OFFSET;
        Item_SwitchToAnim(lara_item, LA(LA_JUMP_UP_TO_POLE_GRAB), 0);
    }

    // Lara_AlignPosition() refuses to move Lara mid-air, so snap to the pole
    // axis directly.
    lara_item->pos.x = item->pos.x;
    lara_item->pos.z = item->pos.z;
    lara_item->rot = item->rot;
    lara_item->gravity = false;
    lara_item->fall_speed = 0;
    lara_item->current_anim_state = LS(LS_POLE_IDLE);
    lara_item->goal_anim_state = LS(LS_POLE_IDLE);
    lara->gun_status = LGS_HANDS_BUSY;
    item->rot.y = old_rot_y;
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();

    if ((g_Input.action && lara->gun_status == LGS_ARMLESS
         && lara_item->current_anim_state == LS(LS_STOP)
         && Item_TestAnimEqual(lara_item, LA(LA_STAND_IDLE)))
        || (lara->interact_target.is_moving
            && lara->interact_target.item_num == item_num)) {
        M_CollisionStanding(item_num, lara_item, coll);
    } else if (
        g_Input.action && lara->gun_status == LGS_ARMLESS && lara_item->gravity
        && lara_item->fall_speed > 0
        && (lara_item->current_anim_state == LS(LS_REACH)
            || lara_item->current_anim_state == LS(LS_JUMP_UP))) {
        M_CollisionAirborne(item_num, lara_item, coll);
    } else {
        const LARA_STATE_ID state = LS_U(lara_item->current_anim_state);
        if ((state < LS_POLE_IDLE || state > LS_POLE_RIGHT)
            && state != LS_JUMP_BACK) {
            Object_Collision(item_num, lara_item, coll);
        }
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->collision_func = M_Collision;
    obj->bounds_func = M_Bounds;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_CLIMBING_POLE, M_Setup)
