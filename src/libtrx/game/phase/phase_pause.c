#include "game/phase/phase_pause.h"

#include "game/fader.h"
#include "game/game.h"
#include "game/game_string.h"
#include "game/input.h"
#include "game/interpolation.h"
#include "game/music.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/ui.h"
#include "memory.h"

#include <stdint.h>

#define FADE_TIME 0.4

typedef enum {
    STATE_FADE_IN,
    STATE_WAIT,
    STATE_ASK,
    STATE_FADE_OUT,
} STATE;

typedef struct {
    STATE state;
    struct {
        bool is_ready;
        UI_PAUSE_STATE state;
    } ui;
    GF_ACTION action;
    FADER back_fader;
} M_PRIV;

static void M_RemoveText(M_PRIV *const p)
{
    Overlay_SetBottomText(nullptr, false);
}

static void M_FadeIn(M_PRIV *const p)
{
    p->state = STATE_FADE_IN;
    Fader_Init(
        &p->back_fader, FADER_TRANSPARENT, FADER_ALMOST_BLACK, FADE_TIME);
}

static void M_FadeOut(M_PRIV *const p)
{
    M_RemoveText(p);
    p->ui.is_ready = false;
    if (p->action == GF_NOOP) {
        Fader_Init(&p->back_fader, FADER_ANY, FADER_TRANSPARENT, FADE_TIME);
    } else {
        Fader_Init(&p->back_fader, FADER_ANY, FADER_BLACK, FADE_TIME);
    }
    p->state = STATE_FADE_OUT;
}

static void M_PauseGame(M_PRIV *const p)
{
    p->action = GF_NOOP;
    Music_Pause();
    Sound_PauseAll();
    M_FadeIn(p);
}

static void M_ReturnToGame(M_PRIV *const p)
{
    Music_Unpause();
    Sound_UnpauseAll();
    M_FadeOut(p);
}

static void M_ExitToTitle(M_PRIV *const p)
{
    p->action = GF_EXIT_TO_TITLE;
    M_FadeOut(p);
}

static void M_CreateText(M_PRIV *const p)
{
    Overlay_SetBottomTextPtr(GS_PTR(PAUSE_PAUSED), false);
}

static PHASE_CONTROL M_Start(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;

    p->ui.is_ready = false;
    UI_Pause_Init(&p->ui.state);
    M_PauseGame(p);
    return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
}

static void M_End(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    M_RemoveText(p);
    UI_Pause_Free(&p->ui.state);
}

static PHASE_CONTROL M_Control(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Input_Update();
    Shell_ProcessInput();

    if (p->ui.is_ready) {
        UI_Pause_Control(&p->ui.state);
    }

    switch (p->state) {
    case STATE_FADE_IN:
        if (g_InputDB.pause) {
            M_ReturnToGame(p);
            return (PHASE_CONTROL) { .action = PHASE_ACTION_NO_WAIT };
        } else if (!Fader_IsActive(&p->back_fader)) {
            p->state = STATE_WAIT;
            M_CreateText(p);
            return (PHASE_CONTROL) { .action = PHASE_ACTION_NO_WAIT };
        }
        break;

    case STATE_WAIT:
        if (g_InputDB.pause) {
            M_ReturnToGame(p);
            return (PHASE_CONTROL) { .action = PHASE_ACTION_NO_WAIT };
        } else if (g_InputDB.option) {
            p->state = STATE_ASK;
        }
        break;

    case STATE_ASK: {
        const UI_PAUSE_EXIT_CHOICE choice = UI_Pause_Control(&p->ui.state);
        switch (choice) {
        case UI_PAUSE_RESUME_PAUSE:
            p->state = STATE_WAIT;
            return (PHASE_CONTROL) { .action = PHASE_ACTION_NO_WAIT };
        case UI_PAUSE_EXIT_TO_GAME:
            M_ReturnToGame(p);
            return (PHASE_CONTROL) { .action = PHASE_ACTION_NO_WAIT };
        case UI_PAUSE_EXIT_TO_TITLE:
            M_ExitToTitle(p);
            return (PHASE_CONTROL) { .action = PHASE_ACTION_NO_WAIT };
        default:
            break;
        }
        break;
    }

    case STATE_FADE_OUT:
        if (!Fader_IsActive(&p->back_fader)) {
            return (PHASE_CONTROL) {
                .action = PHASE_ACTION_END,
                .gf_cmd = { .action = p->action },
            };
        }
        break;
    }

    return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
}

static void M_Draw(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;

    Interpolation_Disable();
    Game_Draw(false);
    Interpolation_Enable();
    UI_BeginFade(&p->back_fader, false);
    UI_EndFade();

    if (p->state == STATE_ASK) {
        UI_Pause(&p->ui.state);
    }
}

PHASE *Phase_Pause_Create(void)
{
    PHASE *const phase = Memory_Alloc(sizeof(PHASE));
    phase->priv = Memory_Alloc(sizeof(M_PRIV));
    phase->start = M_Start;
    phase->end = M_End;
    phase->control = M_Control;
    phase->draw = M_Draw;
    return phase;
}

void Phase_Pause_Destroy(PHASE *phase)
{
    Memory_Free(phase->priv);
    Memory_Free(phase);
}
