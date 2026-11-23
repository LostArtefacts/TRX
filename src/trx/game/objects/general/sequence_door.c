#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/general/door.h>
#include <trx/game/objects/general/switch_sequence.h>

#define M_FIRST_RESULT 0
#define M_FALLBACK_RESULT 2

typedef struct {
    bool is_armed;
} M_PRIV;

static const int32_t m_FirstEntries[SWITCH_SEQUENCE_LENGTH] = { 0, 1, 2 };
static const int32_t m_FallbackEntries[SWITCH_SEQUENCE_LENGTH] = { 1, 0, 2 };

// The door portals come first in the priv, so that the door module can use
// them; this object keeps its own state after them.
static M_PRIV *M_GetPriv(const ITEM *const item)
{
    return (M_PRIV *)((char *)item->priv + Door_GetPrivSize());
}

static int32_t M_GetOCB(const ITEM *const item)
{
    TRX_VALUE value;
    if (!ObjectProperty_GetItemValue(item, "ocb", &value)) {
        return 0;
    }
    return value.as_int;
}

static bool M_IsEntered(const int32_t entries[SWITCH_SEQUENCE_LENGTH])
{
    const SWITCH_SEQUENCE_STATE *const state = SwitchSequence_GetState();
    for (int32_t i = 0; i < SWITCH_SEQUENCE_LENGTH; i++) {
        if (state->entries[i] != entries[i]) {
            return false;
        }
    }
    return true;
}

static bool M_IsResult(const int32_t ocb)
{
    return SwitchSequence_IsComplete() && ocb != SWITCH_SEQUENCE_NO_RESULT
        && SwitchSequence_GetResult() == ocb;
}

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = M_GetPriv(item);
    MUST(JSON_READ_OPT(io, "is_armed", &p->is_armed));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = M_GetPriv(item);
    JSONW_WRITE(io, "is_armed", p->is_armed);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = M_GetPriv(item);
    const int32_t ocb = M_GetOCB(item);

    if (!p->is_armed) {
        if (item->current_anim_state == DOOR_STATE_CLOSED) {
            // Also shuts a door that a wrong order has closed. The original
            // game leaves its portals open, so Lara can walk through it.
            Door_ShutPortals(item);
            if (M_IsResult(ocb)) {
                if (ocb != M_FIRST_RESULT && ocb != M_FALLBACK_RESULT
                    && !SwitchSequence_IsUsed(M_FIRST_RESULT)) {
                    SwitchSequence_SetEntries(m_FallbackEntries);
                    return;
                }
                item->goal_anim_state = DOOR_STATE_OPEN;
                p->is_armed = true;
            }
        }
    } else if (Item_IsTriggerActive(item)) {
        if (item->current_anim_state == DOOR_STATE_CLOSED) {
            item->goal_anim_state = DOOR_STATE_OPEN;
        } else {
            Door_OpenPortals(item);
            if (SwitchSequence_IsComplete()) {
                if (M_IsResult(ocb) && M_IsEntered(m_FirstEntries)) {
                    SwitchSequence_BeginReset();
                    SwitchSequence_SetUsed(ocb);
                }
            } else if (
                ocb == M_FALLBACK_RESULT
                && (SwitchSequence_GetEnteredCount() == 1
                    || SwitchSequence_GetEnteredCount() == 2)) {
                item->trigger.spent = false;
                item->trigger.anti_spent = false;
                item->goal_anim_state = DOOR_STATE_CLOSED;
                p->is_armed = false;
            }
        }
    } else if (item->current_anim_state == DOOR_STATE_OPEN) {
        item->goal_anim_state = DOOR_STATE_CLOSED;
    } else {
        if (M_IsResult(ocb)) {
            SwitchSequence_BeginReset();
            if (ocb != M_FALLBACK_RESULT) {
                SwitchSequence_SetUsed(ocb);
            }
        }
        Door_ShutPortals(item);
    }

    Item_Animate(item);
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = Door_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Door_Collision;
    obj->draw_func = Item_DrawUnclipped;
    obj->priv_size = Door_GetPrivSize() + sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_flags = true;
    obj->save_anim = true;
}

REGISTER_OBJECT(O_SEQUENCE_DOOR, M_Setup)
