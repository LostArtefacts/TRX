#include "game/item_actions/raising_block.h"

#include "game/room.h"
#include "game/sound.h"

void ItemAction_RaisingBlock(ITEM *item)
{
    Sound_Effect(SFX_RAISINGBLOCK_FX, nullptr, SPM_NORMAL);
    Room_SetFlipEffect(-1);
}
