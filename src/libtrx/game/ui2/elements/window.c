#include "game/ui2/elements/window.h"

#include "game/ui2/elements/anchor.h"
#include "game/ui2/elements/frame.h"
#include "game/ui2/elements/label.h"
#include "game/ui2/elements/modal.h"
#include "game/ui2/elements/pad.h"
#include "game/ui2/elements/stack.h"

void UI2_BeginWindow(void)
{
    const float outer_pad = 2.0f;
    const float title_spacing = 3.0f;
    UI2_BeginFrame(UI2_FRAME_DIALOG_BACKGROUND);
    UI2_BeginPad(outer_pad, outer_pad);
    UI2_BeginStackEx((UI2_STACK_SETTINGS) {
        .orientation = UI2_STACK_VERTICAL,
        .align = { .h = UI2_STACK_H_ALIGN_SPAN },
        .spacing = { .v = title_spacing },
    });
}

void UI2_EndWindow(void)
{
    UI2_EndStack();
    UI2_EndPad();
    UI2_EndFrame();
}

void UI2_BeginWindowBody(void)
{
    const float body_pad = 8.0f;
    UI2_BeginPad(body_pad, body_pad);
}

void UI2_EndWindowBody(void)
{
    UI2_EndPad();
}

void UI2_WindowTitle(const char *const title)
{
    UI2_BeginFrame(UI2_FRAME_DIALOG_HEADING);
    UI2_BeginPad(10.0f, 2.0f);
    UI2_BeginAnchor(0.5f, 0.5f);
    UI2_Label(title);
    UI2_EndAnchor();
    UI2_EndPad();
    UI2_EndFrame();
}
