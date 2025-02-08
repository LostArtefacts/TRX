#include "game/item_actions/chain_block.h"

#include "game/room.h"
#include "game/sound.h"

#include <libtrx/config.h>

void ItemAction_ChainBlock(ITEM *item)
{
    if (g_Config.audio.fix_tihocan_secret_sound) {
        if (g_FlipTimer == 0) {
            Sound_Effect(SFX_CHAINBLOCK_FX, nullptr, SPM_NORMAL);
            g_FlipTimer = 1;
            return;
        }
    }

    if (g_FlipTimer == 0) {
        Sound_Effect(SFX_SECRET, nullptr, SPM_NORMAL);
    }

    g_FlipTimer++;
    if (g_FlipTimer == 55) {
        Sound_Effect(SFX_LARA_SPLASH, nullptr, SPM_NORMAL);
        Room_SetFlipEffect(-1);
    }
}
