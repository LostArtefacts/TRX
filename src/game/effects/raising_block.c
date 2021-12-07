#include "game/effects/raising_block.h"

#include "game/sound.h"
#include "global/vars.h"

void RaisingBlock(ITEM_INFO *item)
{
    Sound_Effect(SFX_RAISINGBLOCK_FX, NULL, SPM_NORMAL);
    g_FlipEffect = -1;
}
