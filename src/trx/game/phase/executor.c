#include <trx/game/phase/executor.h>

#include <trx/config.h>
#include <trx/core/benchmark.h>
#include <trx/core/strings.h>
#include <trx/game/clock.h>
#include <trx/game/console/common.h>
#include <trx/game/fader.h>
#include <trx/game/fmv.h>
#include <trx/game/game.h>
#include <trx/game/game_flow.h>
#include <trx/game/input.h>
#include <trx/game/interpolation.h>
#include <trx/game/lua/events.h>
#include <trx/game/lua/events/ui.h>
#include <trx/game/music.h>
#include <trx/game/output.h>
#include <trx/game/output/overlay.h>
#include <trx/game/overlay.h>
#include <trx/game/savegame.h>
#include <trx/game/shell.h>
#include <trx/game/ui.h>
#include <trx/game/ui/dialogs/takeover.h>
#include <trx/game/ui/touch_overlay.h>
#include <trx/gl/context.h>
#include <trx/gl/gpu_timer.h>
#include <trx/gl/track.h>

#include <SDL2/SDL_timer.h>

#define M_MAX_PHASES 10

static int32_t m_CurrentFrame = 0;
static bool m_Exiting;
static FADER m_ExitFader;
static int32_t m_PhaseStackSize = 0;
static PHASE *m_PhaseStack[M_MAX_PHASES] = {};
static bool m_PendingFadeToBlack = false;
static FADER_ARGS m_PendingFadeToBlackArgs;

static double m_LastFlipMs = 0.0;

static GF_COMMAND M_HandleOverride(const PHASE *const phase)
{
    const GF_COMMAND gf_override_cmd = GF_GetOverrideCommand();
    if (!GF_IsOverrideImmediate() && phase != nullptr
        && phase->defers_override) {
        // Defer the command until the phase finishes.
        return (GF_COMMAND) { .action = GF_NOOP };
    }
    if (gf_override_cmd.action == GF_START_FMV) {
        GF_OverrideCommand((GF_COMMAND) { .action = GF_NOOP }, true);
        // Let movies return to the current phase.
        const GF_FMV *const fmv = GF_GetFMV(gf_override_cmd.param);
        if (fmv != nullptr) {
            SHOULD(FMV_Play(fmv->path));
        }
        return (GF_COMMAND) { .action = GF_NOOP };
    }
    if (gf_override_cmd.action != GF_NOOP) {
        const GF_COMMAND gf_cmd = gf_override_cmd;
        GF_OverrideCommand((GF_COMMAND) { .action = GF_NOOP }, true);

        // Stop the current save and music before changing game flow. A level
        // select keeps the save bound by the code that raised it, because
        // the save decides what Lara carries into the level.
        if (gf_cmd.action != GF_SELECT_GAME) {
            SG_Manager_UnbindSlot();
        }
        Game_SetIsPlaying(false);
        Music_Stop();

        return gf_cmd;
    }
    return (GF_COMMAND) { .action = GF_NOOP };
}

static void M_DrawFadeToBlackTransition(const float opacity)
{
    Output_BeginScene();
    Output_SwitchViewport(VIEWPORT_GAME);
    UI_BeginScene();

    Output_Overlay_DrawSnapshot(1.0f);
    Output_Overlay_DrawBlackRectangle(opacity, false);

    LUA_UI_DrawRegions();
    Overlay_DrawUI();
    TouchOverlay_Draw();
    UI_EndScene();

    Output_SwitchViewport(VIEWPORT_UI);
    UI_Draw();

    Output_Flush();
    Output_Overlay_DrawBlackRectangle(
        Fader_GetCurrentValue(&m_ExitFader), true);
    Output_EndScene();

    if (!Output_IsHeadless()
        || TRX_GL_Context_GetScheduledScreenshotPath() != nullptr) {
        Output_FlipScreen();
    } else {
        TRX_GL_Track_Reset();
    }
}

