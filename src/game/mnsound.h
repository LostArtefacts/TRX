#ifndef T1M_GAME_MNSOUND_H
#define T1M_GAME_MNSOUND_H

#include "global/types.h"

#include <stdint.h>

void mn_reset_sound_effects();
int32_t mn_sound_effect(int32_t sfx_num, PHD_3DPOS *pos, uint32_t flags);
MN_SFX_PLAY_INFO *mn_get_fx_slot(
    int32_t sfx_num, uint32_t loudness, PHD_3DPOS *pos, int16_t mode);
void mn_reset_ambient_loudness();
void mn_stop_ambient_samples();
void mn_clear_fx_slot(MN_SFX_PLAY_INFO *slot);
void mn_clear_handles(MN_SFX_PLAY_INFO *slot);
void mn_update_sound_effects();
void mn_get_sound_params(MN_SFX_PLAY_INFO *slot);
void mn_stop_sound_effect(int sfx_num, PHD_3DPOS *pos);
void mn_adjust_master_volume(int8_t volume);
void mn_clear_fx_slot(MN_SFX_PLAY_INFO *slot);
void mn_clear_handles(MN_SFX_PLAY_INFO *slot);

void T1MInjectGameMNSound();

#endif
