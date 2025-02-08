#include "game/item_actions/explosion.h"

#include "game/camera.h"
#include "game/room.h"
#include "game/sound.h"

void ItemAction_Explosion(ITEM *item)
{
    Sound_Effect(SFX_EXPLOSION_FX, nullptr, SPM_NORMAL);
    g_Camera.bounce = -75;
    Room_SetFlipEffect(-1);
}
