#include <trx/config.h>
#include <trx/core/math.h>
#include <trx/game/input.h>
#include <trx/game/lara.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>

#define M_MAX_SPEED 100
#define M_ACCELERATION 5

typedef struct {
    GAME_VECTOR old_pos;
} M_PRIV;

typedef enum {
    M_STATE_EMPTY,
    M_STATE_GRAB,
    M_STATE_HANG,
} M_STATE;

static const XYZ_32 m_Position = {
    .x = 0,
    .y = 0,
    .z = WALL_L / 2 - 141,
};

static const OBJECT_BOUNDS m_DefaultBounds = {
    .shift = {
        .min = { .x = -WALL_L / 4, .y = -100, .z = +WALL_L / 4, },
        .max = { .x = +WALL_L / 4, .y = +100, .z = +WALL_L / 2, },
    },
    .rot = {
        .min = { .x = +0, .y = -25 * DEG_1, .z = +0, },
        .max = { .x = +0, .y = +25 * DEG_1, .z = +0, },
    },
};

static const OBJECT_BOUNDS m_ControlledBounds = {
    .shift = {
        .min = { .x = -WALL_L / 4, .y = -100, .z = +0, },
        .max = { .x = +WALL_L / 4, .y = +100, .z = +WALL_L / 2, },
    },
    .rot = {
        .min = { .x = +0, .y = -25 * DEG_1, .z = +0, },
        .max = { .x = +0, .y = +25 * DEG_1, .z = +0, },
    },
};

static const OBJECT_BOUNDS *M_Bounds(void)
{
    return g_Config.gameplay.enable_walk_to_items ? &m_ControlledBounds
                                                  : &m_DefaultBounds;
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->old_pos.pos = item->pos;
    p->old_pos.room_num = item->room_num;
}

static void M_LetGo(const ITEM *const item, ITEM *const lara_item)
{
    if (lara_item->current_anim_state != LS(LS_ZIPLINE)) {
        return;
    }
    Lara_AnimateUntil(lara_item, LS(LS_JUMP_FORWARD));
    lara_item->gravity = true;
    lara_item->speed = item->fall_speed;
    lara_item->fall_speed = item->fall_speed >> 2;
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_IsInPlay(item)) {
        return;
    }

    if (!item->trigger.spent) {
        const M_PRIV *const p = item->priv;
        item->pos = p->old_pos.pos;
        Item_UpdateRoom(item_num, p->old_pos.room_num);
        item->goal_anim_state = M_STATE_GRAB;
        item->current_anim_state = M_STATE_GRAB;
        Item_SwitchToAnim(item, 0, 0);
        Item_RemoveSimulated(item_num);
        return;
    }

    if (item->current_anim_state == M_STATE_GRAB) {
        Item_Animate(item);
        return;
    }

    Item_Animate(item);
    if (item->fall_speed < M_MAX_SPEED) {
        item->fall_speed += M_ACCELERATION;
    }

    item->pos.y += item->fall_speed >> 2;
    item->pos = XYZ_32_OffsetYaw(item->pos, item->rot.y, item->fall_speed);

    int16_t room_num = item->room_num;
    Room_GetSector(item->pos, &room_num);
    Item_UpdateRoom(item_num, room_num);

    ITEM *const lara_item = Lara_GetItem();
    const bool lara_on_zipline =
        lara_item->current_anim_state == LS(LS_ZIPLINE);
    if (lara_on_zipline) {
        lara_item->pos = item->pos;
        if (!g_Input.action) {
            M_LetGo(item, lara_item);
        }
    }

    XYZ_32 pos = item->pos;
    pos.y += STEP_L >> 2;
    pos = XYZ_32_OffsetYaw(pos, item->rot.y, WALL_L);

    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    if (Room_GetHeight(sector, pos) > pos.y + STEP_L
        && Room_GetCeiling(sector, pos) < pos.y - STEP_L) {
        Sound_Effect(SFX_ZIPLINE_GO, &item->pos, SPM_ALWAYS);
        return;
    }

    if (lara_on_zipline) {
        M_LetGo(item, lara_item);
    }
    Sound_Effect(SFX_ZIPLINE_STOP, &item->pos, SPM_ALWAYS);
    Item_RemoveSimulated(item_num);
    item->trigger.spent = false;
}

static void M_Grab(
    ITEM *const zip_item, ITEM *const lara_item, LARA_INFO *const lara)
{
    Item_SwitchToAnim(lara_item, LA(LA_ZIPLINE_GRAB), 0);
    lara_item->current_anim_state == LS(LS_PULL_UP);
    lara_item->goal_anim_state = LS(LS_ZIPLINE);
    lara->gun_status = LGS_HANDS_BUSY;

    Item_AddSimulated(Item_GetIndex(zip_item));
    zip_item->trigger.spent = true;
}

static void M_CollisionControlled(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    if (!Lara_Interact_CanControl(LARA_INTERACT_SWITCH, item_num)) {
        Object_Collision(item_num, lara_item, coll);
        return;
    }

    ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const OBJECT *const obj = Object_Get(item->object_id);

    if (Lara_TestPosition(item, obj->bounds_func())) {
        if (Lara_MovePosition(item, &m_Position)) {
            Lara_Interact_FinishControl(LARA_INTERACT_SWITCH);
            M_Grab(item, lara_item, lara);
        } else {
            lara->interact_target.item_num = item_num;
        }
    } else if (
        lara->interact_target.is_moving
        && lara->interact_target.item_num == item_num) {
        lara->interact_target.is_moving = false;
        lara->gun_status = LGS_ARMLESS;
    }
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_IsInactive(item)) {
        return;
    }

    if (g_Config.gameplay.enable_walk_to_items) {
        M_CollisionControlled(item_num, lara_item, coll);
        return;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (!g_Input.action || lara->gun_status != LGS_ARMLESS || lara_item->gravity
        || lara_item->current_anim_state != LS(LS_STOP)) {
        return;
    }

    const OBJECT *const obj = Object_Get(item->object_id);
    if (!Lara_TestPosition(item, obj->bounds_func())) {
        return;
    }

    Lara_AlignPosition(item, &m_Position);
    M_Grab(item, lara_item, lara);
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = M_Collision;
    obj->bounds_func = M_Bounds;
    obj->priv_size = sizeof(M_PRIV);
    obj->save_position = true;
    obj->save_flags = true;
    obj->save_anim = true;
}

REGISTER_OBJECT(O_ZIPLINE_HANDLE, M_Setup)
