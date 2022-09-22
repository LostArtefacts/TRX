#include "game/effect_routines/explosion.h"

#include "game/room.h"
#include "game/sound.h"
#include "global/vars.h"

void FX_Explosion(ITEM_INFO *item)
{
    Sound_Effect(SFX_EXPLOSION_FX, NULL, SPM_NORMAL);
    g_Camera.bounce = -75;
    g_FlipEffect = -1;
}
