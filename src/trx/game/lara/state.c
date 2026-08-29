#include <trx/game/lara/state.h>

#include <trx/debug.h>
#include <trx/game/catalog/table.h>
#include <trx/game/lara.h>

typedef void (*M_STATE_ROUTINE)(ITEM *item, COLL_INFO *coll);

static const LARA_ANIMATION_ID m_TestResponsiveAnims[] = {
    // clang-format off
    LA_RUN,
    LA_UNDERWATER_SWIM_FORWARD,
    LA_SLIDE_FORWARD,
    LA_STAND_TO_JUMP,
    LA_REACH_TO_HANG,
    NO_CATALOG_ID,
    // clang-format on
};

CATALOG_TABLE_DEFINE(m_ResponsiveAnims, CATALOG_LARA_ANIMS, bool);
CATALOG_TABLE_DEFINE(m_StateRoutines, CATALOG_LARA_STATES, M_STATE_ROUTINE);
static void (*m_ExtraRoutines[LS_EXTRA_NUMBER_OF])(
    ITEM *item, COLL_INFO *coll) = {};

static bool M_HasResponsiveState(const LARA_ANIMATION_ID anim_idx)
{
    const OBJECT *const obj = Object_Get(O_LARA);
    if (!obj->loaded) {
        return false;
    }

    const ANIM *const anim = Object_GetAnim(obj, LA(anim_idx));
    for (int32_t i = 0; i < anim->num_changes; i++) {
        const ANIM_CHANGE *const change = Anim_GetChange(anim->change_idx + i);
        if (change->goal_anim_state == LS(LS_RESPONSIVE)) {
            return true;
        }
    }

    return false;
}

void Lara_State_Register(
    const LARA_STATE_ID state,
    void (*const handle_func)(ITEM *item, COLL_INFO *coll))
{
    *(M_STATE_ROUTINE *)CatalogTable_Get(&m_StateRoutines, state) = handle_func;
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
    for (int32_t i = 0; m_TestResponsiveAnims[i] != NO_CATALOG_ID; i++) {
        const LARA_ANIMATION_ID anim = m_TestResponsiveAnims[i];
        *(bool *)CatalogTable_Get(&m_ResponsiveAnims, anim) =
            M_HasResponsiveState(anim);
    }
}

bool Lara_State_IsResponsive(const LARA_ANIMATION_ID anim_idx)
{
    const bool *const responsive =
        CatalogTable_Get(&m_ResponsiveAnims, anim_idx);
    return responsive != nullptr && *responsive;
}

void Lara_State_Update(ITEM *const item, COLL_INFO *const coll)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->water_status != LWS_SURFACE && lara->extra_anim) {
        if (m_ExtraRoutines[item->current_anim_state] != nullptr) {
            m_ExtraRoutines[item->current_anim_state](item, coll);
        }
        return;
    }

    const LARA_STATE_ID state = LS_U(item->current_anim_state);
    const M_STATE_ROUTINE *const routine =
        CatalogTable_Get(&m_StateRoutines, state);
    if (routine != nullptr && *routine != nullptr) {
        (*routine)(item, coll);
    }
}
