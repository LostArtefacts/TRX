#ifndef T1M_GAME_EFFECTS_H
#define T1M_GAME_EFFECTS_H

#include "game/types.h"
#include <stdint.h>

// clang-format off
#define ItemNearLara            ((int32_t       (*)(PHD_3DPOS* pos, int32_t distance))0x0041A210)
#define SoundEffect             ((int32_t       (*)(int32_t sfx_num, PHD_3DPOS *pos, uint32_t flags))0x0042AA30)
#define SoundEffects            ((void          (*)())0x0041A2A0)
#define StopSoundEffect         ((void          (*)(int32_t sfx_num, PHD_3DPOS *pos))0x0042B300)
#define ItemSparkle             ((void          (*)(ITEM_INFO* item, int meshmask))0x0041A550)
#define Richochet               ((void          (*)(GAME_VECTOR* pos))0x0041A450)
#define Splash                  ((void          (*)(ITEM_INFO* item))0x0041A860)
#define DoBloodSplat            ((int16_t       (*)(int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE direction, int16_t room_num))0x0041A310)
// clang-format on

void ControlBubble1(int16_t fx_num);
void FxChainBlock(ITEM_INFO* item);

void T1MInjectGameEffects();

#endif
