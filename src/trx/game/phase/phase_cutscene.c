#include <trx/game/phase/phase_cutscene.h>

#include <trx/config.h>
#include <trx/core/memory.h>
#include <trx/game/cutscene.h>
#include <trx/game/fader.h>
#include <trx/game/game.h>
#include <trx/game/lara/common.h>
#include <trx/game/lua/events.h>
#include <trx/game/output.h>
#include <trx/game/output/overlay.h>
#include <trx/version.h>

#define M_CROSS_FADE_DURATION 0.5f

typedef struct {
    PHASE_CUTSCENE_ARGS args;
    FADER cross_fader;
} M_PRIV;

static bool M_UsesCrossFadeIn(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    return p->args.cross_fade_in && g_Config.visuals.enable_fade_effects
        && g_TRVersion == 3;
}

static PHASE_CONTROL M_Start(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    if (phase->uses_cross_fade_in != nullptr
        && phase->uses_cross_fade_in(phase)) {
        Output_Overlay_CaptureSnapshot();
        Fader_InitTo(&p->cross_fader, 1.0f, 0.0f, M_CROSS_FADE_DURATION);
    }

    if (!Cutscene_Start(p->args.level_num)) {
        return (PHASE_CONTROL) {
            .action = PHASE_ACTION_END,
            .gf_cmd = { .action = GF_NOOP },
        };
    }
    Game_SetIsPlaying(true);
    Lara_SetControllable(false);
    // The same event a played level fires: whatever the level is, this is the
    // moment it starts running.
    // A cutscene level is never resumed from a save.
    LUA_FireEventBool(LUA_EVENT_GAME_START, false);
    return (PHASE_CONTROL) {};
}

static void M_End(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Game_SetIsPlaying(false);
    Cutscene_End();
}

static void M_Suspend(PHASE *const phase)
{
    Game_SetIsPlaying(false);
}

static void M_Resume(PHASE *const phase)
{
    Game_SetIsPlaying(true);
}

static PHASE_CONTROL M_Control(PHASE *const phase)
{
    LUA_FireEvent(LUA_EVENT_BEFORE_CONTROL);
    M_PRIV *const p = phase->priv;
    const GF_COMMAND gf_cmd = Cutscene_Control();
    LUA_FireEvent(LUA_EVENT_AFTER_CONTROL);
    if (gf_cmd.action != GF_NOOP) {
        return (PHASE_CONTROL) {
            .action = PHASE_ACTION_END,
            .gf_cmd = gf_cmd,
        };
    }
    return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
}

static void M_Draw(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Cutscene_Draw();
    if (phase->uses_cross_fade_in != nullptr
        && phase->uses_cross_fade_in(phase)) {
        Output_Overlay_DrawSnapshot(Fader_GetCurrentValue(&p->cross_fader));
    }
}

PHASE *Phase_Cutscene_Create(const PHASE_CUTSCENE_ARGS args)
{
    PHASE *const phase = Memory_Alloc(sizeof(PHASE));
    M_PRIV *const p = Memory_Alloc(sizeof(M_PRIV));
    p->args = args;
    phase->priv = p;
    phase->start = M_Start;
    phase->end = M_End;
    phase->suspend = M_Suspend;
    phase->resume = M_Resume;
    phase->control = M_Control;
    phase->draw = M_Draw;
    phase->uses_cross_fade_in = M_UsesCrossFadeIn;
    return phase;
}

void Phase_Cutscene_Destroy(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Memory_Free(p);
    Memory_Free(phase);
}
