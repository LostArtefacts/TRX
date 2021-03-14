#include "specific/sndpc.h"

#include "game/sound.h"
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

void S_CDPlay(int16_t track)
{
    if (!OptionMusicVolume) {
        return;
    }

    if (track == 13) {
        SoundEffect(SFX_SECRET, NULL, SPM_ALWAYS);
    } else if (track <= 2 || track >= 22) {
        if (CDTrack > 0) {
            if (CDTrack >= 26 && CDTrack <= 56) {
                StopSoundEffect(SFX_GYM_HINT_01 + CDTrack - 26, NULL);
            } else {
                CDStop();
            }
            CDTrack = 0;
        }
        if (track >= 26 && track <= 56) {
            SoundEffect(SFX_GYM_HINT_01 + track - 26, NULL, SPM_ALWAYS);
            CDTrack = track;
        } else if (track == 2) {
            CDPlay(2);
            CDTrack = 2;
        } else if (track >= 22 && track <= 25) {
            CDPlay(track - 15);
            CDTrack = track;
        } else {
            if (track > 56) {
                CDPlay(track - 54);
            }
            CDTrack = track;
        }
    }
}

void T1MInjectSpecificSndPC()
{
    INJECT(0x00438D40, S_CDPlay);
    // NOTE: this is a nullsub in OG and is called in many different places
    // for many different purposes so it's not injected.
    // INJECT(0x00437F30, S_CDVolume);
}
