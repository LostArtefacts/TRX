#ifndef T1M_SPECIFIC_MNSOUND_H
#define T1M_SPECIFIC_MNSOUND_H

#include "global/types.h"

#include <stdint.h>

// clang-format off
#define mn_stop_ambient_samples     ((void      (*)())0x0042B000)
#define mn_reset_ambient_loudness   ((void      (*)())0x0042AFD0)
#define mn_update_sound_effects     ((void      (*)())0x0042B080)
#define mn_adjust_master_volume     ((void      (*)(int32_t new_volume))0x0042B410)
#define mn_sound_effect             ((int32_t   (*)(int32_t sfx_num, PHD_3DPOS *pos, uint32_t flags))0x0042AA30)
#define mn_stop_sound_effect        ((void      (*)(int32_t sfx_num, PHD_3DPOS *pos))0x0042B300)
// clang-format on

void mn_reset_sound_effects();
void mn_clear_fx_slot(MN_SFX_PLAY_INFO *slot);

void T1MInjectSpecificMNSound();

#endif
