#include "game/effects/flicker.h"

#include "game/control.h"
#include "game/sound.h"
#include "game/vars.h"

// original name: FlickerFX
void Flicker(ITEM_INFO *item)
{
    if (FlipTimer > 125) {
        FlipMap();
        FlipEffect = -1;
    } else if (
        FlipTimer == 90 || FlipTimer == 92 || FlipTimer == 105
        || FlipTimer == 107) {
        FlipMap();
    }
    FlipTimer++;
}
