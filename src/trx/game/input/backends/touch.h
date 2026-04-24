#pragma once

#include <trx/game/input/backends/base.h>

extern INPUT_BACKEND_IMPL g_Input_Touch;

// Touch positions are abstract indices identifying each assignable location on
// the touch overlay. A single finger touching a specific location activates
// the corresponding position. The overlay owns the mapping between a position
// and its on-screen hit area, while the backend only deals with the indices.
//
// Indices are allocated as follows:
//   0..3: virtual D-pad directions (UP, DOWN, LEFT, RIGHT).
//   4+:   action/system buttons, one per button def exposed by the overlay.
//
// Bindings map an INPUT_ROLE to one or more positions (for multi-touch
// combos), which the resolver converts back to INPUT_STATE flags each frame.

// Set the press state for a touch position (called by overlay).
void Touch_SetPositionState(int32_t position, bool pressed);

// Get the role currently bound to a touch position in the active layout.
INPUT_ROLE Touch_GetPositionRole(int32_t position);

// Whether the running platform exposes any touch input devices.
bool Touch_HasHardwareSupport(void);
