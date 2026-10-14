#include <trx/game/phase/phase_pause.h>

#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/game/input.h>
#include <trx/game/music.h>
#include <trx/game/shell.h>
#include <trx/game/sound.h>
#include <trx/game/ui/dialogs/takeover.h>

// A script draws the pause screen, its backdrop and its fades, and ends it
// with a choice. Without one the game resumes at once.
typedef struct {
    bool is_held;
} M_PRIV;

static PHASE_CONTROL M_Resume(void)
{
    Music_Unpause();
    Sound_UnpauseAll();
    return (PHASE_CONTROL) {
        .action = PHASE_ACTION_END,
        .gf_cmd = { .action = GF_NOOP },
    };
}

static PHASE_CONTROL M_Start(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Music_Pause();
    Sound_PauseAll();
    p->is_held = UI_Takeover_Offer(UI_TAKEOVER_PAUSE, (UI_TAKEOVER_ARGS) {});
    if (!p->is_held) {
        LOG_WARNING("no script draws the pause screen, resuming");
        return M_Resume();
    }
    return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
}

static void M_End(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    if (p->is_held) {
        UI_Takeover_Release(UI_TAKEOVER_PAUSE);
        p->is_held = false;
    }
}

static PHASE_CONTROL M_Control(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Input_Update();
    Shell_ProcessInput();

    switch (UI_Takeover_TakeChoice(UI_TAKEOVER_PAUSE)) {
    case UI_TAKEOVER_CHOICE_CANCEL:
    case UI_TAKEOVER_CHOICE_RESUME:
        p->is_held = false;
        return M_Resume();
    case UI_TAKEOVER_CHOICE_EXIT_TO_TITLE:
        p->is_held = false;
        return (PHASE_CONTROL) {
            .action = PHASE_ACTION_END,
            .gf_cmd = { .action = GF_EXIT_TO_TITLE },
        };
    default:
        return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
    }
}

static void M_Draw(PHASE *const phase)
{
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
