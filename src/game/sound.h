#ifndef T1M_GAME_SOUND_H
#define T1M_GAME_SOUND_H

// clang-format off
#define SoundEffect             ((int32_t       (*)(int32_t sfx_num, PHD_3DPOS *pos, uint32_t flags))0x0042AA30)
#define StopSoundEffect         ((void          (*)(int32_t sfx_num, PHD_3DPOS *pos))0x0042B300)
// clang-format on

void SoundEffects();

#endif
