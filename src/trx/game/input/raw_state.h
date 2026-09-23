#pragma once

// Down and pressed state for a set of keys or buttons, addressed by index and
// knowing nothing of the device the indices come from. Naming a key, naming a
// button and reading an event belong to game/input/raw.h.

#include <stdint.h>

typedef struct {
    bool *down;
    bool *pressed;
    int32_t count;
    // Whether the game holds the inputs rather than the player. While it does,
    // reads report nothing and a press goes unrecorded.
    bool reserved;
} INPUT_RAW_TRACKER;

// Drops the presses of the frame before, leaving what is held alone.
void InputRawState_BeginFrame(INPUT_RAW_TRACKER *tracker);

// Takes an input down or up. Going down reports a press for this frame, unless
// the input was already down or the game holds the inputs. An index the tracker
// does not hold is ignored.
void InputRawState_Set(INPUT_RAW_TRACKER *tracker, int32_t index, bool down);

// Drops everything the tracker holds, presses included.
void InputRawState_Clear(INPUT_RAW_TRACKER *tracker);

// Checks whether the input at the index is down. An index the tracker does not
// hold reports false, and so does every index while the game holds the inputs.
bool InputRawState_IsHeld(const INPUT_RAW_TRACKER *tracker, int32_t index);

// Checks whether the input at the index is physically down, whoever holds the
// inputs. This is what balances the presses and releases a script was told
// about when the game takes the inputs and gives them back.
bool InputRawState_IsHeldRaw(const INPUT_RAW_TRACKER *tracker, int32_t index);

// Checks whether the input at the index went down in this frame. An index the
// tracker does not hold reports false, and so does every index while the game
// holds the inputs.
bool InputRawState_IsPressed(const INPUT_RAW_TRACKER *tracker, int32_t index);

// Position of an axis, from -1 to 1. The negative end reaches one step further
// than the positive one, so dividing by the positive end and clamping keeps
// both ends at 1.
float InputRawState_ScaleAxis(int16_t value);
