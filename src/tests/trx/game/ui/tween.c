#include <harness/harness.h>

#include <trx/game/interpolation.h>
#include <trx/game/lua/events.h>
#include <trx/game/ui/tween.h>

#include <math.h>

static int32_t m_Tick = 0;
static double m_Rate = 1.0;

static bool M_Near(const float actual, const float expected)
{
    return fabsf(actual - expected) < 0.0001f;
}

static UI_TWEEN M_Resting(const float value)
{
    m_Tick = 0;
    m_Rate = 1.0;
    UI_TWEEN tween = {};
    UI_Tween_Set(&tween, value);
    return tween;
}

int32_t LUA_GetTick(void)
{
    return m_Tick;
}

double Interpolation_GetUIRate(void)
{
    return m_Rate;
}

TEST(curves_meet_at_both_ends)
{
    for (UI_EASE ease = UI_EASE_LINEAR; ease <= UI_EASE_QUAD_IN_OUT; ease++) {
        CHECK(M_Near(UI_Ease_Apply(ease, 0.0f), 0.0f));
        CHECK(M_Near(UI_Ease_Apply(ease, 1.0f), 1.0f));
    }
    CHECK(UI_Ease_Apply(UI_EASE_QUAD_IN, 0.5f) < 0.5f);
    CHECK(UI_Ease_Apply(UI_EASE_QUAD_OUT, 0.5f) > 0.5f);
    CHECK(M_Near(UI_Ease_Apply(UI_EASE_QUAD_IN_OUT, 0.5f), 0.5f));
}

TEST(a_resting_tween_holds_its_value)
{
    UI_TWEEN tween = M_Resting(3.0f);
    m_Tick = 50;
    CHECK(M_Near(UI_Tween_Get(&tween), 3.0f));
    CHECK(!UI_Tween_IsMoving(&tween));
}

TEST(a_move_ends_on_its_last_tick)
{
    UI_TWEEN tween = M_Resting(0.0f);
    UI_Tween_To(&tween, 1.0f, 4, 0, UI_EASE_LINEAR);
    CHECK(M_Near(UI_Tween_GetTarget(&tween), 1.0f));

    CHECK(M_Near(UI_Tween_Get(&tween), 0.25f));

    m_Tick = 1;
    CHECK(M_Near(UI_Tween_Get(&tween), 0.5f));
    CHECK(UI_Tween_IsMoving(&tween));

    m_Tick = 3;
    CHECK(M_Near(UI_Tween_Get(&tween), 1.0f));
    CHECK(!UI_Tween_IsMoving(&tween));
}

TEST(a_frame_between_ticks_reads_between_values)
{
    UI_TWEEN tween = M_Resting(0.0f);
    UI_Tween_To(&tween, 1.0f, 4, 0, UI_EASE_LINEAR);
    m_Tick = 1;
    m_Rate = 0.5;
    CHECK(M_Near(UI_Tween_Get(&tween), 0.375f));
}

TEST(a_new_target_starts_from_the_last_frame_drawn)
{
    UI_TWEEN tween = M_Resting(0.0f);
    UI_Tween_To(&tween, 1.0f, 4, 0, UI_EASE_LINEAR);
    m_Tick = 2;
    UI_Tween_To(&tween, 0.0f, 2, 0, UI_EASE_LINEAR);
    m_Rate = 0.0;
    CHECK(M_Near(UI_Tween_Get(&tween), 0.5f));
    m_Rate = 0.5;
    CHECK(M_Near(UI_Tween_Get(&tween), 0.375f));
    m_Rate = 1.0;
    CHECK(M_Near(UI_Tween_Get(&tween), 0.25f));
    m_Tick = 3;
    CHECK(M_Near(UI_Tween_Get(&tween), 0.0f));
}

TEST(set_stops_a_move)
{
    UI_TWEEN tween = M_Resting(0.0f);
    UI_Tween_To(&tween, 1.0f, 4, 0, UI_EASE_LINEAR);
    m_Tick = 1;
    UI_Tween_Set(&tween, 0.75f);
    CHECK(!UI_Tween_IsMoving(&tween));
    m_Tick = 2;
    CHECK(M_Near(UI_Tween_Get(&tween), 0.75f));
}

TEST(a_delayed_move_holds_still_until_it_starts)
{
    UI_TWEEN tween = M_Resting(0.0f);
    UI_Tween_To(&tween, 1.0f, 4, 3, UI_EASE_LINEAR);
    m_Tick = 2;
    CHECK(M_Near(UI_Tween_Get(&tween), 0.0f));
    CHECK(UI_Tween_IsMoving(&tween));
    m_Tick = 4;
    CHECK(M_Near(UI_Tween_Get(&tween), 0.5f));
    m_Tick = 6;
    CHECK(M_Near(UI_Tween_Get(&tween), 1.0f));
    CHECK(!UI_Tween_IsMoving(&tween));
}

TEST(a_jump_lands_at_once)
{
    UI_TWEEN tween = M_Resting(0.0f);
    UI_Tween_To(&tween, 1.0f, 0, 0, UI_EASE_LINEAR);
    m_Rate = 0.0;
    CHECK(M_Near(UI_Tween_Get(&tween), 1.0f));
    CHECK(!UI_Tween_IsMoving(&tween));
}

TEST(a_delayed_jump_waits_for_its_tick)
{
    UI_TWEEN tween = M_Resting(0.0f);
    UI_Tween_To(&tween, 1.0f, 0, 2, UI_EASE_LINEAR);
    m_Tick = 1;
    CHECK(M_Near(UI_Tween_Get(&tween), 0.0f));
    m_Tick = 2;
    CHECK(M_Near(UI_Tween_Get(&tween), 1.0f));
}
