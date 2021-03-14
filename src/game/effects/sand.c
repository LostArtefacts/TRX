#include "game/effects/sand.h"
#include "game/sound.h"
#include "game/vars.h"

// original name: SandFX
void DropSand(ITEM_INFO *item)
{
    PHD_3DPOS pos;
    if (FlipTimer > 120) {
        FlipEffect = -1;
    } else {
        if (!FlipTimer) {
            SoundEffect(SFX_TRAPDOOR_OPEN, NULL, SPM_NORMAL);
        }
        pos.x = Camera.target.x;
        pos.y = Camera.target.y + FlipTimer * 100;
        pos.z = Camera.target.z;
        SoundEffect(SFX_SAND_FX, &pos, SPM_NORMAL);
    }
    FlipTimer++;
}
