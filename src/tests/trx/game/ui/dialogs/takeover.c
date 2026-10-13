#include <harness/harness.h>

#include <trx/game/ui/dialogs/takeover.h>

static bool m_Accept = false;
static int32_t m_Offers = 0;
static OBJECT_ID m_LastObject = -1;
static int32_t m_Releases = 0;

static bool M_Offer(
    const UI_TAKEOVER screen, const UI_TAKEOVER_ARGS *const args)
{
    m_Offers++;
    m_LastObject = args->object_id;
    return m_Accept;
}

static void M_Release(const UI_TAKEOVER screen)
{
    m_Releases++;
}

static void M_Reset(const bool accept)
{
    UI_Takeover_Release(UI_TAKEOVER_RING_ENTRY);
    UI_Takeover_Release(UI_TAKEOVER_PAUSE);
    UI_Takeover_Release(UI_TAKEOVER_SAVE_LOAD);
    UI_Takeover_Release(UI_TAKEOVER_STATS);
    UI_Takeover_Release(UI_TAKEOVER_EXAMINE);
    UI_Takeover_SetHooks((UI_TAKEOVER_HOOKS) {
        .offer = M_Offer,
        .release = M_Release,
    });
    m_Accept = accept;
    m_Offers = 0;
    m_LastObject = -1;
    m_Releases = 0;
}

TEST(a_screen_with_no_owner_is_not_held)
{
    UI_Takeover_SetHooks((UI_TAKEOVER_HOOKS) {});
    CHECK(!UI_Takeover_Offer(
        UI_TAKEOVER_RING_ENTRY, (UI_TAKEOVER_ARGS) { .object_id = 7 }));
    CHECK(!UI_Takeover_IsHeld(UI_TAKEOVER_RING_ENTRY));
    CHECK(!UI_Takeover_IsAnyHeld());
}

TEST(an_owner_that_declines_leaves_the_screen_to_the_engine)
{
    M_Reset(false);
    CHECK(!UI_Takeover_Offer(
        UI_TAKEOVER_RING_ENTRY, (UI_TAKEOVER_ARGS) { .object_id = 7 }));
    CHECK_EQ_INT(m_Offers, 1);
    CHECK_EQ_INT(m_LastObject, 7);
    CHECK(!UI_Takeover_IsHeld(UI_TAKEOVER_RING_ENTRY));
}

TEST(an_owner_that_accepts_holds_the_screen)
{
    M_Reset(true);
    CHECK(UI_Takeover_Offer(
        UI_TAKEOVER_RING_ENTRY, (UI_TAKEOVER_ARGS) { .object_id = 7 }));
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
    UI_Takeover_Offer(UI_TAKEOVER_RING_ENTRY, (UI_TAKEOVER_ARGS) {});
    UI_Takeover_Close(UI_TAKEOVER_RING_ENTRY, UI_TAKEOVER_CHOICE_CANCEL);
    CHECK_EQ_INT(
        UI_Takeover_TakeChoice(UI_TAKEOVER_RING_ENTRY),
        UI_TAKEOVER_CHOICE_CANCEL);
    CHECK(!UI_Takeover_IsHeld(UI_TAKEOVER_RING_ENTRY));
    CHECK_EQ_INT(
        UI_Takeover_TakeChoice(UI_TAKEOVER_RING_ENTRY),
        UI_TAKEOVER_CHOICE_NONE);
}

TEST(a_closed_screen_counts_until_its_choice_is_taken)
{
    M_Reset(true);
    UI_Takeover_Offer(UI_TAKEOVER_RING_ENTRY, (UI_TAKEOVER_ARGS) {});
    CHECK(!UI_Takeover_IsAnyClosed());
    UI_Takeover_Close(UI_TAKEOVER_RING_ENTRY, UI_TAKEOVER_CHOICE_CANCEL);
    CHECK(UI_Takeover_IsAnyClosed());
    UI_Takeover_TakeChoice(UI_TAKEOVER_RING_ENTRY);
    CHECK(!UI_Takeover_IsAnyClosed());
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
    UI_Takeover_Offer(UI_TAKEOVER_RING_ENTRY, (UI_TAKEOVER_ARGS) {});
    UI_Takeover_Release(UI_TAKEOVER_RING_ENTRY);
    CHECK_EQ_INT(m_Releases, 1);
    CHECK(!UI_Takeover_IsHeld(UI_TAKEOVER_RING_ENTRY));
}

TEST(a_release_after_a_choice_does_not_tell_the_owner)
{
    M_Reset(true);
    UI_Takeover_Offer(UI_TAKEOVER_RING_ENTRY, (UI_TAKEOVER_ARGS) {});
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

TEST(the_pause_question_takes_its_own_choices)
{
    CHECK(UI_Takeover_AcceptsChoice(
        UI_TAKEOVER_PAUSE, UI_TAKEOVER_CHOICE_CANCEL));
    CHECK(UI_Takeover_AcceptsChoice(
        UI_TAKEOVER_PAUSE, UI_TAKEOVER_CHOICE_RESUME));
    CHECK(UI_Takeover_AcceptsChoice(
        UI_TAKEOVER_PAUSE, UI_TAKEOVER_CHOICE_EXIT_TO_TITLE));
    CHECK(!UI_Takeover_AcceptsChoice(
        UI_TAKEOVER_PAUSE, UI_TAKEOVER_CHOICE_CONFIRM));
    CHECK(!UI_Takeover_AcceptsChoice(
        UI_TAKEOVER_RING_ENTRY, UI_TAKEOVER_CHOICE_RESUME));
}

TEST(the_quick_save_and_load_screen_takes_only_cancel)
{
    CHECK(UI_Takeover_AcceptsChoice(
        UI_TAKEOVER_SAVE_LOAD, UI_TAKEOVER_CHOICE_CANCEL));
    CHECK(!UI_Takeover_AcceptsChoice(
        UI_TAKEOVER_SAVE_LOAD, UI_TAKEOVER_CHOICE_NONE));
}

TEST(the_stats_screen_takes_cancel_and_confirm)
{
    CHECK(UI_Takeover_AcceptsChoice(
        UI_TAKEOVER_STATS, UI_TAKEOVER_CHOICE_CANCEL));
    CHECK(UI_Takeover_AcceptsChoice(
        UI_TAKEOVER_STATS, UI_TAKEOVER_CHOICE_CONFIRM));
    CHECK(!UI_Takeover_AcceptsChoice(
        UI_TAKEOVER_STATS, UI_TAKEOVER_CHOICE_RESUME));
}

TEST(an_examined_entry_takes_only_cancel)
{
    CHECK(UI_Takeover_AcceptsChoice(
        UI_TAKEOVER_EXAMINE, UI_TAKEOVER_CHOICE_CANCEL));
    CHECK(!UI_Takeover_AcceptsChoice(
        UI_TAKEOVER_EXAMINE, UI_TAKEOVER_CHOICE_CONFIRM));
}

TEST(each_screen_is_held_on_its_own)
{
    M_Reset(true);
    UI_Takeover_Offer(UI_TAKEOVER_SAVE_LOAD, (UI_TAKEOVER_ARGS) {});
    CHECK(UI_Takeover_IsHeld(UI_TAKEOVER_SAVE_LOAD));
    CHECK(!UI_Takeover_IsHeld(UI_TAKEOVER_RING_ENTRY));
    CHECK(UI_Takeover_IsAnyHeld());
}
