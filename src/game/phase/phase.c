#include "game/phase/phase.h"

#include "game/clock.h"
#include "game/interpolation.h"
#include "game/output.h"
#include "game/phase/phase_cutscene.h"
#include "game/phase/phase_demo.h"
#include "game/phase/phase_game.h"
#include "game/phase/phase_inventory.h"
#include "game/phase/phase_pause.h"
#include "game/phase/phase_picture.h"
#include "game/phase/phase_stats.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/log.h>

#include <stdbool.h>
#include <stddef.h>

static PHASE m_Phase = PHASE_NULL;
static PHASER *m_Phaser = NULL;

static bool m_Running = false;
static PHASE m_PhaseToSet = PHASE_NULL;
static void *m_PhaseToSetArg = NULL;

static GAMEFLOW_COMMAND Phase_Control(int32_t nframes);
static void Phase_Draw(void);
static int32_t Phase_Wait(void);
static void Phase_SetUnconditionally(const PHASE phase, void *arg);

static GAMEFLOW_COMMAND Phase_Control(int32_t nframes)
{
    if (g_GameInfo.override_gf_command.action != GF_PHASE_CONTINUE) {
        const GAMEFLOW_COMMAND override = g_GameInfo.override_gf_command;
        g_GameInfo.override_gf_command =
            (GAMEFLOW_COMMAND) { .action = GF_PHASE_CONTINUE };
        return override;
    }

    if (m_Phaser && m_Phaser->control) {
        return m_Phaser->control(nframes);
    }
    return (GAMEFLOW_COMMAND) { .action = GF_PHASE_CONTINUE };
}

static void Phase_Draw(void)
{
    Output_BeginScene();
    if (m_Phaser && m_Phaser->draw) {
        m_Phaser->draw();
    }
    Output_EndScene();
}

static void Phase_SetUnconditionally(const PHASE phase, void *arg)
{
    if (m_Phaser && m_Phaser->end) {
        m_Phaser->end();
    }

    LOG_DEBUG("phase=%d", phase);
    switch (phase) {
    case PHASE_NULL:
        m_Phaser = NULL;
        break;

    case PHASE_GAME:
        m_Phaser = &g_GamePhaser;
        break;

    case PHASE_DEMO:
        m_Phaser = &g_DemoPhaser;
        break;

    case PHASE_CUTSCENE:
        m_Phaser = &g_CutscenePhaser;
        break;

    case PHASE_PAUSE:
        m_Phaser = &g_PausePhaser;
        break;

    case PHASE_PICTURE:
        m_Phaser = &g_PicturePhaser;
        break;

    case PHASE_STATS:
        m_Phaser = &g_StatsPhaser;
        break;

    case PHASE_INVENTORY:
        m_Phaser = &g_InventoryPhaser;
        break;
    }

    if (m_Phaser && m_Phaser->start) {
        m_Phaser->start(arg);
    }

    // set it at the end, so that the start callbacks can retrieve the old phase
    m_Phase = phase;

    Clock_SyncTicks();
}

PHASE Phase_Get(void)
{
    return m_Phase;
}

void Phase_Set(const PHASE phase, void *arg)
{
    // changing the phase in the middle of rendering is asking for trouble,
    // so instead we schedule to run the change on the next iteration
    if (m_Running) {
        m_PhaseToSet = phase;
        m_PhaseToSetArg = arg;
        return;
    }
    Phase_SetUnconditionally(phase, arg);
}

static int32_t Phase_Wait(void)
{
    if (m_Phaser && m_Phaser->wait) {
        return m_Phaser->wait();
    } else {
        return Clock_SyncTicks();
    }
}

GAMEFLOW_COMMAND Phase_Run(void)
{
    int32_t nframes = Clock_SyncTicks();
    GAMEFLOW_COMMAND command = { .action = GF_PHASE_CONTINUE };
    m_Running = true;
    LOG_DEBUG("phase start, phase=%d", m_Phase);

    while (1) {
        command = Phase_Control(nframes);

        if (m_PhaseToSet != PHASE_NULL) {
            Interpolation_SetRate(1.0);
            Phase_Draw();

            Phase_SetUnconditionally(m_PhaseToSet, m_PhaseToSetArg);
            m_PhaseToSet = PHASE_NULL;
            m_PhaseToSetArg = NULL;
            if (command.action != GF_PHASE_CONTINUE) {
                Phase_Draw();
                break;
            }
            nframes = 2;
            // immediately advance to the next logic frame without any wait
            continue;
        }

        if (command.action != GF_PHASE_CONTINUE) {
            Phase_Draw();
            break;
        }

        if (Interpolation_IsEnabled()) {
            Interpolation_SetRate(0.5);
            Phase_Draw();
            Phase_Wait();
        }

        Interpolation_SetRate(1.0);
        Phase_Draw();
        nframes = Phase_Wait();
    }

    if (command.action == GF_PHASE_BREAK) {
        command.action = GF_PHASE_CONTINUE;
    }

    m_Running = false;
    Phase_Set(PHASE_NULL, NULL);

    LOG_DEBUG("phase end, action=%d, param=%d", command.action, command.param);
    return command;
}
