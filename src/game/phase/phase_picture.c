#include "game/phase/phase_picture.h"

#include "game/clock.h"
#include "game/input.h"
#include "game/output.h"
#include "game/shell.h"
#include "global/types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum STATE {
    STATE_FADE_IN,
    STATE_DISPLAY,
    STATE_FADE_OUT,
} STATE;

static STATE m_State = STATE_FADE_IN;
static double m_DisplayTime = 0.0;
static CLOCK_TIMER m_DisplayTimer = { 0 };

static void Phase_Picture_Start(void *arg);
static void Phase_Picture_End(void);
static PHASE_CONTROL Phase_Picture_Control(int32_t nframes);
static void Phase_Picture_Draw(void);

static void Phase_Picture_Start(void *arg)
{
    const PHASE_PICTURE_DATA *data = (const PHASE_PICTURE_DATA *)arg;
    m_State = STATE_FADE_IN;
    m_DisplayTime = data->display_time;
    Clock_ResetTimer(&m_DisplayTimer);
    Output_LoadBackdropImage(data->path);
    Output_FadeResetToBlack();
    Output_FadeToTransparent(true);
}

static void Phase_Picture_End(void)
{
}

static PHASE_CONTROL Phase_Picture_Control(int32_t nframes)
{
    Input_Update();
    Shell_ProcessInput();

    switch (m_State) {
    case STATE_FADE_IN:
        if (!Output_FadeIsAnimating()) {
            m_State = STATE_DISPLAY;
            Clock_ResetTimer(&m_DisplayTimer);
        }
        if (g_InputDB.any) {
            m_State = STATE_FADE_OUT;
        }
        break;

    case STATE_DISPLAY:
        if (g_InputDB.any
            || Clock_CheckElapsedMilliseconds(
                &m_DisplayTimer, m_DisplayTime * 1000)) {
            m_State = STATE_FADE_OUT;
        }
        break;

    case STATE_FADE_OUT:
        Output_FadeToBlack(true);
        if (g_InputDB.any || !Output_FadeIsAnimating()) {
            Output_FadeResetToBlack();
            return (PHASE_CONTROL) {
                .end = true, .command = { .action = GF_CONTINUE_SEQUENCE }
            };
        }
        break;
    }

    return (PHASE_CONTROL) { .end = false };
}

static void Phase_Picture_Draw(void)
{
    Output_AnimateFades();
}

PHASER g_PicturePhaser = {
    .start = Phase_Picture_Start,
    .end = Phase_Picture_End,
    .control = Phase_Picture_Control,
    .draw = Phase_Picture_Draw,
    .wait = NULL,
};
