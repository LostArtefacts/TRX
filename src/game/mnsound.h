#ifndef T1M_GAME_MNSOUND_H
#define T1M_GAME_MNSOUND_H

#include "global/types.h"

#include <stdint.h>

// clang-format off
#define mn_update_sound_effects     ((void      (*)())0x0042B080)
#define mn_adjust_master_volume     ((void      (*)(int32_t new_volume))0x0042B410)
#define mn_stop_sound_effect        ((void      (*)(int32_t sfx_num, PHD_3DPOS *pos))0x0042B300)
// clang-format on

void mn_reset_sound_effects();
int32_t mn_sound_effect(int32_t sfx_num, PHD_3DPOS *pos, uint32_t flags);
MN_SFX_PLAY_INFO *mn_get_fx_slot(
    int32_t sfx_num, uint32_t loudness, PHD_3DPOS *pos, int16_t mode);
void mn_reset_ambient_loudness();
void mn_stop_ambient_samples();
void mn_clear_fx_slot(MN_SFX_PLAY_INFO *slot);
void mn_clear_handles(MN_SFX_PLAY_INFO *slot);

void T1MInjectGameMNSound();

#endif
