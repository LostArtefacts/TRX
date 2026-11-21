#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/camera.h>
#include <trx/game/const.h>
#include <trx/game/input.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>

// clang-format off
#define M_TARGET_ANIM_FRONT 0
#define M_TARGET_ANIM_BACK  4

#define M_SFX_END_1   43
#define M_SFX_START_2 58
#define M_SFX_END_2   115

#define M_CAM_DIST   WALL_L
#define M_CAM_HEIGHT (WALL_L * 2)
// clang-format on

typedef enum {
    // clang-format off
    M_ANIM_STILL          = 0,
    M_ANIM_BACK_PUSH      = 1,
    M_ANIM_BACK_PUSH_END  = 2,
    M_ANIM_FRONT_GRAB     = 4,
    M_ANIM_FRONT_PUSH     = 5,
    M_ANIM_FRONT_PUSH_END = 6,
    // clang-format on
} M_ANIM;

typedef enum {
    M_STATE_IDLE = 0,
} M_STATE;

typedef struct {
    bool is_behind;
    XYZ_32 cam_pos;
} M_PRIV;

static const OBJECT_BOUNDS m_FrontBounds = {
    .shift = {
        .min = { .x = +WALL_L / 2, .y = +0, .z = -WALL_L / 2, },
        .max = { .x = +896,        .y = +0, .z = +0, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
    },
};

static const OBJECT_BOUNDS m_BackBounds = {
    .shift = {
        .min = { .x = +WALL_L / 2, .y = +0, .z = +0, },
        .max = { .x = +896,        .y = +0, .z = +WALL_L / 2, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
    },
};

static const XYZ_32 m_FrontPosition = { .x = 650, .y = 0, .z = -138 };
static const XYZ_32 m_BackPosition = { .x = 650, .y = 0, .z = +138 };

static const BOUNDS_16 m_PushBounds1 = {
    .min = { .x = -WALL_L / 2, .y = -WALL_L / 2, .z = -WALL_L / 2, },
    .max = { .x = +WALL_L / 2, .y = +0,          .z = +WALL_L / 2, },
};

static const BOUNDS_16 m_PushBounds2 = {
    .min = { .x = +STEP_L, .y = -WALL_L / 2, .z = -128, },
    .max = { .x = +WALL_L, .y = +0,          .z = +128, },
};

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "is_behind", &p->is_behind));
    MUST(JSON_READ_OPT(io, "cam_pos", &p->cam_pos));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "is_behind", p->is_behind);
    JSONW_WRITE(io, "cam_pos", p->cam_pos);
}

static void M_AssertCamera(const ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    Camera_UpdateDynamicFixedObject(p->cam_pos, item->room_num);
    g_Camera.num = Camera_GetDynamicFixedObjectIdx();
    g_Camera.type = CAM_FIXED;
    g_Camera.flags = CF_FOLLOW_CENTRE;
}

static bool M_IsPushSoundFrame(const ITEM *const lara_item, const int16_t anim)
{
    if (!Item_TestAnimEqual(lara_item, anim)) {
        return false;
    }
    return Item_TestFrameRange(lara_item, 0, M_SFX_END_1)
        || Item_TestFrameRange(lara_item, M_SFX_START_2, M_SFX_END_2);
}

