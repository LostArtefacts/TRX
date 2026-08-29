// Exercises the ItemAction interceptor seam: a claimed number is handed to the
// interceptor and its routine skipped. The id translation and item lookup are
// faked to the identity so only the seam is under test.

#include <harness/harness.h>

#include <trx/game/items/actions.h>

static ITEM m_Item;
static int16_t m_FakeItemIndex = 12;

static int32_t m_SeenEffect;
static int32_t m_SeenTimer;
static int16_t m_SeenItem;
static bool m_Intercepted;
static int32_t m_ClaimedEffect;

static bool m_RoutineRan;

static bool M_Interceptor(
    const int32_t effect_num, const int32_t timer, const int16_t item_num)
{
    if (effect_num != m_ClaimedEffect) {
        return false;
    }
    m_SeenEffect = effect_num;
    m_SeenTimer = timer;
    m_SeenItem = item_num;
    return true;
}

static void M_Routine(ITEM *const item)
{
    m_RoutineRan = true;
}

static void M_Reset(const int32_t claimed_effect)
{
    m_SeenEffect = -1;
    m_SeenTimer = -1;
    m_SeenItem = -1;
    m_RoutineRan = false;
    m_ClaimedEffect = claimed_effect;
    ItemAction_SetInterceptor(M_Interceptor);
    ItemAction_Register(ITEM_ACTION_LARA_NORMAL, M_Routine);
}

int16_t Item_GetIndex(const ITEM *const item)
{
    return m_FakeItemIndex;
}

ITEM_ACTION_ID ItemAction_SlotToID(const ITEM_ACTION_SLOT action)
{
    return (ITEM_ACTION_ID)action;
}

int32_t Room_GetFlipEffect(void)
{
    return -1;
}

TEST(run_direct_hands_a_claimed_number_to_the_interceptor)
{
    M_Reset((int32_t)ITEM_ACTION_LARA_NORMAL);

    ItemAction_RunDirect((ITEM_ACTION_SLOT)ITEM_ACTION_LARA_NORMAL, &m_Item);

    CHECK_EQ_INT(m_SeenEffect, (int32_t)ITEM_ACTION_LARA_NORMAL);
    CHECK_EQ_INT(m_SeenTimer, 0);
    CHECK_EQ_INT(m_SeenItem, m_FakeItemIndex);
    CHECK(!m_RoutineRan);
}

TEST(run_direct_with_fx_hands_a_claimed_number_to_the_interceptor)
{
    M_Reset((int32_t)ITEM_ACTION_LARA_NORMAL);

    ItemAction_RunDirectWithFX(
        (ITEM_ACTION_SLOT)ITEM_ACTION_LARA_NORMAL, &m_Item, 3);

    CHECK_EQ_INT(m_SeenEffect, (int32_t)ITEM_ACTION_LARA_NORMAL);
    CHECK_EQ_INT(m_SeenItem, m_FakeItemIndex);
    CHECK(!m_RoutineRan);
}

TEST(an_unclaimed_number_runs_its_stock_routine)
{
    // Claim a different number, so this one falls through to the routine.
    M_Reset((int32_t)ITEM_ACTION_TURN_180);

    ItemAction_RunDirect((ITEM_ACTION_SLOT)ITEM_ACTION_LARA_NORMAL, &m_Item);

    CHECK_EQ_INT(m_SeenEffect, -1);
    CHECK(m_RoutineRan);
}
