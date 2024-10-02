#pragma once

#include "game/ui/controllers/controls.h"

#include <libtrx/game/ui/widgets/base.h>

UI_WIDGET *UI_ControlsColumn_Create(
    int32_t column, UI_CONTROLS_CONTROLLER *controller);