static GF_COMMAND M_RunFadeToBlackTransition(
    const PHASE *const phase, const FADER_ARGS args)
{
    Output_Overlay_CaptureSnapshot();

    FADER fader = {};
    Fader_InitToHold(&fader, 0.0f, 1.0f, args.duration, args.debuff);
    while (Fader_IsActive(&fader)) {
        Clock_WaitTick();
        m_CurrentFrame++;

        Shell_ProcessEvents();
        Overlay_Control();

        const GF_COMMAND gf_cmd = M_HandleOverride(phase);
        if (gf_cmd.action == GF_EXIT_GAME) {
            PhaseExecutor_BeginExit();
        } else if (gf_cmd.action != GF_NOOP) {
            return gf_cmd;
        }

        if (Shell_IsExiting()) {
            PhaseExecutor_BeginExit();
        }
        if (m_Exiting && !Fader_IsActive(&m_ExitFader)) {
            return (GF_COMMAND) { .action = GF_EXIT_GAME };
        }

        LUA_FireEvent(LUA_EVENT_TICK);

        Interpolation_SetRate(1.0f);
        Output_SetTime(m_CurrentFrame);
        M_DrawFadeToBlackTransition(Fader_GetCurrentValue(&fader));
    }

    return (GF_COMMAND) { .action = GF_NOOP };
}

static PHASE_CONTROL M_Control(PHASE *const phase)
{
    m_CurrentFrame++;
    Shell_ProcessEvents();
    Overlay_Control();

    const GF_COMMAND gf_cmd = M_HandleOverride(phase);
    if (gf_cmd.action == GF_EXIT_GAME) {
        PhaseExecutor_BeginExit();
    } else if (gf_cmd.action != GF_NOOP) {
        return (PHASE_CONTROL) {
            .action = PHASE_ACTION_END_FAST,
            .gf_cmd = gf_cmd,
        };
    }

    if (Shell_IsExiting()) {
        PhaseExecutor_BeginExit();
    }
    if (m_Exiting) {
        if (!Fader_IsActive(&m_ExitFader)) {
            return (PHASE_CONTROL) {
                .action = PHASE_ACTION_END,
                .gf_cmd = { .action = GF_EXIT_GAME },
            };
        }
        return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
    }

    if (Shell_ShouldPauseForFocusLoss()) {
        return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
    }

    // Publish signals after phase control updates the frame state.
    if (phase != nullptr && phase->control != nullptr) {
        Output_DropPendingFog();
        const bool was_closed = UI_Takeover_IsAnyClosed();
        const PHASE_CONTROL control = phase->control(phase);
        LUA_FireEvent(LUA_EVENT_TICK);
        // Runs the phase again before the next draw when a script closes a
        // screen during the tick. Otherwise one frame shows the screen
        // without the script's layers.
        if (control.action == PHASE_ACTION_CONTINUE && !was_closed
            && UI_Takeover_IsAnyClosed()) {
            return (PHASE_CONTROL) { .action = PHASE_ACTION_NO_WAIT };
        }
        return control;
    }

    LUA_FireEvent(LUA_EVENT_TICK);
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
    if (phase != nullptr && phase->draw != nullptr) {
        phase->draw(phase);
    }

    LUA_UI_DrawRegions();
    Overlay_DrawUI();
    TouchOverlay_Draw();
    UI_EndScene();

    Output_SwitchViewport(VIEWPORT_UI);
    UI_Draw();

    Output_Flush();
    Output_Overlay_DrawBlackRectangle(
        Fader_GetCurrentValue(&m_ExitFader), true);
    Output_EndScene();

    TRX_GL_GpuTimer_Collect();
    if (Shell_GetArgs()->debug_render_performance) {
        const TRX_GL_METRICS metrics = TRX_GL_Track_GetMetrics();
        Benchmark_End(
            &benchmark,
            String_FormatStatic(
                "%.03f KB T:%d U:%d Vo:%d Vt:%d Vb:%d Do:%d Dt:%d Db:%d S:%d "
                "I:%d F:%.03f | gpu bg:%.02f op:%.02f tr:%.02f sub:%.02f "
                "add:%.02f ui:%.02f",
                metrics.buffer_total_bytes / 1024.0f,
                metrics.buffer_transfer_count, metrics.uniform_changes,
                metrics.opaque_vert_count, metrics.trans_vert_count,
                metrics.blend_add_vert_count, metrics.opaque_draw_count,
                metrics.trans_draw_count, metrics.blend_add_draw_count,
                metrics.trans_sort_count, metrics.staged_count, m_LastFlipMs,
                TRX_GL_GpuTimer_GetMs(SCENE_PASS_BACKGROUND),
                TRX_GL_GpuTimer_GetMs(SCENE_PASS_OPAQUE),
                TRX_GL_GpuTimer_GetMs(SCENE_PASS_TRANSPARENT),
                TRX_GL_GpuTimer_GetMs(SCENE_PASS_BLEND_SUB),
                TRX_GL_GpuTimer_GetMs(SCENE_PASS_BLEND_ADD),
                TRX_GL_GpuTimer_GetMs(SCENE_PASS_UI)));
    }

    if (!Output_IsHeadless()
        || TRX_GL_Context_GetScheduledScreenshotPath() != nullptr) {
        const Uint64 flip_start = SDL_GetPerformanceCounter();
        Output_FlipScreen();
        m_LastFlipMs = (double)(SDL_GetPerformanceCounter() - flip_start)
            * 1000.0 / (double)SDL_GetPerformanceFrequency();
    } else {
        m_LastFlipMs = 0.0;
        TRX_GL_Track_Reset();
    }
}

