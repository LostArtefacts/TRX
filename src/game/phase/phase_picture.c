#include "game/phase/phase_picture.h"

#include "game/clock.h"
#include "game/input.h"
#include "game/output.h"
#include "game/shell.h"
#include "global/types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum PICTURE_STATE {
    PICTURE_STATE_FADE_IN,
    PICTURE_STATE_DISPLAY,
    PICTURE_STATE_FADE_OUT,
} PICTURE_STATE;

static PICTURE_STATE m_State = PICTURE_STATE_FADE_IN;
static int32_t m_StartTime = 0;
static double m_DisplayTime = 0.0;

static void Phase_Picture_Start(void *arg);
static void Phase_Picture_End(void);
static GAMEFLOW_OPTION Phase_Picture_Control(int32_t nframes);
static void Phase_Picture_Draw(void);

static void Phase_Picture_Start(void *arg)
{
    const PHASE_PICTURE_DATA *data = (const PHASE_PICTURE_DATA *)arg;
    m_State = PICTURE_STATE_FADE_IN;
    m_DisplayTime = data->display_time;
    Output_LoadBackdropImage(data->path);
    Output_FadeResetToBlack();
    Output_FadeToTransparent(true);
}

static void Phase_Picture_End(void)
{
}

static GAMEFLOW_OPTION Phase_Picture_Control(int32_t nframes)
{
    Input_Update();
    Shell_ProcessInput();

    switch (m_State) {
    case PICTURE_STATE_FADE_IN:
        if (!Output_FadeIsAnimating()) {
            m_State = PICTURE_STATE_DISPLAY;
            m_StartTime = Clock_GetMS();
        }
        if (g_InputDB.any) {
            m_State = PICTURE_STATE_FADE_OUT;
        }
        break;

    case PICTURE_STATE_DISPLAY:
        if (g_InputDB.any
            || (Clock_GetMS() - m_StartTime >= m_DisplayTime * 1000)) {
            m_State = PICTURE_STATE_FADE_OUT;
        }
        break;

    case PICTURE_STATE_FADE_OUT:
        Output_FadeToBlack(true);
        if (g_InputDB.any || !Output_FadeIsAnimating()) {
            return GF_PHASE_BREAK;
        }
        break;
    }

    return GF_PHASE_CONTINUE;
}

static void Phase_Picture_Draw(void)
{
    Output_DrawBackdropImage();
}

PHASER g_PicturePhaser = {
    .start = Phase_Picture_Start,
    .end = Phase_Picture_End,
    .control = Phase_Picture_Control,
    .draw = Phase_Picture_Draw,
    .wait = NULL,
};
