#include "specific/mnsound.h"

#include "global/vars.h"
#include "specific/sndpc.h"
#include "specific/shed.h"
#include "util.h"

#define MN_NOT_AUDIBLE -1

typedef enum SOUND_FLAGS {
    SOUND_WAIT,
    SOUND_RESTART,
    SOUND_AMBIENT,
} SOUND_FLAGS;

void mn_reset_sound_effects()
{
    TRACE("");
    if (!SoundIsActive) {
        return;
    }
    MnSoundMasterVolume = MnSoundMasterVolumeDefault;
    MnSoundMasterFadeOn = 0;
    MnSoundFadeCounter = 0;

    for (int i = 0; i < MAX_PLAYING_FX; i++) {
        mn_clear_fx_slot(&SFXPlaying[i]);
    }

    S_SoundStopAllSamples();

    MnAmbientLookupIdx = 0;

    for (int i = 0; i < MAX_SAMPLES; i++) {
        if (SampleLUT[i] < 0) {
            continue;
        }
        SAMPLE_INFO *s = &SampleInfos[SampleLUT[i]];
        if (s->volume < 0) {
            sprintf(
                StringToShow,
                "sample info for effect %d has incorrect volume(%d)", i,
                s->volume);
            S_ExitSystem(StringToShow);
        }

        if ((s->flags & 3) == SOUND_AMBIENT) {
            if (MnAmbientLookupIdx >= MAX_AMBIENT_FX) {
                S_ExitSystem("Ran out of ambient fx slots in "
                             "mn_reset_sound_effects()");
            }
            MnAmbientLookup[MnAmbientLookupIdx] = i;
            MnAmbientLookupIdx++;
        }
    }
}

void mn_clear_fx_slot(MN_SFX_PLAY_INFO *slot)
{
    slot->handle = -1;
    slot->pos = NULL;
    slot->mn_flags = 0;
    slot->volume = 0;
    slot->pan = 0;
    slot->loudness = MN_NOT_AUDIBLE;
    slot->fxnum = -1;
}

void T1MInjectSpecificMNSound()
{
    INJECT(0x0042A940, mn_reset_sound_effects);
    INJECT(0x0042AA30, mn_sound_effect);
}
