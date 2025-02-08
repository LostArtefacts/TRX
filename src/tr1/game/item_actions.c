#include "game/item_actions.h"

#include "game/item_actions/bubbles.h"
#include "game/item_actions/chain_block.h"
#include "game/item_actions/earthquake.h"
#include "game/item_actions/explosion.h"
#include "game/item_actions/finish_level.h"
#include "game/item_actions/flicker.h"
#include "game/item_actions/flipmap.h"
#include "game/item_actions/flood.h"
#include "game/item_actions/floor_shake.h"
#include "game/item_actions/lara_effects.h"
#include "game/item_actions/powerup.h"
#include "game/item_actions/raising_block.h"
#include "game/item_actions/sand.h"
#include "game/item_actions/stairs2slope.h"
#include "game/item_actions/turn_180.h"
#include "game/room.h"

typedef void (*M_FUNC)(ITEM *item);

static M_FUNC m_Actions[] = {
    [ITEM_ACTION_TURN_180] = ItemAction_Turn180,
    [ITEM_ACTION_FLOOR_SHAKE] = ItemAction_FloorShake,
    [ITEM_ACTION_LARA_NORMAL] = ItemAction_LaraNormal,
    [ITEM_ACTION_BUBBLES] = ItemAction_Bubbles,
    [ITEM_ACTION_FINISH_LEVEL] = ItemAction_FinishLevel,
    [ITEM_ACTION_EARTHQUAKE] = ItemAction_Earthquake,
    [ITEM_ACTION_FLOOD] = ItemAction_Flood,
    [ITEM_ACTION_RAISING_BLOCK] = ItemAction_RaisingBlock,
    [ITEM_ACTION_STAIRS_TO_SLOPE] = ItemAction_Stairs2Slope,
    [ITEM_ACTION_DROP_SAND] = ItemAction_DropSand,
    [ITEM_ACTION_POWER_UP] = ItemAction_PowerUp,
    [ITEM_ACTION_EXPLOSION] = ItemAction_Explosion,
    [ITEM_ACTION_LARA_HANDS_FREE] = ItemAction_LaraHandsFree,
    [ITEM_ACTION_FLIP_MAP] = ItemAction_FlipMap,
    [ITEM_ACTION_LARA_DRAW_RIGHT_GUN] = ItemAction_LaraDrawRightGun,
    [ITEM_ACTION_CHAIN_BLOCK] = ItemAction_ChainBlock,
    [ITEM_ACTION_FLICKER] = ItemAction_Flicker,
};

void ItemAction_Run(int16_t action_id, ITEM *item)
{
    if (m_Actions[action_id] != nullptr) {
        m_Actions[action_id](item);
    }
}

void ItemAction_RunActive(void)
{
    const int32_t flip_effect = Room_GetFlipEffect();
    if (flip_effect != -1) {
        ItemAction_Run(flip_effect, nullptr);
    }
}
