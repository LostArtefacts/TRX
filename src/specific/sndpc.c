#include "specific/sndpc.h"

#include "game/vars.h"

#include "util.h"

#include <windows.h>

void S_CDVolume(int16_t volume)
{
    TRACE("%d", volume);
    int32_t volume_aux = volume * 0xFFFF / 0xFF;
    volume_aux |= volume_aux << 16;
    auxSetVolume(AuxDeviceID, volume_aux);
}

void T1MInjectSpecificSndPC()
{
    // NOTE: this is a nullsub in OG and is called in many different places
    // for many different purposes so it's not injected.
    // INJECT(0x00437F30, S_CDVolume);
}
