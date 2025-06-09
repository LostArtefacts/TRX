#include "game/lara/col.h"

#include "debug.h"
#include "game/lara.h"

static void (*m_CollisionRoutines[LS_NUMBER_OF])(
    ITEM *item, COLL_INFO *coll) = {};

void Lara_Col_Register(
    const LARA_STATE state,
    void (*const handle_func)(ITEM *item, COLL_INFO *coll))
{
    ASSERT(state >= 0 && state < LS_NUMBER_OF);
    m_CollisionRoutines[state] = handle_func;
}

void Lara_Col_Update(ITEM *const item, COLL_INFO *const coll)
{
    if (m_CollisionRoutines[item->current_anim_state] != nullptr) {
        m_CollisionRoutines[item->current_anim_state](item, coll);
    }
}
