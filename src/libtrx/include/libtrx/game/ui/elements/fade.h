#pragma once

#include "../../fader.h"
#include "../common.h"

#include <stdint.h>

// Draw a fade on top or behind children.

void UI_BeginFade(FADER *fader, bool is_on_top);
void UI_EndFade(void);
