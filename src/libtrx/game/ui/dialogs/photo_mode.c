#include "game/ui/dialogs/photo_mode.h"

#include "config.h"
#include "game/game_string.h"
#include "game/input.h"
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

    char tmp[50];

    UI_BeginModal(0.0f, 0.0f);
    UI_BeginPad(8.0f, 8.0f);
    UI_BeginFrame(UI_FRAME_DIALOG_BACKGROUND);
    UI_BeginPad(8.0, 6.0);

    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_VERTICAL,
        .spacing = { .v = 8.0f },
    });
    UI_Label(GS(PHOTO_MODE_TITLE));

    UI_BeginStack(UI_STACK_HORIZONTAL);

    // Inputs column
    UI_BeginStack(UI_STACK_VERTICAL);
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
    UI_Label(tmp);
    UI_Label(
        "\\{button left} \\{button up} "
        "\\{button down} \\{button right} : ");
    sprintf(tmp, "%s: ", GS(PHOTO_MODE_ROLL_ROLE));
    UI_Label(tmp);
    sprintf(tmp, "%s: ", GS(KEYMAP_ROLL));
    UI_Label(tmp);
    sprintf(tmp, "%s: ", GS(PHOTO_MODE_FOV_ROLE));
    UI_Label(tmp);
    sprintf(tmp, "%s: ", GS(KEYMAP_LOOK));
    UI_Label(tmp);
    sprintf(
        tmp, "%s: ",
        Input_GetKeyName(
            INPUT_BACKEND_KEYBOARD, g_Config.input.keyboard_layout,
            INPUT_ROLE_TOGGLE_UI));
    UI_Label(tmp);
    sprintf(tmp, "%s: ", GS(KEYMAP_ACTION));
    UI_Label(tmp);
    sprintf(
        tmp, "%s/%s: ",
        Input_GetKeyName(
            INPUT_BACKEND_KEYBOARD, g_Config.input.keyboard_layout,
            INPUT_ROLE_TOGGLE_PHOTO_MODE),
        Input_GetKeyName(
            INPUT_BACKEND_KEYBOARD, g_Config.input.keyboard_layout,
            INPUT_ROLE_OPTION));
    UI_Label(tmp);
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
    UI_Label(GS(MISC_TOGGLE_HELP));
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
