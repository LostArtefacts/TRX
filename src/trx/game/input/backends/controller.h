#pragma once

#include <trx/game/input/backends/base.h>

extern INPUT_BACKEND_IMPL g_Input_Controller;

// Uses recorded button and axis input when no controller is attached.
void Input_Controller_SetAssumeAttached(bool enabled);