GF_COMMAND PhaseExecutor_Run(PHASE *const phase)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };
    bool skip_fade_out = false;

    gf_cmd = M_HandleOverride(phase);
    if (gf_cmd.action != GF_NOOP) {
        return gf_cmd;
    }

    PHASE *const prev_phase =
        m_PhaseStackSize > 0 ? m_PhaseStack[m_PhaseStackSize - 1] : nullptr;
    if (prev_phase != nullptr && prev_phase->suspend != nullptr) {
        prev_phase->suspend(prev_phase);
    }
    m_PhaseStack[m_PhaseStackSize++] = phase;

    if (m_PendingFadeToBlack) {
        const bool uses_cross_fade_in = phase != nullptr
            && phase->uses_cross_fade_in != nullptr
            && phase->uses_cross_fade_in(phase);
        if (!uses_cross_fade_in) {
            gf_cmd =
                M_RunFadeToBlackTransition(phase, m_PendingFadeToBlackArgs);
            if (gf_cmd.action != GF_NOOP) {
                goto finish;
            }
        }
        m_PendingFadeToBlack = false;
    }

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
        } else if (control.action == PHASE_ACTION_END_FAST) {
            gf_cmd = control.gf_cmd;
            skip_fade_out = true;
            goto finish;
        }
    }

    while (true) {
        int32_t nframes = Clock_WaitTick();
        int32_t frame = 0;
        while (true) {
            const PHASE_CONTROL control = M_Control(phase);
            if (control.action == PHASE_ACTION_END
                || control.action == PHASE_ACTION_END_FAST) {
                if (Shell_IsExiting()) {
                    gf_cmd = (GF_COMMAND) { .action = GF_EXIT_GAME };
                } else {
                    skip_fade_out = control.action == PHASE_ACTION_END_FAST;
                    gf_cmd = control.gf_cmd;
                }
                if (gf_cmd.action != GF_EXIT_GAME
                    || !PhaseExecutor_BeginExit()) {
                    goto finish;
                }
            } else if (control.action == PHASE_ACTION_NO_WAIT) {
                continue;
            }

            frame++;
            if (frame >= nframes) {
                break;
            }
        }

        if (!Shell_ShouldPauseForFocusLoss() && !m_Exiting
            && Interpolation_IsActive()) {
            Interpolation_SetRate(0.5);
            Output_SetTime(m_CurrentFrame - 0.5f);
            M_Draw(phase);
            Clock_WaitTick();
        }

        Interpolation_SetRate(1.0);
        Output_SetTime(m_CurrentFrame);
        Output_SetControlFrame(true);
        M_Draw(phase);
        Output_SetControlFrame(false);
    }

finish:
    if (phase->end != nullptr) {
        phase->end(phase);
    }

    if (!skip_fade_out && phase->request_fade_to_black != nullptr) {
        m_PendingFadeToBlack =
            phase->request_fade_to_black(phase, &m_PendingFadeToBlackArgs);
    } else {
        m_PendingFadeToBlack = false;
    }

    if (prev_phase != nullptr && prev_phase->resume != nullptr) {
        Clock_SyncTick();
        prev_phase->resume(prev_phase);
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

bool PhaseExecutor_BeginExit(void)
{
    if (!m_Exiting) {
        m_Exiting = true;
        if (g_Config.visuals.enable_exit_fade_effects) {
            Fader_InitFromCurrentHold(&m_ExitFader, 1.0f, 0.333f, 0.1f);
        }
    }
    return Fader_IsActive(&m_ExitFader);
}

float PhaseExecutor_GetExitFadeOpacity(void)
{
    return Fader_GetCurrentValue(&m_ExitFader);
}
