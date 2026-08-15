#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/input.h>
#include <trx/game/lara.h>
#include <trx/version.h>

// clang-format off
#define M_MIN_PULLS         1
#define M_PULLEY_GRAB_FRAME 1
#define M_PULLEY_PULL_FRAME 15
#define M_LARA_GRAB_FRAME   1
#define M_LARA_PULL_FRAME   1
#define M_LARA_RESET_FRAME  44
// clang-format on

typedef struct {
    bool is_on;
    bool is_locked;
    bool is_single_use;
    int32_t required_pulls;
    int32_t pulls_done;
} M_PRIV;

static const OBJECT_BOUNDS m_Bounds = {
    .shift = {
        .min = { .x = -STEP_L, .y = 0, .z = -STEP_L * 2, },
        .max = { .x = +STEP_L, .y = 0, .z = +STEP_L * 2, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
    },
};

static const XYZ_32 m_Position = { .x = 0, .y = 0, .z = -148 };

// What is left to pull. A requirement raised mid-pull asks for the difference;
// one lowered past what has already been done leaves nothing to do.
static int32_t M_RemainingPulls(const M_PRIV *const p)
{
    return MAX(0, p->required_pulls - p->pulls_done);
}

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "is_on", &p->is_on));
    MUST(JSON_READ_OPT(io, "is_locked", &p->is_locked));
    MUST(JSON_READ_OPT(io, "pulls_done", &p->pulls_done));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "is_on", p->is_on);
    JSONW_WRITE(io, "is_locked", p->is_locked);
    JSONW_WRITE(io, "pulls_done", p->pulls_done);
}

// Fewer pulls than the minimum would leave the pulley unusable.
static const char *M_CheckRequiredPulls(const TRX_VALUE *const in)
{
    return in->as_int < M_MIN_PULLS
        ? "fewer pulls than that would leave the pulley unusable"
        : nullptr;
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;

    // The visibility initialisation flag is temporarily used to indicate that
    // the pulley cannot be operated until later events take place.
    // TODO: implement as an event during O_OBELISK implementation.
    p->is_locked = !item->is_visible;
    if (p->is_locked) {
        Item_SetVisible(item, true);
    }
}

static void M_ControlWithLara(ITEM *const item)
{
    ITEM *const lara_item = Lara_GetItem();
    M_PRIV *const p = item->priv;
    if (g_Input.action && M_RemainingPulls(p) != 0) {
        lara_item->goal_anim_state = LS(LS_PULLEY);
    } else {
        lara_item->goal_anim_state = LS(LS_STOP);
    }

    const bool is_pulling = Item_TestAnimEqual(lara_item, LA(LA_PULLEY_PULL));
    const int16_t lara_frame = Item_GetRelativeFrame(lara_item);
    if (Item_TestAnimEqual(lara_item, LA(LA_PULLEY_GRAB))
        && lara_frame == M_LARA_GRAB_FRAME) {
        item->frame_num = M_PULLEY_GRAB_FRAME;
    } else if (is_pulling && lara_frame == M_LARA_PULL_FRAME) {
        item->frame_num = M_PULLEY_PULL_FRAME;
    }

    Item_Animate(item);

    if (!is_pulling) {
        return;
    }

    if (M_RemainingPulls(p) == 0 || p->is_locked) {
        return;
    }

    if (lara_frame != M_LARA_RESET_FRAME) {
        return;
    }

    p->pulls_done++;
    if (M_RemainingPulls(p) == 0) {
        if (p->is_single_use) {
            item->trigger.spent = true;
        } else {
            // Potentially multiple pulls to activate, but one to deactivate.
            // Turning it back off takes a single pull.
            p->is_on = !p->is_on;
            p->pulls_done = p->is_on ? p->required_pulls - M_MIN_PULLS : 0;
        }
        Item_SetFinished(item, true);
    }
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    LARA_INFO *const lara = Lara_GetLaraInfo();

    if (!item->trigger.spent
        && Lara_Interact_CanControl(LARA_INTERACT_SWITCH, item_num)) {
        const XYZ_16 old_rot = item->rot;
        item->rot.y = lara_item->rot.y;

        if (Lara_TestPosition(item, &m_Bounds)) {
            if (p->is_locked) {
                Lara_RefuseInteraction();
            } else {
                if (Lara_MovePosition(item, &m_Position)) {
                    Item_SwitchToAnim(lara_item, LA(LA_PULLEY_GRAB), 0);
                    lara_item->current_anim_state = LS(LS_PULLEY);
                    Lara_Interact_FinishControl(LARA_INTERACT_SWITCH);
                    Item_AddSimulated(item_num);
                }
                lara->interact_target.item_num = item_num;
            }
        } else if (
            lara->interact_target.is_moving
            && lara->interact_target.item_num == item_num) {
            lara->interact_target.is_moving = false;
            lara->gun_status = LGS_ARMLESS;
        }

        item->rot = old_rot;
    } else if (lara_item->current_anim_state != LS(LS_PULLEY)) {
        Object_Collision(item_num, lara_item, coll);
    }

    if (lara->interact_target.item_num == item_num) {
        M_ControlWithLara(item);
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->trigger.mask = TRIGGER_MASK_ALL;
    if (!Item_IsTriggerActive(item)) {
        item->timer = 0;
    }

    if (g_TRVersion >= 3 && item->trigger.switch_spent) {
        item->trigger.switch_spent = false;
        item->trigger.spent = true;
    }

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->interact_target.item_num != item_num) {
        Item_RemoveSimulated(item_num);
        Item_SwitchToAnim(item, 0, 0);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->collision_func = M_Collision;
    obj->control_func = M_Control;
    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_flags = true;
    obj->save_anim = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_CHECKED(
            M_PRIV, required_pulls, M_MIN_PULLS, M_CheckRequiredPulls,
            "The number of pulls required before activating the trigger under "
            "the pulley. Value range: minimum 1"),
        OBJECT_PROPERTY(
            M_PRIV, is_single_use, false,
            "Whether or not the pulley can only be used once."));
}

REGISTER_OBJECT(O_SWITCH_TYPE_PULLEY, M_Setup)
