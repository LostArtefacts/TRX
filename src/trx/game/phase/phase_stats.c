#include <trx/game/phase/phase_stats.h>

#include <trx/core/memory.h>
#include <trx/game/game.h>
#include <trx/game/game_flow.h>
#include <trx/game/input.h>
#include <trx/game/shell.h>
#include <trx/game/ui/dialogs/takeover.h>

// A script draws the screen, its backdrop and its fades, and ends it with a
// choice. Without one there is nothing to show.
typedef struct {
    PHASE_STATS_ARGS args;
    bool is_held;
} M_PRIV;

static PHASE_CONTROL M_Start(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    if (Game_IsInGym()) {
        return (PHASE_CONTROL) {
            .action = PHASE_ACTION_END,
            .gf_cmd = { .action = GF_NOOP },
        };
    }

    const GF_LEVEL *const level = GF_GetLevel(
        GFLT_MAIN,
        p->args.level_num != -1 ? p->args.level_num
                                : Game_GetCurrentLevel()->num);
    p->is_held = UI_Takeover_Offer(
        UI_TAKEOVER_STATS,
        (UI_TAKEOVER_ARGS) {
            .level = level,
            .is_final = p->args.show_final_stats,
            .is_bare = p->args.use_bare_style,
            .background = p->args.background_path,
        });
    if (!p->is_held) {
        return (PHASE_CONTROL) {
            .action = PHASE_ACTION_END,
            .gf_cmd = { .action = GF_NOOP },
        };
    }
    return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
}

static void M_End(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    if (p->is_held) {
        UI_Takeover_Release(UI_TAKEOVER_STATS);
        p->is_held = false;
    }
}

static PHASE_CONTROL M_Control(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Input_Update();
    Shell_ProcessInput();
    if (UI_Takeover_TakeChoice(UI_TAKEOVER_STATS) != UI_TAKEOVER_CHOICE_NONE) {
        p->is_held = false;
        return (PHASE_CONTROL) {
            .action = PHASE_ACTION_END,
            .gf_cmd = { .action = GF_NOOP },
        };
    }
    return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
}

static void M_Draw(PHASE *const phase)
{
}

PHASE *Phase_Stats_Create(const PHASE_STATS_ARGS args)
{
    PHASE *const phase = Memory_Alloc(sizeof(PHASE));
    M_PRIV *const p = Memory_Alloc(sizeof(M_PRIV));
    p->args = args;
    phase->priv = p;
    phase->start = M_Start;
    phase->end = M_End;
    phase->control = M_Control;
    phase->draw = M_Draw;
    return phase;
}

void Phase_Stats_Destroy(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Memory_Free(p);
    Memory_Free(phase);
}