static void M_SetTargetDirection(const ITEM *const item, const bool is_behind)
{
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
    if (sector->trigger == nullptr) {
        return;
    }

    for (const TRIGGER_CMD *cmd = sector->trigger->command; cmd != nullptr;
         cmd = cmd->next_cmd) {
        if (cmd->type != TO_ITEM) {
            continue;
        }

        ITEM *const target_item = Item_Get((int16_t)(intptr_t)cmd->parameter);
        if (target_item == item) {
            continue;
        }

        if (!Item_IsTriggerActive(target_item)) {
            Item_SwitchToAnim(
                target_item,
                is_behind ? M_TARGET_ANIM_BACK : M_TARGET_ANIM_FRONT, 0);
        }
        return;
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    ITEM *const lara_item = Lara_GetItem();
    const M_PRIV *const p = item->priv;

    int16_t push_end_anim;
    int16_t lara_push_anim;
    int16_t lara_end_anim;
    int16_t item_push_anim;
    int16_t turn;
    if (p->is_behind) {
        push_end_anim = M_ANIM_BACK_PUSH_END;
        item_push_anim = M_ANIM_BACK_PUSH;
        lara_push_anim = LA(LA_TURNSWITCH_PUSH_RIGHT_START);
        lara_end_anim = LA(LA_TURNSWITCH_PUSH_RIGHT_END);
        turn = DEG_90;
    } else {
        push_end_anim = M_ANIM_FRONT_PUSH_END;
        item_push_anim = M_ANIM_FRONT_PUSH;
        lara_push_anim = LA(LA_TURNSWITCH_PUSH_LEFT_START);
        lara_end_anim = LA(LA_TURNSWITCH_PUSH_LEFT_END);
        turn = -DEG_90;
    }

    if (Item_TestAnimEqual(item, push_end_anim)) {
        item->rot.y += turn;
        if (g_Input.action) {
            Item_SwitchToAnim(lara_item, lara_push_anim, 0);
            Item_SwitchToAnim(item, item_push_anim, 0);
        }
    }

    const bool is_done = Item_TestAnimEqual(lara_item, lara_end_anim)
        && Item_TestFrameEqual(lara_item, -1);

    if (M_IsPushSoundFrame(lara_item, lara_push_anim)) {
        Sound_Effect(SFX_PUSHABLE_MOVE, &item->pos, SPM_ALWAYS);
    }

    Item_Animate(item);

    if (!is_done) {
        M_AssertCamera(item);
        return;
    }

    Item_SwitchToAnim(lara_item, LA(LA_STAND_IDLE), 0);
    lara_item->current_anim_state = LS(LS_STOP);
    Item_SwitchToAnim(item, M_ANIM_STILL, 0);
    Item_RemoveSimulated(item_num);
    Lara_GetLaraInfo()->gun_status = LGS_ARMLESS;
}

static bool M_Grab(
    ITEM *const item, ITEM *const lara_item, const bool is_behind)
{
    if (!Lara_Interact_MoveTo(
            item, is_behind ? &m_BackPosition : &m_FrontPosition)) {
        return false;
    }

    M_PRIV *const p = item->priv;
    p->is_behind = is_behind;
    p->cam_pos = XYZ_32_OffsetYaw(
        item->pos, item->rot.y, is_behind ? M_CAM_DIST : -M_CAM_DIST);
    p->cam_pos.y = item->pos.y - M_CAM_HEIGHT;
    if (is_behind) {
        Item_SwitchToAnim(lara_item, LA(LA_TURNSWITCH_GRAB_LEFT), 0);
    } else {
        Item_SwitchToAnim(lara_item, LA(LA_TURNSWITCH_GRAB_RIGHT), 0);
        Item_SwitchToAnim(item, M_ANIM_FRONT_GRAB, 0);
    }
    return true;
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);

    bool is_grabbed = false;
    if (Lara_Interact_HasActiveTarget(item_num)
        || (item->current_anim_state == M_STATE_IDLE
            && Lara_Interact_CanControl(LARA_INTERACT_SWITCH, item_num))) {
        if (Lara_TestPosition(item, &m_FrontBounds)) {
            is_grabbed = M_Grab(item, lara_item, false);
        } else {
            lara_item->rot.y += DEG_180;
            if (Lara_TestPosition(item, &m_BackBounds)) {
                is_grabbed = M_Grab(item, lara_item, true);
            } else {
                Lara_Interact_Release(item_num);
            }
            lara_item->rot.y += DEG_180;
        }
    }

    if (is_grabbed) {
        Lara_Interact_FinishControl(LARA_INTERACT_SWITCH);
        Lara_GetLaraInfo()->torso_rot = (XYZ_16) {};
        lara_item->current_anim_state = LS(LS_TURN_SWITCH);
        Item_AddSimulated(item_num);
        M_AssertCamera(item);
        const M_PRIV *const p = item->priv;
        M_SetTargetDirection(item, p->is_behind);
    } else if (
        coll->enable_baddie_push
        && Item_TestBoundsCollide(item, lara_item, coll->radius)) {
        Lara_Col_ItemPushEx(item, coll, false, false, &m_PushBounds1);
        Lara_Col_ItemPushEx(item, coll, false, false, &m_PushBounds2);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = M_Collision;
    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_flags = true;
    obj->save_anim = true;
}

REGISTER_OBJECT(O_TURN_SWITCH, M_Setup)
