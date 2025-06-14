#include "game/lara/state.h"

#include "debug.h"

static void (*m_StateRoutines[LS_NUMBER_OF])(ITEM *item, COLL_INFO *coll) = {};

void Lara_State_Register(
    const LARA_STATE state,
    void (*const handle_func)(ITEM *item, COLL_INFO *coll))
{
    ASSERT(state >= 0 && state < LS_NUMBER_OF);
    m_StateRoutines[state] = handle_func;
}

void Lara_State_Update(ITEM *const item, COLL_INFO *const coll)
{
    if (m_StateRoutines[item->current_anim_state] != nullptr) {
        m_StateRoutines[item->current_anim_state](item, coll);
    }
}
