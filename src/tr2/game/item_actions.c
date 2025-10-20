#include "game/stats.h"

#include <libtrx/game/camera.h>
#include <libtrx/game/collision.h>
#include <libtrx/game/game.h>
#include <libtrx/game/gym.h>
#include <libtrx/game/items/actions/ids.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/random.h>
#include <libtrx/game/sound.h>
#include <libtrx/game/spawn.h>
#include <libtrx/game/viewport.h>
#include <libtrx/utils.h>

typedef void (*M_FUNC)(ITEM *item);

static void M_FlipMap(ITEM *const item)
{
    Room_FlipMap();
}

static void M_AssaultStart(ITEM *const item)
{
    Gym_StartAssault();
    Room_SetFlipEffect(-1);
}

static void M_AssaultStop(ITEM *const item)
{
    Gym_StopAssault();
    Room_SetFlipEffect(-1);
}

static void M_AssaultReset(ITEM *const item)
{
    Gym_ResetAssault();
    Room_SetFlipEffect(-1);
}

static void M_AssaultFinished(ITEM *const item)
{
    Gym_FinishAssault();
    Room_SetFlipEffect(-1);
}

void Item_ActionRunLegacy(ITEM_TRX_ACTION action_id, ITEM *item)
{
    static M_FUNC m_Actions[] = {
        // clang-format off
        [ITEM_ACTION_FLIP_MAP]                     = M_FlipMap,
        [ITEM_ACTION_ASSAULT_RESET]                = M_AssaultReset,
        [ITEM_ACTION_ASSAULT_STOP]                 = M_AssaultStop,
        [ITEM_ACTION_ASSAULT_START]                = M_AssaultStart,
        [ITEM_ACTION_ASSAULT_FINISHED]             = M_AssaultFinished,
        // clang-format on
    };

    if (action_id >= 0 && action_id < ITEM_ACTION_NUMBER_OF
        && m_Actions[action_id] != nullptr) {
        m_Actions[action_id](item);
    }
}
