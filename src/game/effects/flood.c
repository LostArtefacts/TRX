#include "game/effects/flood.h"

#include "game/sound.h"
#include "global/vars.h"

// original name: FloodFX
void Flood(ITEM_INFO *item)
{
    PHD_3DPOS pos;

    if (FlipTimer > 120) {
        FlipEffect = -1;
    } else {
        pos.x = LaraItem->pos.x;
        if (FlipTimer < 30) {
            pos.y = Camera.target.y + (30 - FlipTimer) * 100;
        } else {
            pos.y = Camera.target.y + (FlipTimer - 30) * 100;
        }
        pos.z = LaraItem->pos.z;
        SoundEffect(SFX_WATERFALL_BIG, &pos, SPM_NORMAL);
    }

    FlipTimer++;
}
