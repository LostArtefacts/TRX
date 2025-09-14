#include "game/ui/elements/window.h"

#include "game/ui/elements/anchor.h"
#include "game/ui/elements/frame.h"
#include "game/ui/elements/label.h"
#include "game/ui/elements/modal.h"
#include "game/ui/elements/pad.h"
#include "game/ui/elements/stack.h"

void UI_BeginWindow(void)
{
    const float outer_pad = TR_VERSION == 2 ? 3.0f : 2.0f;
    const float title_spacing = 3.0f;
    UI_BeginFrame(UI_FRAME_DIALOG_BACKGROUND);
    UI_BeginPad(outer_pad, outer_pad);
    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_VERTICAL,
        .align = { .h = UI_STACK_H_ALIGN_SPAN },
        .spacing = { .v = title_spacing },
    });
}

void UI_EndWindow(void)
{
    UI_EndStack();
    UI_EndPad();
    UI_EndFrame();
}

void UI_BeginWindowBody(void)
{
    const float body_pad = TR_VERSION == 2 ? 4.0f : 8.0f;
    UI_BeginPad(body_pad, body_pad);
}

void UI_EndWindowBody(void)
{
    UI_EndPad();
}

void UI_WindowTitle(const char *const title)
{
    UI_BeginFrame(UI_FRAME_DIALOG_HEADING);
    UI_BeginPad(10.0f, TR_VERSION == 2 ? 1.0f : 2.0f);
    UI_BeginAnchor(0.5f, 0.5f);
    UI_Label(title);
    UI_EndAnchor();
    UI_EndPad();
    UI_EndFrame();
}
