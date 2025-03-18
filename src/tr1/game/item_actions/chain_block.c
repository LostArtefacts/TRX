#include "game/item_actions/chain_block.h"

#include "game/room.h"
#include "game/sound.h"

#include <libtrx/config.h>

void ItemAction_ChainBlock(ITEM *item)
{
    const int32_t flip_timer = Room_GetFlipTimer();
    if (g_Config.audio.fix_tihocan_secret_sound) {
        if (flip_timer == 0) {
            Sound_Effect(SFX_CHAINBLOCK_FX, nullptr, SPM_NORMAL);
            Room_SetFlipTimer(1);
            return;
        }
    }

    if (flip_timer == 0) {
        Sound_Effect(SFX_SECRET, nullptr, SPM_NORMAL);
    }

    if (flip_timer == 54) {
        Sound_Effect(SFX_LARA_SPLASH, nullptr, SPM_NORMAL);
        Room_SetFlipEffect(-1);
    }
    Room_IncrementFlipTimer(1);
}
