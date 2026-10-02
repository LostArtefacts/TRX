#include <harness/harness.h>

#include <trx/game/ui/dialogs/takeover.h>

static bool m_Accept = false;
static int32_t m_Offers = 0;
static int32_t m_LastArg = -1;
static int32_t m_Releases = 0;

static bool M_Offer(const UI_TAKEOVER screen, const int32_t arg)
{
    m_Offers++;
    m_LastArg = arg;
    return m_Accept;
}

static void M_Release(const UI_TAKEOVER screen)
{
    m_Releases++;
}

static void M_Reset(const bool accept)
{
    UI_Takeover_Release(UI_TAKEOVER_RING_ENTRY);
    UI_Takeover_SetHooks((UI_TAKEOVER_HOOKS) {
        .offer = M_Offer,
        .release = M_Release,
    });
    m_Accept = accept;
    m_Offers = 0;
    m_LastArg = -1;
    m_Releases = 0;
}

TEST(a_screen_with_no_owner_is_not_held)
{
    UI_Takeover_SetHooks((UI_TAKEOVER_HOOKS) {});
    CHECK(!UI_Takeover_Offer(UI_TAKEOVER_RING_ENTRY, 7));
    CHECK(!UI_Takeover_IsHeld(UI_TAKEOVER_RING_ENTRY));
    CHECK(!UI_Takeover_IsAnyHeld());
}

TEST(an_owner_that_declines_leaves_the_screen_to_the_engine)
{
    M_Reset(false);
    CHECK(!UI_Takeover_Offer(UI_TAKEOVER_RING_ENTRY, 7));
    CHECK_EQ_INT(m_Offers, 1);
    CHECK_EQ_INT(m_LastArg, 7);
    CHECK(!UI_Takeover_IsHeld(UI_TAKEOVER_RING_ENTRY));
}

TEST(an_owner_that_accepts_holds_the_screen)
{
    M_Reset(true);
    CHECK(UI_Takeover_Offer(UI_TAKEOVER_RING_ENTRY, 7));
    CHECK(UI_Takeover_IsHeld(UI_TAKEOVER_RING_ENTRY));
    CHECK(UI_Takeover_IsAnyHeld());
    CHECK_EQ_INT(
        UI_Takeover_TakeChoice(UI_TAKEOVER_RING_ENTRY),
        UI_TAKEOVER_CHOICE_NONE);
    CHECK(UI_Takeover_IsHeld(UI_TAKEOVER_RING_ENTRY));
}

TEST(a_choice_is_taken_once_and_ends_the_hold)
{
    M_Reset(true);
    UI_Takeover_Offer(UI_TAKEOVER_RING_ENTRY, 0);
    UI_Takeover_Close(UI_TAKEOVER_RING_ENTRY, UI_TAKEOVER_CHOICE_CANCEL);
    CHECK_EQ_INT(
        UI_Takeover_TakeChoice(UI_TAKEOVER_RING_ENTRY),
        UI_TAKEOVER_CHOICE_CANCEL);
    CHECK(!UI_Takeover_IsHeld(UI_TAKEOVER_RING_ENTRY));
    CHECK_EQ_INT(
        UI_Takeover_TakeChoice(UI_TAKEOVER_RING_ENTRY),
        UI_TAKEOVER_CHOICE_NONE);
}

TEST(closing_a_screen_nobody_holds_does_nothing)
{
    M_Reset(false);
    UI_Takeover_Close(UI_TAKEOVER_RING_ENTRY, UI_TAKEOVER_CHOICE_CANCEL);
    CHECK_EQ_INT(
        UI_Takeover_TakeChoice(UI_TAKEOVER_RING_ENTRY),
        UI_TAKEOVER_CHOICE_NONE);
}

TEST(a_release_tells_an_owner_that_did_not_close_the_screen)
{
    M_Reset(true);
    UI_Takeover_Offer(UI_TAKEOVER_RING_ENTRY, 0);
    UI_Takeover_Release(UI_TAKEOVER_RING_ENTRY);
    CHECK_EQ_INT(m_Releases, 1);
    CHECK(!UI_Takeover_IsHeld(UI_TAKEOVER_RING_ENTRY));
}

TEST(a_release_after_a_choice_does_not_tell_the_owner)
{
    M_Reset(true);
    UI_Takeover_Offer(UI_TAKEOVER_RING_ENTRY, 0);
    UI_Takeover_Close(UI_TAKEOVER_RING_ENTRY, UI_TAKEOVER_CHOICE_CANCEL);
    UI_Takeover_Release(UI_TAKEOVER_RING_ENTRY);
    CHECK_EQ_INT(m_Releases, 0);
}

TEST(a_release_of_a_screen_nobody_holds_does_nothing)
{
    M_Reset(false);
    UI_Takeover_Release(UI_TAKEOVER_RING_ENTRY);
    CHECK_EQ_INT(m_Releases, 0);
}

TEST(a_ring_entry_takes_cancel_and_confirm)
{
    CHECK(UI_Takeover_AcceptsChoice(
        UI_TAKEOVER_RING_ENTRY, UI_TAKEOVER_CHOICE_CANCEL));
    CHECK(UI_Takeover_AcceptsChoice(
        UI_TAKEOVER_RING_ENTRY, UI_TAKEOVER_CHOICE_CONFIRM));
    CHECK(!UI_Takeover_AcceptsChoice(
        UI_TAKEOVER_RING_ENTRY, UI_TAKEOVER_CHOICE_NONE));
}
