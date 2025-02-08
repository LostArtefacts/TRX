#include "game/item_actions/chain_block.h"

#include "game/room.h"
#include "game/sound.h"

#include <libtrx/config.h>

void ItemAction_ChainBlock(ITEM *item)
{
    if (g_Config.audio.fix_tihocan_secret_sound) {
        if (Room_GetFlipTimer() == 0) {
            Sound_Effect(SFX_CHAINBLOCK_FX, nullptr, SPM_NORMAL);
            Room_SetFlipTimer(1);
            return;
        }
    }

    int32_t flip_timer = Room_GetFlipTimer();
    if (flip_timer == 0) {
        Sound_Effect(SFX_SECRET, nullptr, SPM_NORMAL);
    }

    Room_SetFlipTimer(++flip_timer);
    if (flip_timer == 55) {
        Sound_Effect(SFX_LARA_SPLASH, nullptr, SPM_NORMAL);
        Room_SetFlipEffect(-1);
    }
}
