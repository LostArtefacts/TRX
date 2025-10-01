#include "game/phase/phase_picture.h"

#include "game/input.h"
#include "game/output.h"
#include "game/shell.h"
#include "game/ui.h"
#include "memory.h"

typedef enum {
    STATE_FADE_IN,
    STATE_DISPLAY,
    STATE_FADE_OUT,
} M_STATE;

typedef struct {
    M_STATE state;
    FADER fader;
    CLOCK_TIMER timer;
    PHASE_PICTURE_ARGS args;
} M_PRIV;

static void M_FadeOut(M_PRIV *const p)
{
    p->state = STATE_FADE_OUT;
    Fader_Init(&p->fader, FADER_ANY, FADER_BLACK, p->args.fade_out_time);
}

static PHASE_CONTROL M_Start(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Output_LoadBackgroundFromFile(p->args.file_name);
    Fader_Init(&p->fader, FADER_BLACK, FADER_TRANSPARENT, p->args.fade_in_time);
    ClockTimer_Sync(&p->timer);
    return (PHASE_CONTROL) {};
}

static void M_End(PHASE *const phase)
{
    Output_UnloadBackground();
}

static PHASE_CONTROL M_Control(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Input_Update();
    Shell_ProcessInput();

    switch (p->state) {
    case STATE_FADE_IN:
        if (g_InputDB.menu_confirm || g_InputDB.menu_back
            || g_InputDB.menu_skip) {
            M_FadeOut(p);
        } else if (!Fader_IsActive(&p->fader)) {
            p->state = STATE_DISPLAY;
            ClockTimer_Sync(&p->timer);
        }
        break;

    case STATE_DISPLAY:
        if (g_InputDB.menu_confirm || g_InputDB.menu_back || g_InputDB.menu_skip
            || ClockTimer_CheckElapsed(
                &p->timer,
                p->args.display_time
                    - (p->args.display_time_includes_fades
                           ? p->args.fade_in_time + p->args.fade_out_time
                           : 0.0))) {
            M_FadeOut(p);
        }
        break;

    case STATE_FADE_OUT:
        if (g_InputDB.menu_confirm || g_InputDB.menu_back || g_InputDB.menu_skip
            || !Fader_IsActive(&p->fader)) {
            return (PHASE_CONTROL) {
                .action = PHASE_ACTION_END,
                .gf_cmd = { .action = GF_NOOP },
            };
        }
    }

    return (PHASE_CONTROL) {};
}

static void M_Draw(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Output_DrawBackground();
    UI_BeginFade(&p->fader, false);
    UI_EndFade();
}

PHASE *Phase_Picture_Create(const PHASE_PICTURE_ARGS args)
{
    PHASE *const phase = Memory_Alloc(sizeof(PHASE));
    M_PRIV *const p = Memory_Alloc(sizeof(M_PRIV));
    p->args = args;
    p->state = STATE_FADE_IN;
    phase->priv = p;
    phase->start = M_Start;
    phase->end = M_End;
    phase->control = M_Control;
    phase->draw = M_Draw;
    return phase;
}

void Phase_Picture_Destroy(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Memory_Free(p);
    Memory_Free(phase);
}
