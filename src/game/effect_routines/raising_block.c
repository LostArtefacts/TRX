#include "game/effect_routines/raising_block.h"

#include "game/room.h"
#include "game/sound.h"

#include <stddef.h>

void FX_RaisingBlock(ITEM_INFO *item)
{
    Sound_Effect(SFX_RAISINGBLOCK_FX, NULL, SPM_NORMAL);
    g_FlipEffect = -1;
}
