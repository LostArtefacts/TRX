#include "game/effects.h"
#include "game/vars.h"
#include "config.h"
#include "util.h"

void __cdecl FxChainBlock(ITEM_INFO* item)
{
#ifdef TOMB1M_FEAT_LEVEL_FIXES
    if (T1MConfig.fix_tihocan_secret_sound) {
        SoundEffect(33, NULL, 0);
        FlipEffect = -1;
    } else {
#else
    {
#endif
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

void Tomb1MInjectGameEffects()
{
    INJECT(0x0041AD00, FxChainBlock);
}
