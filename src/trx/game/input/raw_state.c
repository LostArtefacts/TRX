#include <trx/game/input/raw_state.h>

#include <trx/core/utils.h>

#include <string.h>

// Largest reading the positive end of an axis gives.
#define M_AXIS_MAX 32767.0f

static bool M_IsInRange(const INPUT_RAW_TRACKER *const tracker, int32_t index)
{
    return index >= 0 && index < tracker->count;
}

void InputRawState_BeginFrame(INPUT_RAW_TRACKER *const tracker)
{
    memset(tracker->pressed, 0, tracker->count * sizeof tracker->pressed[0]);
}

void InputRawState_Set(
    INPUT_RAW_TRACKER *const tracker, const int32_t index, const bool down)
{
    if (!M_IsInRange(tracker, index)) {
        return;
    }
    if (down) {
        tracker->pressed[index] = !tracker->down[index];
    }
    tracker->down[index] = down;
}

void InputRawState_Clear(INPUT_RAW_TRACKER *const tracker)
{
    memset(tracker->down, 0, tracker->count * sizeof tracker->down[0]);
    memset(tracker->pressed, 0, tracker->count * sizeof tracker->pressed[0]);
}

bool InputRawState_IsHeld(
    const INPUT_RAW_TRACKER *const tracker, const int32_t index)
{
    return M_IsInRange(tracker, index) && tracker->down[index];
}

bool InputRawState_IsPressed(
    const INPUT_RAW_TRACKER *const tracker, const int32_t index)
{
    return M_IsInRange(tracker, index) && tracker->pressed[index];
}

float InputRawState_ScaleAxis(const int16_t value)
{
    float result = value / M_AXIS_MAX;
    CLAMP(result, -1.0f, 1.0f);
    return result;
}
