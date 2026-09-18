#include <trx/game/phase/phase_loading_camera.h>

#include <trx/core/memory.h>
#include <trx/game/camera/vars.h>
#include <trx/game/fader.h>
#include <trx/game/game/draw.h>
#include <trx/game/input.h>
#include <trx/game/interpolation.h>
#include <trx/game/output.h>
#include <trx/game/rooms.h>
#include <trx/game/shell.h>
#include <trx/game/viewport.h>

typedef enum {
    STATE_FADE_IN,
    STATE_DISPLAY,
    STATE_FADE_OUT,
} M_STATE;

typedef struct {
    M_STATE state;
    FADER fader;
    CLOCK_TIMER timer;
    PHASE_LOADING_CAMERA_ARGS args;
} M_PRIV;

static void M_PlaceCamera(const M_PRIV *const p)
{
    g_Camera.type = CAM_LOADING_SCREEN;
    g_Camera.pos = (GAME_VECTOR) {
        .pos = p->args.source,
        .room_num = p->args.room_num,
    };
    g_Camera.target = (GAME_VECTOR) {
        .pos = p->args.target,
        .room_num = p->args.room_num,
    };
    g_Camera.shift = 0;
    g_Camera.roll = 0;
    g_Camera.bounce = 0;
    g_Camera.fov = Viewport_GetEffectiveFOV();
    g_Camera.underwater = Room_Get(p->args.room_num)->flags.underwater;
    Interpolation_Remember();
}

static PHASE_CONTROL M_Start(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    M_PlaceCamera(p);
    Fader_InitTo(&p->fader, 1.0f, 0.0f, p->args.fade_in_time);
    ClockTimer_Sync(&p->timer);
    return (PHASE_CONTROL) {};
}

static PHASE_CONTROL M_Control(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Input_Update();
    Shell_ProcessInput();

    switch (p->state) {
    case STATE_FADE_IN:
        if (g_InputDB.menu_skip) {
            p->state = STATE_FADE_OUT;
            Fader_InitFromCurrentHold(
                &p->fader, 1.0f, p->args.fade_out_time, 0.1f);
        } else if (!Fader_IsActive(&p->fader)) {
            p->state = STATE_DISPLAY;
            ClockTimer_Sync(&p->timer);
        }
        break;

    case STATE_DISPLAY:
        if (g_InputDB.menu_skip
            || ClockTimer_CheckElapsed(&p->timer, p->args.display_time)) {
            p->state = STATE_FADE_OUT;
            Fader_InitFromCurrentHold(
                &p->fader, 1.0f, p->args.fade_out_time, 0.1f);
        }
        break;

    case STATE_FADE_OUT:
        if (g_InputDB.menu_skip || !Fader_IsActive(&p->fader)) {
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
    M_PlaceCamera(p);
    Game_Draw(false);
    Output_Overlay_DrawBlackRectangle(Fader_GetCurrentValue(&p->fader), false);
}

PHASE *Phase_LoadingCamera_Create(const PHASE_LOADING_CAMERA_ARGS args)
{
    PHASE *const phase = Memory_Alloc(sizeof(PHASE));
    M_PRIV *const p = Memory_Alloc(sizeof(M_PRIV));
    p->args = args;
    p->state = STATE_FADE_IN;
    phase->priv = p;
    phase->start = M_Start;
    phase->end = nullptr;
    phase->control = M_Control;
    phase->draw = M_Draw;
    phase->request_fade_to_black = nullptr;
    phase->uses_cross_fade_in = nullptr;
    return phase;
}

void Phase_LoadingCamera_Destroy(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Memory_Free(p);
    Memory_Free(phase);
}
