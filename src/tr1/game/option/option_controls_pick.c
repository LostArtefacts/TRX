#include "game/option/option_controls_pick.h"

#include "game/game_string.h"
#include "game/input.h"
#include "game/text.h"
#include "global/types.h"
#include "global/vars.h"

#include <stdio.h>

typedef enum {
    TEXT_KEYBOARD = 0,
    TEXT_CONTROLLER = 1,
    TEXT_TITLE = 2,
    TEXT_TITLE_BORDER = 3,
    TEXT_NUMBER_OF = 4,
    TEXT_OPTION_MIN = TEXT_KEYBOARD,
    TEXT_OPTION_MAX = TEXT_CONTROLLER,
} PICK_TEXT;

static TEXTSTRING *m_Text[TEXT_NUMBER_OF] = {};

static void M_InitText(void);
static void M_ShutdownText(void);

static void M_InitText(void)
{
    m_Text[TEXT_TITLE_BORDER] = Text_Create(0, -32, " ");
    Text_AddBackground(m_Text[TEXT_TITLE_BORDER], 180, 85, 0, 0, TS_BACKGROUND);
    Text_AddOutline(m_Text[TEXT_TITLE_BORDER], TS_BACKGROUND);

    m_Text[TEXT_TITLE] = Text_Create(0, -30, GS(CONTROL_CUSTOMIZE));
    Text_AddBackground(m_Text[TEXT_TITLE], 176, 0, 0, 0, TS_HEADING);
    Text_AddOutline(m_Text[TEXT_TITLE], TS_HEADING);

    m_Text[TEXT_KEYBOARD] = Text_Create(0, 0, GS(CONTROL_BACKEND_KEYBOARD));
    m_Text[TEXT_CONTROLLER] =
        Text_Create(0, 25, GS(CONTROL_BACKEND_CONTROLLER));

    Text_AddBackground(m_Text[g_OptionSelected], 128, 0, 0, 0, TS_REQUESTED);
    Text_AddOutline(m_Text[g_OptionSelected], TS_REQUESTED);

    for (int i = 0; i < TEXT_NUMBER_OF; i++) {
        Text_CentreH(m_Text[i], 1);
        Text_CentreV(m_Text[i], 1);
    }
}

static void M_ShutdownText(void)
{
    for (int i = 0; i < TEXT_NUMBER_OF; i++) {
        Text_Remove(m_Text[i]);
        m_Text[i] = nullptr;
    }
}

CONTROL_MODE Option_ControlsPick_Control(const bool is_busy)
{
    if (is_busy) {
        return CM_PICK;
    }

    if (!m_Text[TEXT_KEYBOARD]) {
        M_InitText();
    }

    if (g_InputDB.menu_up && g_OptionSelected > TEXT_OPTION_MIN) {
        Text_RemoveOutline(m_Text[g_OptionSelected]);
        Text_RemoveBackground(m_Text[g_OptionSelected]);
        --g_OptionSelected;
        Text_AddBackground(
            m_Text[g_OptionSelected], 128, 0, 0, 0, TS_REQUESTED);
        Text_AddOutline(m_Text[g_OptionSelected], TS_REQUESTED);
    }

    if (g_InputDB.menu_down && g_OptionSelected < TEXT_OPTION_MAX) {
        Text_RemoveOutline(m_Text[g_OptionSelected]);
        Text_RemoveBackground(m_Text[g_OptionSelected]);
        ++g_OptionSelected;
        Text_AddBackground(
            m_Text[g_OptionSelected], 128, 0, 0, 0, TS_REQUESTED);
        Text_AddOutline(m_Text[g_OptionSelected], TS_REQUESTED);
    }

    switch (g_OptionSelected) {
    case TEXT_KEYBOARD:
        if (g_InputDB.menu_confirm) {
            M_ShutdownText();
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
            return CM_KEYBOARD;
        }
        break;

    case TEXT_CONTROLLER:
        if (g_InputDB.menu_confirm) {
            M_ShutdownText();
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
            return CM_CONTROLLER;
        }
        break;
    }

    if (g_InputDB.menu_back) {
        M_ShutdownText();
    }

    return CM_PICK;
}

void Option_ControlsPick_Shutdown(void)
{
    M_ShutdownText();
}
