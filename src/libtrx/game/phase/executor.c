#include "game/phase/executor.h"

#include "benchmark.h"
#include "config.h"
#include "game/clock.h"
#include "game/console/common.h"
#include "game/fader.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/input.h"
#include "game/interpolation.h"
#include "game/music.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/savegame.h"
#include "game/shell.h"
#include "game/ui.h"
#include "gfx/context.h"
#include "gfx/gl/track.h"

#define M_MAX_PHASES 10

static int32_t m_CurrentFrame = 0;
static bool m_Exiting;
static FADER m_ExitFader;
static int32_t m_PhaseStackSize = 0;
static PHASE *m_PhaseStack[M_MAX_PHASES] = {};

static GF_COMMAND M_HandleOverride(void)
{
    const GF_COMMAND gf_override_cmd = GF_GetOverrideCommand();
    if (gf_override_cmd.action != GF_NOOP) {
        const GF_COMMAND gf_cmd = gf_override_cmd;
        GF_OverrideCommand((GF_COMMAND) { .action = GF_NOOP });

        // A change in the game flow is not natural. Force features like death
        // counter to break from the currently active savegame file.
        Savegame_UnbindSlot();
        // This flag needs to be cleared as well.
        Game_SetIsPlaying(false);
        // Usually, sequences permit music to flow through - for instance, the
        // end of level screen in The Great Wall transitioning to Venice.
        // We must stop it manually here when derailing the sequence (#3469).
        Music_Stop();

        return gf_cmd;
    }
    return (GF_COMMAND) { .action = GF_NOOP };
}

static PHASE_CONTROL M_Control(PHASE *const phase)
{
    m_CurrentFrame++;
    Shell_ProcessEvents();
    Console_Control();
    Overlay_Control();

    const GF_COMMAND gf_cmd = M_HandleOverride();
    if (gf_cmd.action != GF_NOOP) {
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
        return phase->control(phase);
    }
    return (PHASE_CONTROL) {
        .action = PHASE_ACTION_END,
        .gf_cmd = { .action = GF_NOOP },
    };
}

static void M_Draw(PHASE *const phase)
{
    BENCHMARK benchmark = Benchmark_Start();

    Output_BeginScene();
    Output_SwitchViewport(VIEWPORT_GAME);
    UI_BeginScene();
    UI_BeginFade(&m_ExitFader, true);
    if (phase != nullptr && phase->draw != nullptr) {
        phase->draw(phase);
    }

    Overlay_Draw();
    Console_Draw();
    UI_EndFade();
    UI_EndScene();

    Output_SwitchViewport(VIEWPORT_UI);
    UI_Draw();

    Output_Flush();
    Output_EndScene();

    if (Shell_GetArgs()->debug_render_performance) {
        char buffer[80];
        const GFX_METRICS metrics = GFX_Track_GetMetrics();
        sprintf(
            buffer, "%.03f KB T:%d U:%d Vo:%d Vt:%d",
            metrics.buffer_total_bytes / 1024.0f, metrics.buffer_transfer_count,
            metrics.uniform_changes, metrics.opaque_vert_count,
            metrics.trans_vert_count);
        Benchmark_End(&benchmark, buffer);
    }

    if (!Output_IsHeadless()
        || GFX_Context_GetScheduledScreenshotPath() != nullptr) {
        Output_FlipScreen();
    } else {
        GFX_Track_Reset();
    }
}

GF_COMMAND PhaseExecutor_Run(PHASE *const phase)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };

    gf_cmd = M_HandleOverride();
    if (gf_cmd.action != GF_NOOP) {
        return gf_cmd;
    }

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

    while (true) {
        int32_t nframes = Clock_WaitTick();
        int32_t frame = 0;
        while (true) {
            const PHASE_CONTROL control = M_Control(phase);
            if (control.action == PHASE_ACTION_END) {
                if (Shell_IsExiting()) {
                    gf_cmd = (GF_COMMAND) { .action = GF_EXIT_GAME };
                } else {
                    gf_cmd = control.gf_cmd;
                }
                goto finish;
            } else if (control.action == PHASE_ACTION_NO_WAIT) {
                continue;
            }

            frame++;
            if (frame >= nframes) {
                break;
            }
        }

        if (Interpolation_IsActive()) {
            Interpolation_SetRate(0.5);
            Output_SetTime(m_CurrentFrame - 0.5f);
            M_Draw(phase);
            Clock_WaitTick();
        }

        Interpolation_SetRate(1.0);
        Output_SetTime(m_CurrentFrame);
        M_Draw(phase);
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

PHASE *PhaseExecutor_GetOuterPhase(void)
{
    if (m_PhaseStackSize < 2) {
        return nullptr;
    }
    return m_PhaseStack[m_PhaseStackSize - 2];
}
