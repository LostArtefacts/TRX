#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/objects/general/switch_sequence.h>

#define M_ANIM_DOWN 2

typedef enum {
    M_STATE_DOWN = 0,
    M_STATE_UP = 1,
} M_STATE;

typedef struct {
    bool is_pressed;
    bool is_entered;
} M_PRIV;

static const OBJECT_BOUNDS m_Bounds = {
    .shift = {
        .min = { .x = -384, .y = +0,     .z = +0, },
        .max = { .x = +384, .y = +STEP_L, .z = +WALL_L / 2, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
    },
};

static const XYZ_32 m_Position = { .x = 0, .y = STEP_L, .z = 100 };

static int32_t M_GetOCB(const ITEM *const item)
{
    TRX_VALUE value;
    if (!ObjectProperty_GetItemValue(item, "ocb", &value)) {
        return 0;
    }
    return value.as_int;
}

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "is_pressed", &p->is_pressed));
    MUST(JSON_READ_OPT(io, "is_entered", &p->is_entered));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "is_pressed", p->is_pressed);
    JSONW_WRITE(io, "is_entered", p->is_entered);
}

static bool M_CanPress(const int16_t item_num)
{
    if (Lara_Interact_HasActiveTarget(item_num)) {
        return true;
    }

    const ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    return !p->is_pressed && !item->trigger.spent
        && item->current_anim_state == M_STATE_UP && SwitchSequence_IsOpen()
        && Lara_Interact_CanControl(LARA_INTERACT_FLOOR_SWITCH, item_num);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;

    if (Item_TestAnimEqual(item, M_ANIM_DOWN) && SwitchSequence_IsOpen()
        && !p->is_entered) {
        p->is_entered = true;
        SwitchSequence_Enter(M_GetOCB(item));
    } else if (SwitchSequence_IsResetting()) {
        p->is_entered = false;
        p->is_pressed = false;
        item->goal_anim_state = M_STATE_UP;
        SwitchSequence_StepReset();
    }

    Item_Animate(item);

    const ITEM *const lara_item = Lara_GetItem();
    if (Item_TestAnimEqual(lara_item, LA(LA_GIANT_BUTTON_PUSH))
        && Item_TestFrameEqual(lara_item, -1)) {
        Lara_GetLaraInfo()->gun_status = LGS_ARMLESS;
    }
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    if (!M_CanPress(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    if (Lara_Interact_Reach(item, &m_Bounds, &m_Position)
        != LARA_REACH_ARRIVED) {
        return;
    }

    Item_SwitchToAnim(lara_item, LA(LA_GIANT_BUTTON_PUSH), 0);
    lara_item->current_anim_state = LS(LS_SWITCH_ON);
    lara_item->goal_anim_state = LS(LS_STOP);
    item->goal_anim_state = M_STATE_DOWN;

    M_PRIV *const p = item->priv;
    p->is_pressed = true;
    Item_AddSimulated(item_num);
    Item_Animate(item);

    Lara_Interact_FinishControl(LARA_INTERACT_FLOOR_SWITCH);
    Lara_GetLaraInfo()->torso_rot = (XYZ_16) {};
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

REGISTER_OBJECT(O_SEQUENCE_SWITCH_1, M_Setup)
REGISTER_OBJECT(O_SEQUENCE_SWITCH_2, M_Setup)
REGISTER_OBJECT(O_SEQUENCE_SWITCH_3, M_Setup)
