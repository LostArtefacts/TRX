#include <trx/game/phase/phase_picture.h>

#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/game/const.h>
#include <trx/game/input.h>
#include <trx/game/output.h>
#include <trx/game/shell.h>
#include <trx/game/ui/dialogs/takeover.h>

// The picture is timed, faded and skipped by a script, which ends it with a
// choice. Without one the picture is skipped.
typedef struct {
    PHASE_PICTURE_ARGS args;
    bool is_held;
} M_PRIV;

static int32_t M_ToTicks(const double seconds)
{
    return (int32_t)(seconds * LOGIC_FPS + 0.5);
}

static bool M_UsesCrossFadeIn(PHASE *const phase)
{
    const M_PRIV *const p = phase->priv;
    return p->args.loading_pic && !p->args.block_cross_fade_in;
}

static PHASE_CONTROL M_Start(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    if (!Output_Overlay_LoadImage(p->args.file_name)) {
        return (PHASE_CONTROL) {
            .action = PHASE_ACTION_END,
            .gf_cmd = { .action = GF_NOOP },
        };
    }

    p->is_held = UI_Takeover_Offer(
        UI_TAKEOVER_PICTURE,
        (UI_TAKEOVER_ARGS) {
            .background = p->args.file_name,
            .display_ticks = M_ToTicks(p->args.display_time),
            .fade_in_ticks = M_ToTicks(p->args.fade_in_time),
            .fade_out_ticks = M_ToTicks(p->args.fade_out_time),
            .is_loading = p->args.loading_pic,
            .can_cross_fade_in = M_UsesCrossFadeIn(phase),
        });
    if (!p->is_held) {
        LOG_WARNING("no script draws pictures, skipping %s", p->args.file_name);
        return (PHASE_CONTROL) {
            .action = PHASE_ACTION_END,
            .gf_cmd = { .action = GF_NOOP },
        };
    }
    return (PHASE_CONTROL) {};
}

static void M_End(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    if (p->is_held) {
        UI_Takeover_Release(UI_TAKEOVER_PICTURE);
        p->is_held = false;
    }
}

static PHASE_CONTROL M_Control(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Input_Update();
    Shell_ProcessInput();
    if (UI_Takeover_TakeChoice(UI_TAKEOVER_PICTURE)
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
}

PHASE *Phase_Picture_Create(const PHASE_PICTURE_ARGS args)
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
    phase->uses_cross_fade_in = M_UsesCrossFadeIn;
    return phase;
}

void Phase_Picture_Destroy(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Memory_Free(p);
    Memory_Free(phase);
}
