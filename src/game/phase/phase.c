#include "game/phase/phase.h"

#include "game/clock.h"
#include "game/output.h"
#include "game/phase/phase_cutscene.h"
#include "game/phase/phase_game.h"
#include "game/phase/phase_inventory.h"
#include "game/phase/phase_pause.h"
#include "game/phase/phase_picture.h"
#include "game/phase/phase_stats.h"
#include "global/types.h"
#include "global/vars.h"
#include "log.h"

#include <stdbool.h>
#include <stddef.h>

static PHASE m_Phase = PHASE_NULL;
static PHASER *m_Phaser = NULL;

static bool m_Running = false;
static PHASE m_PhaseToSet = PHASE_NULL;
static void *m_PhaseToSetArg = NULL;

static GAMEFLOW_OPTION Phase_Control(int32_t nframes);
static void Phase_Draw(void);
static int32_t Phase_Wait(void);
static void Phase_SetUnconditionally(const PHASE phase, void *arg);

static GAMEFLOW_OPTION Phase_Control(int32_t nframes)
{
    if (m_Phaser && m_Phaser->control) {
        return m_Phaser->control(nframes);
    }
    return GF_NOP;
}

static void Phase_Draw(void)
{
    if (m_Phaser && m_Phaser->draw) {
        m_Phaser->draw();
    }
}

static void Phase_SetUnconditionally(const PHASE phase, void *arg)
{
    if (m_Phaser && m_Phaser->end) {
        m_Phaser->end();
    }

    LOG_DEBUG("%d", phase);
    switch (phase) {
    case PHASE_NULL:
        m_Phaser = NULL;
        break;

    case PHASE_GAME:
        m_Phaser = &g_GamePhaser;
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
        int32_t nframes = Clock_SyncTicks();
        Output_AnimateFades(nframes);
        g_Camera.number_frames = nframes;
        return nframes;
    }
}

GAMEFLOW_OPTION Phase_Run(void)
{
    int32_t nframes = Clock_SyncTicks();
    GAMEFLOW_OPTION ret = GF_NOP;
    m_Running = true;
    while (1) {
        ret = Phase_Control(nframes);
        Phase_Draw();
        Output_DumpScreen();

        if (m_PhaseToSet != PHASE_NULL) {
            Phase_SetUnconditionally(m_PhaseToSet, m_PhaseToSetArg);
            m_PhaseToSet = PHASE_NULL;
            m_PhaseToSetArg = NULL;
            if (ret != GF_NOP) {
                break;
            }
            nframes = 2;
        } else {
            if (ret != GF_NOP) {
                break;
            }
            nframes = Phase_Wait();
        }
    }

    if (ret == GF_NOP_BREAK) {
        ret = GF_NOP;
    }

    m_Running = false;
    Phase_Set(PHASE_NULL, NULL);

    LOG_DEBUG("Phase_Run() exited with %d", ret);
    return ret;
}
