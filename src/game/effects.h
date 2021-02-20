#ifndef TOMB1MAIN_GAME_EFFECTS_H
#define TOMB1MAIN_GAME_EFFECTS_H

#include <stdint.h>
#include "game/types.h"

// clang-format off
#define SoundEffect             ((int32_t       __cdecl(*)(int32_t sfx_num, PHD_3DPOS *pos, uint32_t flags))0x0042AA30)
#define SoundEffects            ((void          __cdecl(*)())0x0041A2A0)
#define StopSoundEffect         ((void          __cdecl(*)(int32_t sfx_num, PHD_3DPOS *pos))0x0042B300)
#define ItemSparkle             ((void          __cdecl(*)(ITEM_INFO* item, int meshmask))0x0041A550)
#define Richochet               ((void          __cdecl(*)(GAME_VECTOR* pos))0x0041A450)
#define Splash                  ((void          __cdecl(*)(ITEM_INFO* item))0x0041A860)
#define DoBloodSplat            ((int16_t       __cdecl(*)(int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE direction, int16_t room_num))0x0041A310)
// clang-format on

void __cdecl FxChainBlock(ITEM_INFO* item);

void Tomb1MInjectGameEffects();

#endif
