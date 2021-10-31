#include "game/effects/flood.h"

#include "game/sound.h"
#include "global/vars.h"

void Flood(ITEM_INFO *item)
{
    PHD_3DPOS pos;

    if (FlipTimer > FRAMES_PER_SECOND * 4) {
        FlipEffect = -1;
    } else {
        pos.x = LaraItem->pos.x;
        if (FlipTimer < FRAMES_PER_SECOND) {
            pos.y = Camera.target.y + (FRAMES_PER_SECOND - FlipTimer) * 100;
        } else {
            pos.y = Camera.target.y + (FlipTimer - FRAMES_PER_SECOND) * 100;
        }
        pos.z = LaraItem->pos.z;
        SoundEffect(SFX_WATERFALL_BIG, &pos, SPM_NORMAL);
    }

    FlipTimer++;
}
