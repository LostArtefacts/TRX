#include "game/effects/powerup.h"

#include "game/sound.h"
#include "global/vars.h"

// original name: PowerUpFX
void PowerUp(ITEM_INFO *item)
{
    PHD_3DPOS pos;
    if (FlipTimer > FRAMES_PER_SECOND * 4) {
        FlipEffect = -1;
    } else {
        pos.x = Camera.target.x;
        pos.y = Camera.target.y + FlipTimer * 100;
        pos.z = Camera.target.z;
        SoundEffect(SFX_POWERUP_FX, &pos, SPM_NORMAL);
    }
    FlipTimer++;
}
