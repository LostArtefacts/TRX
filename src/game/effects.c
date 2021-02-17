#include "data.h"
#include "effects.h"
#include "mod.h"
#include "util.h"

void __cdecl FxChainBlock(ITEM_INFO* item)
{
    if (TR1MConfig.fix_tihocan_secret_sound) {
        SoundEffect(33, NULL, 0);
        FlipEffect = -1;
    } else {
        if (FlipTimer == 0) {
            SoundEffect(173, NULL, 0);
        }

        FlipTimer++;
        if (FlipTimer == 55) {
            SoundEffect(33, NULL, 0);
            FlipEffect = -1;
        }
    }
}

void TR1MInjectGameEffects()
{
    INJECT(0x0041AD00, FxChainBlock);
}
