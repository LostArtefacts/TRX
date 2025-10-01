#include "game/ui/dialogs/photo_mode.h"

#include "config.h"
#include "game/game_string.h"
#include "game/lara/pose.h"
#include "game/ui/elements/frame.h"
#include "game/ui/elements/label.h"
#include "game/ui/elements/modal.h"
#include "game/ui/elements/pad.h"
#include "game/ui/elements/spacer.h"
#include "game/ui/elements/stack.h"

#include <stdio.h>

static bool M_HasIcon(const INPUT_ROLE role)
{
    return Input_GetKeyName(
               g_Config.input.backend,
               g_Config.input.layout[g_Config.input.backend], role)
        != nullptr;
}

static void M_Title(const PHOTO_MODE current_mode)
{
    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_HORIZONTAL,
        .align = { .h = UI_STACK_H_ALIGN_DISTRIBUTE },
        .spacing = { .v = 8.0f },
    });
    switch (current_mode) {
    case PHOTO_MODE_CAMERA:
        UI_Label(GS(PHOTO_MODE_TITLE_CAMERA_POS));
        break;
    case PHOTO_MODE_LARA_POS:
        UI_Label(GS(PHOTO_MODE_TITLE_LARA_POS));
        break;
    }
    UI_Label("\\{input step_left}\\{input step_right}");
    UI_EndStack();
}

static void M_Inputs(const PHOTO_MODE current_mode)
{
    UI_Label(
        "\\{input camera_up}\\{input camera_down}"
        "\\{input camera_forward}\\{input camera_back}"
        "\\{input camera_left}\\{input camera_right}");
    UI_Label(
        "\\{input left}\\{input forward}"
        "\\{input back}\\{input right}");
    UI_Label("\\{input slow}+\\{input camera_up}/\\{input camera_down}");
    UI_Label("\\{input roll}");
    UI_Label("\\{input look}");

    UI_Label("[\\{input slow}+]\\{input draw}");
    if (Lara_Pose_IsAvailable()) {
        UI_Label("[\\{input slow}+]\\{input fly_cheat}");
    }
    UI_Label("[\\{input slow}+]\\{input pause}");
    UI_Label("\\{input toggle_ui}");
    UI_Label("\\{input action}");

    if (M_HasIcon(INPUT_ROLE_TOGGLE_PHOTO_MODE)
        && M_HasIcon(INPUT_ROLE_INVENTORY)) {
        UI_Label("\\{input toggle_photo_mode}/\\{input option}");
    } else if (M_HasIcon(INPUT_ROLE_TOGGLE_PHOTO_MODE)) {
        UI_Label("\\{input toggle_photo_mode}");
    } else if (M_HasIcon(INPUT_ROLE_INVENTORY)) {
        UI_Label("\\{input option}");
    }
}

static void M_Actions(const PHOTO_MODE current_mode)
{
    switch (current_mode) {
    case PHOTO_MODE_CAMERA:
        UI_Label(GS(PHOTO_MODE_CAMERA_MOVE_PROMPT));
        UI_Label(GS(PHOTO_MODE_CAMERA_ROTATE_PROMPT));
        UI_Label(GS(PHOTO_MODE_CAMERA_ROLL_PROMPT));
        UI_Label(GS(PHOTO_MODE_CAMERA_ROTATE_90_PROMPT));
        UI_Label(GS(PHOTO_MODE_CAMERA_RESET_PROMPT));
        break;
    case PHOTO_MODE_LARA_POS:
        UI_Label(GS(PHOTO_MODE_LARA_MOVE_PROMPT));
        UI_Label(GS(PHOTO_MODE_LARA_ROTATE_PROMPT));
        UI_Label(GS(PHOTO_MODE_LARA_ROLL_PROMPT));
        UI_Label(GS(PHOTO_MODE_LARA_ROTATE_90_PROMPT));
        UI_Label(GS(PHOTO_MODE_LARA_RESET_PROMPT));
        break;
    }
    UI_Label(GS(PHOTO_MODE_FOV_PROMPT));
    if (Lara_Pose_IsAvailable()) {
        UI_Label(GS(PHOTO_MODE_CHANGE_LARA_POSE));
    }
    UI_Label(GS(PHOTO_MODE_ADVANCE_FRAME));
    UI_Label(GS(PHOTO_MODE_TOGGLE_HELP));
    UI_Label(GS(PHOTO_MODE_SNAP_PROMPT));
    UI_Label(GS(MISC_EXIT));
}

void UI_PhotoMode(const PHOTO_MODE current_mode)
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
        .align = { .h = UI_STACK_H_ALIGN_SPAN },
        .spacing = { .v = 8.0f },
    });

    M_Title(current_mode);

    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_HORIZONTAL,
        .spacing = { .h = 8.0f },
    });

    // Inputs column
    UI_BeginStack(UI_STACK_VERTICAL);
    M_Inputs(current_mode);
    UI_EndStack();
    UI_BeginStack(UI_STACK_VERTICAL);
    M_Actions(current_mode);
    UI_EndStack();

    UI_EndStack();

    UI_EndStack();
    UI_EndPad();
    UI_EndFrame();
    UI_EndPad();
    UI_EndModal();
}
