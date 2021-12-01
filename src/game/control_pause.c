#include "game/control.h"

#include "game/gameflow.h"
#include "game/input.h"
#include "game/music.h"
#include "game/output.h"
#include "game/requester.h"
#include "game/screen.h"
#include "game/text.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "specific/s_misc.h"

#include <stddef.h>

#define PAUSE_MAX_ITEMS 5
#define PAUSE_MAX_TEXT_LENGTH 50

static TEXTSTRING *m_PausedText = NULL;

static char m_PauseStrings[PAUSE_MAX_ITEMS][PAUSE_MAX_TEXT_LENGTH] = { 0 };
static REQUEST_INFO m_PauseRequester = {
    .items = 0,
    .requested = 0,
    .vis_lines = 0,
    .line_offset = 0,
    .line_old_offset = 0,
    .pix_width = 160,
    .line_height = TEXT_HEIGHT + 7,
    .x = 0,
    .y = 0,
    .z = 0,
    .flags = 0,
    .heading_text = NULL,
    .item_texts = &m_PauseStrings[0][0],
    .item_text_len = PAUSE_MAX_TEXT_LENGTH,
    0,
};

static void Control_Pause_RemoveText();
static void Control_Pause_DisplayText();
static int32_t Control_Pause_DisplayRequester(
    const char *header, const char *option1, const char *option2,
    int16_t requested);
static int32_t Control_Pause_Loop();

static void Control_Pause_RemoveText()
{
    Text_Remove(m_PausedText);
    m_PausedText = NULL;
}

static void Control_Pause_DisplayText()
{
    if (m_PausedText == NULL) {
        m_PausedText = Text_Create(0, -24, g_GameFlow.strings[GS_PAUSE_PAUSED]);
        Text_CentreH(m_PausedText, 1);
        Text_AlignBottom(m_PausedText, 1);
    }
}

static int32_t Control_Pause_DisplayRequester(
    const char *header, const char *option1, const char *option2,
    int16_t requested)
{
    static bool is_pause_text_ready = false;
    if (!is_pause_text_ready) {
        InitRequester(&m_PauseRequester);
        SetRequesterSize(&m_PauseRequester, 2, -48);
        m_PauseRequester.requested = requested;
        SetRequesterHeading(&m_PauseRequester, header);
        AddRequesterItem(&m_PauseRequester, option1, 0);
        AddRequesterItem(&m_PauseRequester, option2, 0);

        is_pause_text_ready = true;
        g_InputDB = (INPUT_STATE) { 0 };
        g_Input = (INPUT_STATE) { 0 };
    }

    int select = DisplayRequester(&m_PauseRequester);
    if (select > 0) {
        is_pause_text_ready = false;
    } else {
        g_InputDB = (INPUT_STATE) { 0 };
        g_Input = (INPUT_STATE) { 0 };
    }
    return select;
}

static int32_t Control_Pause_Loop()
{
    int32_t state = 0;

    while (1) {
        Output_InitialisePolyList(0);
        Output_CopyBufferToScreen();
        Control_Pause_DisplayText();
        Text_Draw();
        Output_DumpScreen();
        Input_Update();

        switch (state) {
        case 0:
            if (g_InputDB.pause) {
                return 1;
            }
            if (g_InputDB.option) {
                state = 1;
            }
            break;

        case 1: {
            int32_t choice = Control_Pause_DisplayRequester(
                g_GameFlow.strings[GS_PAUSE_EXIT_TO_TITLE],
                g_GameFlow.strings[GS_PAUSE_CONTINUE],
                g_GameFlow.strings[GS_PAUSE_QUIT], 1);
            if (choice == 1) {
                return 1;
            } else if (choice == 2) {
                state = 2;
            }
            break;
        }

        case 2: {
            int32_t choice = Control_Pause_DisplayRequester(
                g_GameFlow.strings[GS_PAUSE_ARE_YOU_SURE],
                g_GameFlow.strings[GS_PAUSE_YES],
                g_GameFlow.strings[GS_PAUSE_NO], 1);
            if (choice == 1) {
                return -1;
            } else if (choice == 2) {
                return 1;
            }
            break;
        }
        }
    }

    return 0;
}

bool Control_Pause()
{
    g_OldInputDB = g_Input;

    int old_overlay_flag = g_OverlayFlag;
    g_OverlayFlag = -3;
    g_InvMode = INV_PAUSE_MODE;

    Text_RemoveAll();
    S_FadeInInventory(1);
    Output_SetupAboveWater(false);

    Music_Pause();

    int32_t select = Control_Pause_Loop();

    Music_Unpause();
    RemoveRequester(&m_PauseRequester);
    Control_Pause_RemoveText();
    S_FadeOutInventory(1);
    g_OverlayFlag = old_overlay_flag;
    return select < 0;
}
