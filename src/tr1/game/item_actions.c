#include "game/item_actions/finish_level.h"
#include "game/item_actions/flipmap.h"

#include <libtrx/game/items.h>
#include <libtrx/game/rooms.h>

typedef void (*M_FUNC)(ITEM *item);

void Item_ActionRunLegacy(ITEM_TRX_ACTION action_id, ITEM *item)
{
    static M_FUNC m_Actions[] = {
        [ITEM_ACTION_FINISH_LEVEL] = ItemAction_FinishLevel,
        [ITEM_ACTION_FLIP_MAP] = ItemAction_FlipMap,
    };

    if (action_id >= 0 && action_id < ITEM_ACTION_NUMBER_OF
        && m_Actions[action_id] != nullptr) {
        m_Actions[action_id](item);
    }
}
