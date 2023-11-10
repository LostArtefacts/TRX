#include "game/game.h"

#include "game/gameflow.h"
#include "game/input.h"
#include "game/music.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/requester.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
    .flags = 0,
    .heading_text = NULL,
    .item_texts = &m_PauseStrings[0][0],
    .item_text_len = PAUSE_MAX_TEXT_LENGTH,
    0,
};

static void Game_Pause_RemoveText(void);
static void Game_Pause_UpdateText(void);
static int32_t Game_Pause_DisplayRequester(
    const char *header, const char *option1, const char *option2,
    int16_t requested);
static int32_t Game_Pause_Loop(void);

static void Game_Pause_RemoveText(void)
{
    Text_Remove(m_PausedText);
    m_PausedText = NULL;
}

static void Game_Pause_UpdateText(void)
{
    if (m_PausedText == NULL) {
        m_PausedText = Text_Create(0, -24, g_GameFlow.strings[GS_PAUSE_PAUSED]);
        Text_CentreH(m_PausedText, 1);
        Text_AlignBottom(m_PausedText, 1);
    }
}

static int32_t Game_Pause_DisplayRequester(
    const char *header, const char *option1, const char *option2,
    int16_t requested)
{
    static bool is_pause_text_ready = false;
    if (!is_pause_text_ready) {
        Requester_Init(&m_PauseRequester);
        Requester_SetSize(&m_PauseRequester, 2, -48);
        m_PauseRequester.requested = requested;
        Requester_SetHeading(&m_PauseRequester, header);
        Requester_AddItem(&m_PauseRequester, option1, 0);
        Requester_AddItem(&m_PauseRequester, option2, 0);

        is_pause_text_ready = true;
        g_InputDB = (INPUT_STATE) { 0 };
        g_Input = (INPUT_STATE) { 0 };
    }

    int select = Requester_Display(&m_PauseRequester);
    if (select > 0) {
        is_pause_text_ready = false;
    } else {
        g_InputDB = (INPUT_STATE) { 0 };
        g_Input = (INPUT_STATE) { 0 };
    }
    return select;
}

static int32_t Game_Pause_Loop(void)
{
    int32_t state = 0;

    while (1) {
        Game_DrawScene(false);
        Game_Pause_UpdateText();
        Text_Draw();
        Output_DumpScreen();

        Input_Update();
        Shell_ProcessInput();
        Game_ProcessInput();

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
            int32_t choice = Game_Pause_DisplayRequester(
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
            int32_t choice = Game_Pause_DisplayRequester(
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

bool Game_Pause(void)
{
    g_OldInputDB = g_Input;

    Overlay_HideGameInfo();
    Output_SetupAboveWater(false);

    Music_Pause();
    Sound_PauseAll();
    Game_SetStatus(GMS_IN_PAUSE);

    Output_FadeToSemiBlack(true);
    int32_t select = Game_Pause_Loop();
    Output_FadeToTransparent(true);

    Music_Unpause();
    Sound_UnpauseAll();
    Requester_Remove(&m_PauseRequester);
    Game_Pause_RemoveText();
    Game_RestoreStatus();
    return select < 0;
}
