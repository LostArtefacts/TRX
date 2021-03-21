#include "game/sound.h"

#include "global/vars.h"
#include "specific/mnsound.h"

#include <stddef.h>

#define FLIPFLAG 0x40
#define UNFLIPFLAG 0x80

void SoundEffects()
{
    mn_reset_ambient_loudness();

    for (int i = 0; i < NumberSoundEffects; i++) {
        OBJECT_VECTOR *sound = &SoundEffectsTable[i];
        if (FlipStatus && (sound->flags & FLIPFLAG)) {
            SoundEffect(sound->data, (PHD_3DPOS *)sound, SPM_NORMAL);
        } else if (!FlipStatus && (sound->flags & UNFLIPFLAG)) {
            SoundEffect(sound->data, (PHD_3DPOS *)sound, SPM_NORMAL);
        }
    }

    // XXX: Why are we firing this here? Some of the FX routines rely on the
    // item to be not null!
    if (FlipEffect != -1) {
        EffectRoutines[FlipEffect](NULL);
    }

    mn_update_sound_effects();
}

int32_t SoundEffect(int32_t sfx_num, PHD_3DPOS *pos, uint32_t flags)
{
    return mn_sound_effect(sfx_num, pos, flags);
}

void StopSoundEffect(int32_t sfx_num, PHD_3DPOS *pos)
{
    mn_stop_sound_effect(sfx_num, pos);
}
