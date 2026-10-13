#include <trx/game/phase/phase_loading_camera.h>

#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/game/camera/vars.h>
#include <trx/game/const.h>
#include <trx/game/game/draw.h>
#include <trx/game/input.h>
#include <trx/game/interpolation.h>
#include <trx/game/output.h>
#include <trx/game/rooms.h>
#include <trx/game/shell.h>
#include <trx/game/ui/dialogs/takeover.h>
#include <trx/game/viewport.h>

// The view is timed, faded and skipped by a script, which ends it with a
// choice. Without one the view is skipped.
typedef struct {
    PHASE_LOADING_CAMERA_ARGS args;
    CAMERA_INFO old_camera;
    bool is_held;
} M_PRIV;

static int32_t M_ToTicks(const double seconds)
{
    return (int32_t)(seconds * LOGIC_FPS + 0.5);
}

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
    p->old_camera = g_Camera;
    M_PlaceCamera(p);
    p->is_held = UI_Takeover_Offer(
        UI_TAKEOVER_LEVEL_VIEW,
        (UI_TAKEOVER_ARGS) {
            .display_ticks = M_ToTicks(p->args.display_time),
            .fade_in_ticks = M_ToTicks(p->args.fade_in_time),
            .fade_out_ticks = M_ToTicks(p->args.fade_out_time),
        });
    if (!p->is_held) {
        LOG_WARNING("no script draws the level views, skipping them");
        return (PHASE_CONTROL) {
            .action = PHASE_ACTION_END,
            .gf_cmd = { .action = GF_NOOP },
        };
    }
    return (PHASE_CONTROL) {};
}

// Restores the camera after the view ends.
static void M_End(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    if (p->is_held) {
        UI_Takeover_Release(UI_TAKEOVER_LEVEL_VIEW);
        p->is_held = false;
    }
    g_Camera = p->old_camera;
    Interpolation_Remember();
}

static PHASE_CONTROL M_Control(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Input_Update();
    Shell_ProcessInput();
    if (UI_Takeover_TakeChoice(UI_TAKEOVER_LEVEL_VIEW)
        != UI_TAKEOVER_CHOICE_NONE) {
        p->is_held = false;
        return (PHASE_CONTROL) {
            .action = PHASE_ACTION_END,
            .gf_cmd = { .action = GF_NOOP },
        };
    }
    return (PHASE_CONTROL) {};
}

static void M_Draw(PHASE *const phase)
{
    const M_PRIV *const p = phase->priv;
    M_PlaceCamera(p);
    Game_Draw(false);
}

PHASE *Phase_LoadingCamera_Create(const PHASE_LOADING_CAMERA_ARGS args)
{
    PHASE *const phase = Memory_Alloc(sizeof(PHASE));
    M_PRIV *const p = Memory_Alloc(sizeof(M_PRIV));
    p->args = args;
    phase->priv = p;
    phase->start = M_Start;
    phase->end = M_End;
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
