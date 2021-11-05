#ifndef T1M_GAME_MNSOUND_H
#define T1M_GAME_MNSOUND_H

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

void mn_reset_sound_effects();
bool mn_sound_effect(int32_t sfx_num, PHD_3DPOS *pos, uint32_t flags);
void mn_reset_ambient_loudness();
void mn_stop_ambient_samples();
void mn_update_sound_effects();
void mn_stop_sound_effect(int sfx_num, PHD_3DPOS *pos);
void mn_adjust_master_volume(int8_t volume);

#endif
