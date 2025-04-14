#include "game/ui2/dialogs/photo_mode.h"

#include "config.h"
#include "game/game_string.h"
#include "game/input.h"
#include "game/ui2/elements/frame.h"
#include "game/ui2/elements/label.h"
#include "game/ui2/elements/modal.h"
#include "game/ui2/elements/pad.h"
#include "game/ui2/elements/spacer.h"
#include "game/ui2/elements/stack.h"

#include <stdio.h>

void UI2_PhotoMode(void)
{
    if (!g_Config.ui.enable_photo_mode_ui) {
        return;
    }

    char tmp[50];

    UI2_BeginModal(0.0f, 0.0f);
    UI2_BeginPad(8.0f, 10.0f);
    UI2_BeginFrame(UI2_FRAME_DIALOG_BACKGROUND);
    UI2_BeginPad(8.0, 6.0);

    UI2_BeginStackEx((UI2_STACK_SETTINGS) {
        .orientation = UI2_STACK_VERTICAL,
        .spacing = { .v = 8.0f },
    });
    UI2_Label(GS(PHOTO_MODE_TITLE));

    UI2_BeginStack(UI2_STACK_HORIZONTAL);

    // Inputs column
    UI2_BeginStack(UI2_STACK_VERTICAL);
    sprintf(
        tmp, "%s%s%s%s%s%s: ",
        Input_GetKeyName(
            INPUT_BACKEND_KEYBOARD, g_Config.input.keyboard_layout,
            INPUT_ROLE_CAMERA_UP),
        Input_GetKeyName(
            INPUT_BACKEND_KEYBOARD, g_Config.input.keyboard_layout,
            INPUT_ROLE_CAMERA_DOWN),
        Input_GetKeyName(
            INPUT_BACKEND_KEYBOARD, g_Config.input.keyboard_layout,
            INPUT_ROLE_CAMERA_FORWARD),
        Input_GetKeyName(
            INPUT_BACKEND_KEYBOARD, g_Config.input.keyboard_layout,
            INPUT_ROLE_CAMERA_BACK),
        Input_GetKeyName(
            INPUT_BACKEND_KEYBOARD, g_Config.input.keyboard_layout,
            INPUT_ROLE_CAMERA_LEFT),
        Input_GetKeyName(
            INPUT_BACKEND_KEYBOARD, g_Config.input.keyboard_layout,
            INPUT_ROLE_CAMERA_RIGHT));
    UI2_Label(tmp);
    UI2_Label(
        "\\{button left} \\{button up} "
        "\\{button down} \\{button right} : ");
    sprintf(tmp, "%s: ", GS(PHOTO_MODE_ROLL_ROLE));
    UI2_Label(tmp);
    sprintf(tmp, "%s: ", GS(KEYMAP_ROLL));
    UI2_Label(tmp);
    sprintf(tmp, "%s: ", GS(PHOTO_MODE_FOV_ROLE));
    UI2_Label(tmp);
    sprintf(tmp, "%s: ", GS(KEYMAP_LOOK));
    UI2_Label(tmp);
    sprintf(
        tmp, "%s: ",
        Input_GetKeyName(
            INPUT_BACKEND_KEYBOARD, g_Config.input.keyboard_layout,
            INPUT_ROLE_TOGGLE_UI));
    UI2_Label(tmp);
    sprintf(tmp, "%s: ", GS(KEYMAP_ACTION));
    UI2_Label(tmp);
    sprintf(
        tmp, "%s/%s: ",
        Input_GetKeyName(
            INPUT_BACKEND_KEYBOARD, g_Config.input.keyboard_layout,
            INPUT_ROLE_TOGGLE_PHOTO_MODE),
        Input_GetKeyName(
            INPUT_BACKEND_KEYBOARD, g_Config.input.keyboard_layout,
            INPUT_ROLE_OPTION));
    UI2_Label(tmp);
    UI2_EndStack();

    UI2_Spacer(4.0f, 0.0f);

    // Behaviors column
    UI2_BeginStack(UI2_STACK_VERTICAL);
    UI2_Label(GS(PHOTO_MODE_MOVE_PROMPT));
    UI2_Label(GS(PHOTO_MODE_ROTATE_PROMPT));
    UI2_Label(GS(PHOTO_MODE_ROLL_PROMPT));
    UI2_Label(GS(PHOTO_MODE_ROTATE90_PROMPT));
    UI2_Label(GS(PHOTO_MODE_FOV_PROMPT));
    UI2_Label(GS(PHOTO_MODE_RESET_PROMPT));
    UI2_Label(GS(MISC_TOGGLE_HELP));
    UI2_Label(GS(PHOTO_MODE_SNAP_PROMPT));
    UI2_Label(GS(MISC_EXIT));
    UI2_EndStack();

    UI2_EndStack();

    UI2_EndStack();
    UI2_EndPad();
    UI2_EndFrame();
    UI2_EndPad();
    UI2_EndModal();
}
