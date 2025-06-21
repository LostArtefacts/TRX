#include "game/ui/dialogs/photo_mode.h"

#include "config.h"
#include "game/game_string.h"
#include "game/ui/elements/frame.h"
#include "game/ui/elements/label.h"
#include "game/ui/elements/modal.h"
#include "game/ui/elements/pad.h"
#include "game/ui/elements/spacer.h"
#include "game/ui/elements/stack.h"

#include <stdio.h>

void UI_PhotoMode(void)
{
    if (!g_Config.ui.enable_photo_mode_ui) {
        return;
    }

    UI_BeginModal(0.0f, 0.0f);
    UI_BeginPad(8.0f, 8.0f);
    UI_BeginFrame(UI_FRAME_DIALOG_BACKGROUND);
    UI_BeginPad(8.0, 6.0);

    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_VERTICAL,
        .spacing = { .v = 8.0f },
    });
    UI_Label(GS(PHOTO_MODE_TITLE));

    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_HORIZONTAL,
        .spacing = { .h = 8.0f },
    });

    // Inputs column
    UI_BeginStack(UI_STACK_VERTICAL);
    UI_Label(
        "\\{input camera_up}\\{input camera_down}"
        "\\{input camera_forward}\\{input camera_back}"
        "\\{input camera_left}\\{input camera_right}");
    UI_Label(
        "\\{input left}\\{input forward}"
        "\\{input back}\\{input right}");
    UI_Label("\\{input step_left}\\{input step_right}");
    UI_Label("\\{input roll}");
    UI_Label("[\\{input slow}+]\\{input draw}");
    UI_Label("\\{input look}");
    UI_Label("[\\{input slow}+]\\{input pause}");
    UI_Label("\\{input toggle_ui}");
    UI_Label("\\{input action}");
    UI_Label("\\{input toggle_photo_mode}/\\{input option}");
    UI_EndStack();

    UI_Spacer(4.0f, 0.0f);

    // Behaviors column
    UI_BeginStack(UI_STACK_VERTICAL);
    UI_Label(GS(PHOTO_MODE_MOVE_PROMPT));
    UI_Label(GS(PHOTO_MODE_ROTATE_PROMPT));
    UI_Label(GS(PHOTO_MODE_ROLL_PROMPT));
    UI_Label(GS(PHOTO_MODE_ROTATE90_PROMPT));
    UI_Label(GS(PHOTO_MODE_FOV_PROMPT));
    UI_Label(GS(PHOTO_MODE_RESET_PROMPT));
    UI_Label(GS(PHOTO_MODE_ADVANCE_FRAME));
    UI_Label(GS(PHOTO_MODE_TOGGLE_HELP));
    UI_Label(GS(PHOTO_MODE_SNAP_PROMPT));
    UI_Label(GS(MISC_EXIT));
    UI_EndStack();

    UI_EndStack();

    UI_EndStack();
    UI_EndPad();
    UI_EndFrame();
    UI_EndPad();
    UI_EndModal();
}
