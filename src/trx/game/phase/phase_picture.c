#include <trx/game/phase/phase_picture.h>

#include <trx/game/fader.h>
#include <trx/game/input.h>
#include <trx/game/output.h>
#include <trx/game/output/background.h>
#include <trx/game/shell.h>
#include <trx/game/ui.h>
#include <trx/memory.h>

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
    bool snapshot_enabled;
    bool end_requested;
    bool has_drawn;
} M_PRIV;

static void M_FadeOut(M_PRIV *const p)
{
    p->state = STATE_FADE_OUT;
    Fader_Init(&p->fader, FADER_ANY, FADER_BLACK, p->args.fade_out_time);
}

static PHASE_CONTROL M_Start(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    if (!Output_LoadBackgroundFromFile(p->args.file_name)) {
        return (PHASE_CONTROL) {
            .action = PHASE_ACTION_END,
            .gf_cmd = { .action = GF_NOOP },
        };
    }

    Output_Background_EnableSnapshot(true);
    Output_Background_CaptureSnapshotPresented();
    p->snapshot_enabled = true;
    Output_Background_SetOverlayOpacity(0.0f);

    if (p->args.loading_pic) {
        Fader_Init(
            &p->fader, FADER_TRANSPARENT, FADER_BLACK, p->args.fade_in_time);
    } else {
        Fader_Init(
            &p->fader, FADER_BLACK, FADER_TRANSPARENT, p->args.fade_in_time);
    }
    ClockTimer_Sync(&p->timer);
    return (PHASE_CONTROL) {};
}

static void M_End(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    if (p->snapshot_enabled) {
        Output_Background_EnableSnapshot(false);
        p->snapshot_enabled = false;
    }
    Output_UnloadBackground();
}

static PHASE_CONTROL M_Control(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Input_Update();
    Shell_ProcessInput();

    const int32_t fade_value = Fader_GetRealValue(&p->fader);
    p->fader.target_drawn |= fade_value == p->fader.args.target;

    switch (p->state) {
    case STATE_FADE_IN:
        if (g_InputDB.menu_confirm || g_InputDB.menu_back
            || g_InputDB.menu_skip) {
            M_FadeOut(p);
        } else if (!Fader_IsActive(&p->fader)) {
            p->state = STATE_DISPLAY;
            ClockTimer_Sync(&p->timer);
            if (p->snapshot_enabled) {
                Output_Background_EnableSnapshot(false);
                p->snapshot_enabled = false;
            }
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
        if (p->args.loading_pic && p->has_drawn) {
            Output_Background_BeginTransitionFadeOut(p->args.fade_out_time);
            return (PHASE_CONTROL) {
                .action = PHASE_ACTION_END,
                .gf_cmd = { .action = GF_NOOP },
            };
        }

        if (g_InputDB.menu_confirm || g_InputDB.menu_back || g_InputDB.menu_skip
            || !Fader_IsActive(&p->fader)) {
            return (PHASE_CONTROL) {
                .action = PHASE_ACTION_END,
                .gf_cmd = { .action = GF_NOOP },
            };
        }
        break;
    }

    return (PHASE_CONTROL) {};
}

static void M_Draw(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    if (p->args.loading_pic) {
        if (p->state == STATE_FADE_IN) {
            const float overlay_opacity =
                Fader_GetRealValue(&p->fader) / 255.0f;
            Output_Background_SetOverlayOpacity(overlay_opacity);
        } else {
            Output_Background_SetOverlayOpacity(1.0f);
        }
        Output_DrawBackground();
    } else {
        Output_Background_SetOverlayOpacity(1.0f);
        Output_DrawBackground();
        UI_BeginFade(&p->fader, false);
        UI_EndFade();
    }
    p->has_drawn = true;
}

PHASE *Phase_Picture_Create(const PHASE_PICTURE_ARGS args)
{
    PHASE *const phase = Memory_Alloc(sizeof(PHASE));
    M_PRIV *const p = Memory_Alloc(sizeof(M_PRIV));
    p->args = args;
    p->state = STATE_FADE_IN;
    p->snapshot_enabled = false;
    p->end_requested = false;
    p->has_drawn = false;
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
