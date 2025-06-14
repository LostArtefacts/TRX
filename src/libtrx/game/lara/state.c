#include "game/lara/state.h"

#include "debug.h"
#include "game/lara.h"

static bool m_ResponsiveAnims[LA_NUMBER_OF] = {};
static void (*m_StateRoutines[LS_NUMBER_OF])(ITEM *item, COLL_INFO *coll) = {};
static void (*m_ExtraRoutines[LS_EXTRA_NUMBER_OF])(
    ITEM *item, COLL_INFO *coll) = {};

static bool M_HasResponsiveState(LARA_ANIMATION anim_idx);
static inline void (*M_GetRoutine(const ITEM *item))(
    ITEM *item, COLL_INFO *coll);

static bool M_HasResponsiveState(const LARA_ANIMATION anim_idx)
{
#if TR_VERSION == 1
    const OBJECT *const obj = Object_Get(O_LARA);
    if (!obj->loaded) {
        return false;
    }

    const ANIM *const anim = Object_GetAnim(obj, anim_idx);
    for (int32_t i = 0; i < anim->num_changes; i++) {
        const ANIM_CHANGE *const change = Anim_GetChange(anim->change_idx + i);
        if (change->goal_anim_state == LS_RESPONSIVE) {
            return true;
        }
    }

    return false;
#else
    return true;
#endif
}

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

void Lara_State_Initialise(void)
{
    m_ResponsiveAnims[LA_RUN] = M_HasResponsiveState(LA_RUN);
    m_ResponsiveAnims[LA_UNDERWATER_SWIM_FORWARD] =
        M_HasResponsiveState(LA_UNDERWATER_SWIM_FORWARD);
}

bool Lara_State_IsResponsive(const LARA_ANIMATION anim_idx)
{
    return m_ResponsiveAnims[anim_idx];
}

void Lara_State_Update(ITEM *const item, COLL_INFO *const coll)
{
    void (*const routine)(ITEM *item, COLL_INFO *coll) = M_GetRoutine(item);
    if (routine != nullptr) {
        routine(item, coll);
    }
}
