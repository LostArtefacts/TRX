#include "game/savegame.h"
#include "game/stats.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/game.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/lua.h>
#include <libtrx/game/music.h>
#include <libtrx/game/option/passport.h>
#include <libtrx/game/output.h>

static DECLARE_GF_EVENT_HANDLER(M_HandleSetStartAnim);

static DECLARE_GF_EVENT_HANDLER((*m_EventHandlers[GFS_NUMBER_OF])) = {
    // clang-format off
    [GFS_SET_START_ANIM]   = M_HandleSetStartAnim,
    // clang-format on
};

static DECLARE_GF_EVENT_HANDLER(M_HandleSetStartAnim)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };
    if (seq_ctx != GFSC_STORY) {
        Lara_SetStartAnimState((LARA_EXTRA_STATE)(intptr_t)event->data);
    }
    return gf_cmd;
}

void GF_PreSequenceHook(
    const GF_SEQUENCE_CONTEXT seq_ctx, void *const seq_ctx_arg)
{
    Room_SetAbyssHeight(0);
    Output_SetSunsetEnabled(false);
    Lara_SetControllable(false);
    Lara_SetStartAnimState(LS_EXTRA_BREATH);
    if (seq_ctx == GFSC_SAVED) {
        Game_SetBonusFlag(GBF_NONE);
    }
}

GF_SEQUENCE_CONTEXT GF_SwitchSequenceContext(
    const GF_SEQUENCE_EVENT *const event, const GF_SEQUENCE_CONTEXT seq_ctx)
{
    // Update sequence context if necessary
    if (event->type != GFS_LOOP_GAME) {
        return seq_ctx;
    }
    switch (seq_ctx) {
    case GFSC_SAVED:
    case GFSC_RESTART:
    case GFSC_SELECT:
        return GFSC_NORMAL;
    default:
        return seq_ctx;
    }
}

void GF_InitSequencer(void)
{
    for (GF_SEQUENCE_EVENT_TYPE event_type = 0; event_type < GFS_NUMBER_OF;
         event_type++) {
        if (m_EventHandlers[event_type] != nullptr) {
            GF_SetSequenceEventHandler(event_type, m_EventHandlers[event_type]);
        }
    }
}
