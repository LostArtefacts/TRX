#include "game/vars.h"
#include "specific/sndpc.h"
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
    INJECT(0x00437F30, S_CDVolume);
}
