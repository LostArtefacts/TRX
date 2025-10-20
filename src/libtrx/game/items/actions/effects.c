#include "config.h"
#include "game/camera.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/rooms.h"
#include "game/sound.h"
#include "version.h"

static void M_ChainBlock(ITEM *const item)
{
    const int32_t flip_timer = Room_GetFlipTimer();
    if (g_Config.audio.fix_chainblock_secret_sound) {
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

static void M_Flood(ITEM *const item)
{
    const int32_t flip_timer = Room_GetFlipTimer();
    if (flip_timer > 4 * LOGIC_FPS) {
        Room_SetFlipEffect(-1);
        Room_IncrementFlipTimer(1);
        return;
    }

    const ITEM *const lara_item = Lara_GetItem();
    XYZ_32 pos = {
        .x = lara_item->pos.x,
        .y = g_Camera.target.pos.y,
        .z = lara_item->pos.z,
    };
    if (flip_timer >= LOGIC_FPS) {
        pos.y += 100 * (flip_timer - LOGIC_FPS);
    } else {
        pos.y += 100 * (LOGIC_FPS - flip_timer);
    }

    // TODO: unify
    Sound_Effect(
        g_TRVersion == 1 ? SFX_WATERFALL_BIG : SFX_WATERFALL_LOOP, &pos,
        SPM_NORMAL);
    Room_IncrementFlipTimer(1);
}

REGISTER_ITEM_ACTION(ITEM_ACTION_CHAIN_BLOCK, M_ChainBlock)
REGISTER_ITEM_ACTION(ITEM_ACTION_FLOOD, M_Flood)
