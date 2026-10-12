#include <trx/game/phase/phase_globe_select.h>

#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/debug.h>
#include <trx/game/game_flow.h>
#include <trx/game/input.h>
#include <trx/game/output.h>
#include <trx/game/overlay.h>
#include <trx/game/shell.h>
#include <trx/game/ui/dialogs/takeover.h>

// The globe is drawn by a script, which picks an adventure by starting its
// level. Without one the phase has nothing to offer and leaves for the title.
typedef struct {
    PHASE_GLOBE_SELECT_ARGS args;
    bool is_held;
} M_PRIV;

static PHASE_CONTROL M_Start(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    UI_Takeover_SetBackground(
        UI_TAKEOVER_GLOBE_SELECT, p->args.background_path);
    p->is_held = UI_Takeover_Offer(UI_TAKEOVER_GLOBE_SELECT, 0);
    if (!p->is_held) {
        LOG_WARNING("no script draws the globe, leaving for the title");
        return (PHASE_CONTROL) {
            .action = PHASE_ACTION_END,
            .gf_cmd = { .action = GF_EXIT_TO_TITLE },
        };
    }
    return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
}

static void M_End(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    if (p->is_held) {
        UI_Takeover_Release(UI_TAKEOVER_GLOBE_SELECT);
        p->is_held = false;
    }
    UI_Takeover_SetBackground(UI_TAKEOVER_GLOBE_SELECT, nullptr);
    Overlay_SetBottomText((OVERLAY_TEXT) { 0 });
    Overlay_ShowArrow(OVERLAY_ARROW_BCL, false);
    Overlay_ShowArrow(OVERLAY_ARROW_BCR, false);
}

static PHASE_CONTROL M_Control(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Input_Update();
    Shell_ProcessInput();
    // The screen picks the adventure by asking for its level, and the phase
    // ends into it as the inventory ring's globe did.
    const GF_COMMAND gf_cmd = GF_GetOverrideCommand();
    if (gf_cmd.action != GF_NOOP) {
        GF_OverrideCommand((GF_COMMAND) { .action = GF_NOOP }, true);
        return (PHASE_CONTROL) { .action = PHASE_ACTION_END, .gf_cmd = gf_cmd };
    }
    if (UI_Takeover_TakeChoice(UI_TAKEOVER_GLOBE_SELECT)
        != UI_TAKEOVER_CHOICE_NONE) {
        p->is_held = false;
        return (PHASE_CONTROL) {
            .action = PHASE_ACTION_END,
            .gf_cmd = { .action = GF_EXIT_TO_TITLE },
        };
    }
    return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
}

static void M_Draw(PHASE *const phase)
{
    // The script draws the whole screen, the picture behind the globe too.
}

PHASE *Phase_GlobeSelect_Create(const PHASE_GLOBE_SELECT_ARGS args)
{
    PHASE *const phase = Memory_Alloc(sizeof(*phase));
    M_PRIV *const p = Memory_Alloc(sizeof(*p));
    p->args = args;
    phase->priv = p;
    phase->start = M_Start;
    phase->end = M_End;
    phase->control = M_Control;
    phase->draw = M_Draw;
    phase->defers_override = true;
    return phase;
}

void Phase_GlobeSelect_Destroy(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Memory_Free(p);
    Memory_Free(phase);
}
