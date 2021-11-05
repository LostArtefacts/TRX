#include "game/sound.h"

#include "game/mnsound.h"
#include "global/vars.h"

#include <stddef.h>

#define FLIPFLAG 0x40
#define UNFLIPFLAG 0x80

void Sound_UpdateEffects()
{
    mn_reset_ambient_loudness();

    for (int i = 0; i < NumberSoundEffects; i++) {
        OBJECT_VECTOR *sound = &SoundEffectsTable[i];
        if (FlipStatus && (sound->flags & FLIPFLAG)) {
            Sound_Effect(sound->data, (PHD_3DPOS *)sound, SPM_NORMAL);
        } else if (!FlipStatus && (sound->flags & UNFLIPFLAG)) {
            Sound_Effect(sound->data, (PHD_3DPOS *)sound, SPM_NORMAL);
        }
    }

    // XXX: Why are we firing this here? Some of the FX routines rely on the
    // item to be not null!
    if (FlipEffect != -1) {
        EffectRoutines[FlipEffect](NULL);
    }

    mn_update_sound_effects();
}

void Sound_Effect(int32_t sfx_num, PHD_3DPOS *pos, uint32_t flags)
{
    mn_sound_effect(sfx_num, pos, flags);
}

void Sound_StopEffect(int32_t sfx_num, PHD_3DPOS *pos)
{
    mn_stop_sound_effect(sfx_num, pos);
}
