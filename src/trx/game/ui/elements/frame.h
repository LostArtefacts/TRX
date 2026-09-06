#pragma once

#include <trx/game/output/types.h>
#include <trx/game/ui/common.h>

// A frame around the child widget.

typedef enum {
    UI_FRAME_DIALOG_BACKGROUND,
    UI_FRAME_DIALOG_BACKGROUND_HEAVY,
    UI_FRAME_DIALOG_HEADING,
    UI_FRAME_SELECTED_OPTION,
    UI_FRAME_OUTLINE_ONLY,
} UI_FRAME_STYLE;

// The text style a frame style draws its background and its outline in.
TEXT_STYLE UI_Frame_GetTextStyle(UI_FRAME_STYLE style);

// Whether a frame style fills the box behind its child, rather than drawing
// only the outline.
bool UI_Frame_HasBackground(UI_FRAME_STYLE style);

void UI_BeginFrame(UI_FRAME_STYLE style);
void UI_EndFrame(void);
