#include "game/effects/chain_block.h"

#include "config.h"
#include "game/sound.h"
#include "global/vars.h"

void ChainBlock(ITEM_INFO *item)
{
    if (T1MConfig.fix_tihocan_secret_sound) {
        SoundEffect(SFX_LARA_SPLASH, NULL, SPM_NORMAL);
        FlipEffect = -1;
        return;
    }

    if (FlipTimer == 0) {
        SoundEffect(SFX_SECRET, NULL, SPM_NORMAL);
    }

    FlipTimer++;
    if (FlipTimer == 55) {
        SoundEffect(SFX_LARA_SPLASH, NULL, SPM_NORMAL);
        FlipEffect = -1;
    }
}
