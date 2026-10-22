#include <trx/game/ui/tween.h>

#include <trx/core/utils.h>
#include <trx/game/interpolation.h>
#include <trx/game/lua/events.h>

// The tick a frame is drawn at, part way between the last two ticks.
static double M_Now(void)
{
    return LUA_GetTick() - 1 + Interpolation_GetUIRate();
}

// The tick of the last frame drawn before the current tick. A move set during
// a tick starts from there, so the frames of that tick already show it moving
// on from what the screen last showed.
static int32_t M_LastDrawn(void)
{
    return LUA_GetTick() - 1;
}

static float M_GetAt(const UI_TWEEN *const tween, const double now)
{
    const double elapsed = now - tween->start;
    if (elapsed <= 0.0) {
        return tween->from;
    }
    if (tween->ticks <= 0 || elapsed >= tween->ticks) {
        return tween->to;
    }
    const float ratio =
        UI_Ease_Apply(tween->ease, (float)(elapsed / tween->ticks));
    return tween->from + (tween->to - tween->from) * ratio;
}

float UI_Ease_Apply(const UI_EASE ease, const float ratio)
{
    switch (ease) {
    case UI_EASE_LINEAR:
        return ratio;
    case UI_EASE_QUAD_IN:
        return ratio * ratio;
    case UI_EASE_QUAD_OUT: {
        const float rest = 1.0f - ratio;
        return 1.0f - rest * rest;
    }
    case UI_EASE_QUAD_IN_OUT: {
        if (ratio < 0.5f) {
            return 2.0f * ratio * ratio;
        }
        const float rest = ratio - 1.0f;
        return 1.0f - 2.0f * rest * rest;
    }
    }
    return ratio;
}

void UI_Tween_Set(UI_TWEEN *const tween, const float value)
{
    tween->from = value;
    tween->to = value;
    tween->ticks = 0;
}

void UI_Tween_To(
    UI_TWEEN *const tween, const float target, const int32_t ticks,
    const int32_t delay, const UI_EASE ease)
{
    tween->from =
        ticks <= 0 && delay <= 0 ? target : M_GetAt(tween, M_LastDrawn());
    tween->to = target;
    tween->start = M_LastDrawn() + MAX(delay, 0);
    tween->ticks = ticks;
    tween->ease = ease;
}

float UI_Tween_Get(const UI_TWEEN *const tween)
{
    return M_GetAt(tween, M_Now());
}

float UI_Tween_GetTarget(const UI_TWEEN *const tween)
{
    return tween->to;
}

bool UI_Tween_IsMoving(const UI_TWEEN *const tween)
{
    return LUA_GetTick() - tween->start < tween->ticks;
}
