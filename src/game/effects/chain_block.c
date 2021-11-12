#include "game/effects/chain_block.h"

#include "config.h"
#include "game/sound.h"
#include "global/vars.h"

void ChainBlock(ITEM_INFO *item)
{
    if (T1MConfig.fix_tihocan_secret_sound) {
        Sound_Effect(SFX_LARA_SPLASH, NULL, SPM_NORMAL);
        FlipEffect = -1;
        return;
    }

    if (FlipTimer == 0) {
        Sound_Effect(SFX_SECRET, NULL, SPM_NORMAL);
    }

    FlipTimer++;
    if (FlipTimer == 55) {
        Sound_Effect(SFX_LARA_SPLASH, NULL, SPM_NORMAL);
        FlipEffect = -1;
    }
}
