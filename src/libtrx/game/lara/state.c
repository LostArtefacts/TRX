#include "game/lara/state.h"

#include "debug.h"
#include "game/lara.h"

static void (*m_StateRoutines[LS_NUMBER_OF])(ITEM *item, COLL_INFO *coll) = {};
static void (*m_ExtraRoutines[LS_EXTRA_NUMBER_OF])(
    ITEM *item, COLL_INFO *coll) = {};

static inline void (*M_GetRoutine(const ITEM *item))(
    ITEM *item, COLL_INFO *coll);

static inline void (*M_GetRoutine(const ITEM *const item))(
    ITEM *item, COLL_INFO *coll)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (TR_VERSION >= 2 && lara->water_status != LWS_SURFACE
        && lara->extra_anim) {
        return m_ExtraRoutines[item->current_anim_state];
    }
    return m_StateRoutines[item->current_anim_state];
}

void Lara_State_Register(
    const LARA_STATE state,
    void (*const handle_func)(ITEM *item, COLL_INFO *coll))
{
    ASSERT(state >= 0 && state < LS_NUMBER_OF);
    m_StateRoutines[state] = handle_func;
}

void Lara_State_RegisterExtra(
    const LARA_EXTRA_STATE state,
    void (*const handle_func)(ITEM *item, COLL_INFO *coll))
{
    ASSERT(state >= 0 && state < LS_EXTRA_NUMBER_OF);
    m_ExtraRoutines[state] = handle_func;
}

void Lara_State_Update(ITEM *const item, COLL_INFO *const coll)
{
    void (*const routine)(ITEM *item, COLL_INFO *coll) = M_GetRoutine(item);
    if (routine != nullptr) {
        routine(item, coll);
    }
}
