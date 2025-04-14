#include "game/phase/executor.h"

#include "benchmark.h"
#include "config.h"
#include "game/clock.h"
#include "game/console/common.h"
#include "game/fader.h"
#include "game/game_flow.h"
#include "game/input.h"
#include "game/interpolation.h"
#include "game/output.h"
#include "game/savegame.h"
#include "game/shell.h"
#include "game/text.h"
#include "game/ui2.h"
#include "gfx/gl/track.h"

#define DEBUG_OPTIM 0
#define MAX_PHASES 10

static bool m_Exiting;
static FADER m_ExitFader;
static int32_t m_PhaseStackSize = 0;
static PHASE *m_PhaseStack[MAX_PHASES] = {};

static PHASE_CONTROL M_Control(PHASE *phase, int32_t nframes);
static void M_Draw(PHASE *phase);
static int32_t M_Wait(PHASE *phase);

static PHASE_CONTROL M_Control(PHASE *const phase, const int32_t nframes)
{
    const GF_COMMAND gf_override_cmd = GF_GetOverrideCommand();
    if (gf_override_cmd.action != GF_NOOP) {
        const GF_COMMAND gf_cmd = gf_override_cmd;
        GF_OverrideCommand((GF_COMMAND) { .action = GF_NOOP });

        // A change in the game flow is not natural. Force features like death
        // counter to break from the currently active savegame file.
        Savegame_UnbindSlot();

        return (PHASE_CONTROL) { .action = PHASE_ACTION_END, .gf_cmd = gf_cmd };
    }

    if (Shell_IsExiting() && !m_Exiting) {
        m_Exiting = true;
        if (g_Config.visuals.enable_exit_fade_effects) {
            Fader_Init(&m_ExitFader, FADER_ANY, FADER_BLACK, 1.0 / 3.0);
        }
    } else if (m_Exiting && !Fader_IsActive(&m_ExitFader)) {
        return (PHASE_CONTROL) {
            .action = PHASE_ACTION_END,
            .gf_cmd = { .action = GF_EXIT_GAME },
        };
    }

    if (phase != nullptr && phase->control != nullptr) {
        return phase->control(phase, nframes);
    }
    return (PHASE_CONTROL) {
        .action = PHASE_ACTION_END,
        .gf_cmd = { .action = GF_NOOP },
    };
}

static void M_Draw(PHASE *const phase)
{
    Output_BeginScene();
    UI2_BeginScene();
#if DEBUG_OPTIM
    BENCHMARK benchmark = Benchmark_Start();
#endif
    if (phase != nullptr && phase->draw != nullptr) {
        phase->draw(phase);
    }

    Console_Draw();
    Text_Draw();
    UI2_EndScene();
    Output_DrawPolyList();
    Fader_Draw(&m_ExitFader);

    Output_EndScene();

#if DEBUG_OPTIM
    char buffer[80];
    const GFX_METRICS metrics = GFX_Track_GetMetrics();
    sprintf(
        buffer, "%.03f KB T:%d U:%d", metrics.buffer_total_bytes / 1024.0f,
        metrics.buffer_transfer_count, metrics.uniform_changes);
    Benchmark_End(&benchmark, buffer);
#endif
}

static int32_t M_Wait(PHASE *const phase)
{
    if (phase != nullptr && phase->wait != nullptr) {
        return phase->wait(phase);
    } else {
        return Clock_WaitTick();
    }
}

GF_COMMAND PhaseExecutor_Run(PHASE *const phase)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };

    PHASE *const prev_phase =
        m_PhaseStackSize > 0 ? m_PhaseStack[m_PhaseStackSize - 1] : nullptr;
    if (prev_phase != nullptr && prev_phase->suspend != nullptr) {
        prev_phase->suspend(phase);
    }
    m_PhaseStack[m_PhaseStackSize++] = phase;

    if (phase->start != nullptr) {
        Clock_SyncTick();
        g_OldInputDB = g_Input;
        const PHASE_CONTROL control = phase->start(phase);
        if (Shell_IsExiting()) {
            gf_cmd = (GF_COMMAND) { .action = GF_EXIT_GAME };
            goto finish;
        } else if (control.action == PHASE_ACTION_END) {
            gf_cmd = control.gf_cmd;
            goto finish;
        }
    }

    int32_t nframes = Clock_WaitTick();
    while (true) {
        const PHASE_CONTROL control = M_Control(phase, nframes);

        if (control.action == PHASE_ACTION_END) {
            if (Shell_IsExiting()) {
                gf_cmd = (GF_COMMAND) { .action = GF_EXIT_GAME };
            } else {
                gf_cmd = control.gf_cmd;
            }
            goto finish;
        } else if (control.action == PHASE_ACTION_NO_WAIT) {
            nframes = 0;
            continue;
        } else {
            nframes = 0;
            if (Interpolation_IsEnabled()) {
                Interpolation_SetRate(0.5);
                M_Draw(phase);
                M_Wait(phase);
            }

            Interpolation_SetRate(1.0);
            M_Draw(phase);
            nframes += M_Wait(phase);
        }
    }

finish:
    if (phase->end != nullptr) {
        phase->end(phase);
    }
    if (prev_phase != nullptr && prev_phase->resume != nullptr) {
        Clock_SyncTick();
        prev_phase->resume(phase);
    }
    m_PhaseStackSize--;

    return gf_cmd;
}
