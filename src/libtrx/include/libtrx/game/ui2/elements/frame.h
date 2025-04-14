#pragma once

#include "../common.h"

// A frame around the child widget.

typedef enum {
    UI2_FRAME_DIALOG_BACKGROUND,
    UI2_FRAME_DIALOG_HEADING,
    UI2_FRAME_SELECTED_OPTION,
    UI2_FRAME_OUTLINE_ONLY,
} UI2_FRAME_STYLE;

void UI2_BeginFrame(UI2_FRAME_STYLE style);
void UI2_EndFrame(void);
