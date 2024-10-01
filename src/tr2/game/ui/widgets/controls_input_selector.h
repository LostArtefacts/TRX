#pragma once

#include "game/ui/controllers/controls.h"

#include <libtrx/game/ui/widgets/base.h>

UI_WIDGET *UI_ControlsInputSelector_Create(
    INPUT_ROLE input_role, UI_CONTROLS_CONTROLLER *controller);
