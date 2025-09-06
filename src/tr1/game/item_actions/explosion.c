#include "game/item_actions/explosion.h"

#include <libtrx/game/camera.h>
#include <libtrx/game/rooms.h>
#include <libtrx/game/sound.h>

void ItemAction_Explosion(ITEM *item)
{
    Sound_Effect(SFX_EXPLOSION_FX, nullptr, SPM_NORMAL);
    g_Camera.bounce = -75;
    Room_SetFlipEffect(-1);
}
