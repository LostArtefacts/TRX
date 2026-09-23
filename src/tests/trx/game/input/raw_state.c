// What a key or a button is doing, kept apart from the device that says so. A
// press lasts the frame it arrived in, and reading it must not end it.

#include <harness/harness.h>

#include <trx/game/input/raw_state.h>

#define M_COUNT 4

static bool m_Down[M_COUNT];
static bool m_Pressed[M_COUNT];

static INPUT_RAW_TRACKER M_Tracker(void)
{
    for (int32_t i = 0; i < M_COUNT; i++) {
        m_Down[i] = false;
        m_Pressed[i] = false;
    }
    return (INPUT_RAW_TRACKER) {
        .down = m_Down,
        .pressed = m_Pressed,
        .count = M_COUNT,
    };
}

TEST(input_raw_state_reports_a_press_and_a_hold)
{
    INPUT_RAW_TRACKER tracker = M_Tracker();
    CHECK(!InputRawState_IsHeld(&tracker, 1));
    CHECK(!InputRawState_IsPressed(&tracker, 1));

    InputRawState_Set(&tracker, 1, true);
    CHECK(InputRawState_IsHeld(&tracker, 1));
    CHECK(InputRawState_IsPressed(&tracker, 1));
    CHECK(InputRawState_IsPressed(&tracker, 1));
}

TEST(input_raw_state_drops_a_press_on_the_next_frame)
{
    INPUT_RAW_TRACKER tracker = M_Tracker();
    InputRawState_Set(&tracker, 1, true);

    InputRawState_BeginFrame(&tracker);
    CHECK(InputRawState_IsHeld(&tracker, 1));
    CHECK(!InputRawState_IsPressed(&tracker, 1));
}

TEST(input_raw_state_presses_again_only_after_a_release)
{
    INPUT_RAW_TRACKER tracker = M_Tracker();
    InputRawState_Set(&tracker, 1, true);
    InputRawState_BeginFrame(&tracker);

    InputRawState_Set(&tracker, 1, true);
    CHECK(!InputRawState_IsPressed(&tracker, 1));

    InputRawState_Set(&tracker, 1, false);
    InputRawState_Set(&tracker, 1, true);
    CHECK(InputRawState_IsPressed(&tracker, 1));
}

TEST(input_raw_state_keeps_a_press_that_is_already_over)
{
    // The player can tap a key and let go inside one frame, and the script
    // reading the frame has to hear about it.
    INPUT_RAW_TRACKER tracker = M_Tracker();
    InputRawState_Set(&tracker, 1, true);
    InputRawState_Set(&tracker, 1, false);

    CHECK(!InputRawState_IsHeld(&tracker, 1));
    CHECK(InputRawState_IsPressed(&tracker, 1));
}

TEST(input_raw_state_holds_one_input_at_a_time)
{
    INPUT_RAW_TRACKER tracker = M_Tracker();
    InputRawState_Set(&tracker, 2, true);

    CHECK(InputRawState_IsHeld(&tracker, 2));
    CHECK(!InputRawState_IsHeld(&tracker, 0));
    CHECK(!InputRawState_IsHeld(&tracker, 3));
}

TEST(input_raw_state_ignores_an_index_it_does_not_hold)
{
    INPUT_RAW_TRACKER tracker = M_Tracker();
    InputRawState_Set(&tracker, -1, true);
    InputRawState_Set(&tracker, M_COUNT, true);

    CHECK(!InputRawState_IsHeld(&tracker, -1));
    CHECK(!InputRawState_IsHeld(&tracker, M_COUNT));
    CHECK(!InputRawState_IsPressed(&tracker, -1));
    CHECK(!InputRawState_IsPressed(&tracker, M_COUNT));
}

TEST(input_raw_state_clears_everything_it_holds)
{
    // A controller that goes away reports nothing more, so what it held has to
    // go with it rather than read that way for good.
    INPUT_RAW_TRACKER tracker = M_Tracker();
    InputRawState_Set(&tracker, 0, true);
    InputRawState_Set(&tracker, 3, true);

    InputRawState_Clear(&tracker);
    for (int32_t i = 0; i < M_COUNT; i++) {
        CHECK(!InputRawState_IsHeld(&tracker, i));
        CHECK(!InputRawState_IsPressed(&tracker, i));
    }
}

TEST(input_raw_state_scales_an_axis_to_both_ends)
{
    CHECK(InputRawState_ScaleAxis(0) == 0.0f);
    CHECK(InputRawState_ScaleAxis(32767) == 1.0f);
    // The negative end reaches one step further than the positive one, and
    // both ends still have to read as 1.
    CHECK(InputRawState_ScaleAxis(-32768) == -1.0f);
    CHECK(InputRawState_ScaleAxis(-32767) == -1.0f);
}

TEST(input_raw_state_scales_an_axis_in_between)
{
    const float half = InputRawState_ScaleAxis(16384);
    CHECK(half > 0.49f && half < 0.51f);
}

TEST(input_raw_state_reports_nothing_while_the_game_holds_the_inputs)
{
    INPUT_RAW_TRACKER tracker = M_Tracker();
    InputRawState_Set(&tracker, 1, true);

    tracker.reserved = true;
    CHECK(!InputRawState_IsHeld(&tracker, 1));
    CHECK(!InputRawState_IsPressed(&tracker, 1));
    CHECK(InputRawState_IsHeldRaw(&tracker, 1));
}

TEST(input_raw_state_drops_a_press_the_game_took)
{
    // The console closes, and the key the player typed into it must not arrive
    // as a press the moment a script can read again.
    INPUT_RAW_TRACKER tracker = M_Tracker();
    tracker.reserved = true;
    InputRawState_Set(&tracker, 1, true);

    tracker.reserved = false;
    CHECK(!InputRawState_IsPressed(&tracker, 1));
    CHECK(InputRawState_IsHeld(&tracker, 1));
}

TEST(input_raw_state_keeps_a_release_the_game_took)
{
    INPUT_RAW_TRACKER tracker = M_Tracker();
    InputRawState_Set(&tracker, 1, true);

    tracker.reserved = true;
    InputRawState_Set(&tracker, 1, false);
    tracker.reserved = false;
    CHECK(!InputRawState_IsHeld(&tracker, 1));
}
