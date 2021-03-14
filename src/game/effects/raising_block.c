#include "game/effects/raising_block.h"

#include "game/sound.h"
#include "game/vars.h"

// original name: RaisingBlockFX
void RaisingBlock(ITEM_INFO *item)
{
    SoundEffect(SFX_RAISINGBLOCK_FX, NULL, SPM_NORMAL);
    FlipEffect = -1;
}
