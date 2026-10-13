#include <trx/game/phase/phase_stats.h>

#include <trx/config.h>
#include <trx/core/memory.h>
#include <trx/debug.h>
#include <trx/game/game.h>
#include <trx/game/game_flow.h>
#include <trx/game/input.h>
#include <trx/game/output.h>
#include <trx/game/shell.h>
#include <trx/game/ui/dialogs/takeover.h>

// The screen and its backdrop are drawn and faded in by a script.
typedef enum {
    STATE_DISPLAY,
    STATE_FINISH,
} STATE;

typedef struct {
    PHASE_STATS_ARGS args;
    STATE state;
} M_PRIV;

static bool M_EnableFade(const M_PRIV *const p)
{
    return g_Config.ui.stats_fade_effects || p->args.show_final_stats;
}

static PHASE_CONTROL M_Start(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;

    // A script draws the statistics, and a screen that no script takes has
    // nothing to show.
    if (!Game_IsInGym()) {
        const GF_LEVEL *const level = GF_GetLevel(
            GFLT_MAIN,
            p->args.level_num != -1 ? p->args.level_num
                                    : Game_GetCurrentLevel()->num);
        if (!UI_Takeover_Offer(
                UI_TAKEOVER_STATS,
                UI_TAKEOVER_STATS_ARG(
                    GF_GetLevelOrdinalNumber(GFLT_MAIN, level),
                    p->args.show_final_stats, p->args.use_bare_style))) {
            p->state = STATE_FINISH;
            return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
        }
    }

    Output_Overlay_CaptureGameSnapshot();
    if (p->args.background_path != nullptr
        && !Output_Overlay_LoadImage(p->args.background_path)) {
        LOG_WARNING(
            "Failed to load background image: %s", p->args.background_path);
    }
    UI_Takeover_SetBackground(UI_TAKEOVER_STATS, p->args.background_path);
    p->state = Game_IsInGym() ? STATE_FINISH : STATE_DISPLAY;

    return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
}

static void M_End(PHASE *const phase)
{
    UI_Takeover_Release(UI_TAKEOVER_STATS);
    UI_Takeover_SetBackground(UI_TAKEOVER_STATS, nullptr);
}

static PHASE_CONTROL M_Control(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Input_Update();
    Shell_ProcessInput();
    if (g_InputDB.menu_skip) {
        Input_HoldOffSkip(INPUT_SKIP_TO_SCREEN);
    }

    if (UI_Takeover_IsHeld(UI_TAKEOVER_STATS)
        && UI_Takeover_TakeChoice(UI_TAKEOVER_STATS)
            != UI_TAKEOVER_CHOICE_NONE) {
        p->state = STATE_FINISH;
    }

    switch (p->state) {
    case STATE_DISPLAY:
        if (g_InputDB.menu_skip) {
            p->state = STATE_FINISH;
        }
        break;

    case STATE_FINISH:
        return (PHASE_CONTROL) {
            .action = PHASE_ACTION_END,
            .gf_cmd = { .action = GF_NOOP },
        };
    }

    return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
}

static bool M_RequestFadeToBlack(PHASE *const phase, FADER_ARGS *const out_args)
{
    M_PRIV *const p = phase->priv;
    if (!M_EnableFade(p)) {
        return false;
    }

    if (out_args != nullptr) {
        *out_args = (FADER_ARGS) {
            .from_current = false,
            .initial = 0.0f,
            .target = 1.0f,
            .duration = 0.5f,
            .debuff = 0.1f,
        };
    }
    return true;
}

static void M_Draw(PHASE *const phase)
{
}

PHASE *Phase_Stats_Create(const PHASE_STATS_ARGS args)
{
    PHASE *const phase = Memory_Alloc(sizeof(PHASE));
    M_PRIV *const p = Memory_Alloc(sizeof(M_PRIV));
    p->args = args;
    p->state = STATE_DISPLAY;
    phase->priv = p;
    phase->start = M_Start;
    phase->end = M_End;
    phase->control = M_Control;
    phase->draw = M_Draw;
    phase->request_fade_to_black = M_RequestFadeToBlack;
    phase->uses_cross_fade_in = nullptr;
    return phase;
}

void Phase_Stats_Destroy(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Memory_Free(p);
    Memory_Free(phase);
}
