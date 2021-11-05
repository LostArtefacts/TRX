#ifndef T1M_GAME_SOUND_H
#define T1M_GAME_SOUND_H

#include "global/types.h"

#include <stdint.h>

void SoundEffects();
void SoundEffect(int32_t sfx_num, PHD_3DPOS *pos, uint32_t flags);
void StopSoundEffect(int32_t sfx_num, PHD_3DPOS *pos);

#endif
