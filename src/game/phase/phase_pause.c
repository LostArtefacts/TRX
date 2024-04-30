#include "game/phase/phase_pause.h"

#include "game/game.h"
#include "game/game_string.h"
#include "game/input.h"
#include "game/interpolation.h"
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

typedef enum STATE {
    STATE_DEFAULT,
    STATE_ASK,
    STATE_CONFIRM,
} STATE;

static STATE m_PauseState = STATE_DEFAULT;

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

static void Phase_Pause_RemoveText(void);
static void Phase_Pause_UpdateText(void);
static int32_t Phase_Pause_DisplayRequester(
    const char *header, const char *option1, const char *option2,
    int16_t requested);

static void Phase_Pause_Start(void *arg);
static void Phase_Pause_End(void);
static GAMEFLOW_OPTION Phase_Pause_Control(int32_t nframes);
static void Phase_Pause_Draw(void);

static void Phase_Pause_RemoveText(void)
{
    Text_Remove(m_PausedText);
    m_PausedText = NULL;
}

static void Phase_Pause_UpdateText(void)
{
    if (m_PausedText == NULL) {
        m_PausedText = Text_Create(0, -24, GS(PAUSE_PAUSED));
        Text_CentreH(m_PausedText, 1);
        Text_AlignBottom(m_PausedText, 1);
    }
}

static int32_t Phase_Pause_DisplayRequester(
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

static void Phase_Pause_Start(void *arg)
{
    g_OldInputDB = g_Input;

    Overlay_HideGameInfo();
    Output_SetupAboveWater(false);

    Music_Pause();
    Sound_PauseAll();

    Output_FadeToSemiBlack(true);

    m_PauseState = STATE_DEFAULT;
}

static void Phase_Pause_End(void)
{
    Output_FadeToTransparent(true);

    Requester_Remove(&m_PauseRequester);
    Phase_Pause_RemoveText();
}

static GAMEFLOW_OPTION Phase_Pause_Control(int32_t nframes)
{
    Phase_Pause_UpdateText();

    Input_Update();
    Shell_ProcessInput();
    Game_ProcessInput();

    switch (m_PauseState) {
    case STATE_DEFAULT:
        if (g_InputDB.pause) {
            Music_Unpause();
            Sound_UnpauseAll();
            Phase_Set(PHASE_GAME, NULL);
        } else if (g_InputDB.option) {
            m_PauseState = STATE_ASK;
        }
        break;

    case STATE_ASK: {
        int32_t choice = Phase_Pause_DisplayRequester(
            GS(PAUSE_EXIT_TO_TITLE), GS(PAUSE_CONTINUE), GS(PAUSE_QUIT), 1);
        if (choice == 1) {
            Music_Unpause();
            Sound_UnpauseAll();
            Phase_Set(PHASE_GAME, NULL);
        } else if (choice == 2) {
            m_PauseState = STATE_CONFIRM;
        }
        break;
    }

    case STATE_CONFIRM: {
        int32_t choice = Phase_Pause_DisplayRequester(
            GS(PAUSE_ARE_YOU_SURE), GS(PAUSE_YES), GS(PAUSE_NO), 1);
        if (choice == 1) {
            return GF_EXIT_TO_TITLE;
        } else if (choice == 2) {
            Music_Unpause();
            Sound_UnpauseAll();
            Phase_Set(PHASE_GAME, NULL);
        }
        break;
    }
    }

    return GF_PHASE_CONTINUE;
}

static void Phase_Pause_Draw(void)
{
    Interpolation_Disable();
    Game_DrawScene(false);
    Interpolation_Enable();
    Output_AnimateFades();
    Text_Draw();
}

PHASER g_PausePhaser = {
    .start = Phase_Pause_Start,
    .end = Phase_Pause_End,
    .control = Phase_Pause_Control,
    .draw = Phase_Pause_Draw,
    .wait = NULL,
};
