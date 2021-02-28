#ifndef T1M_GAME_EFFECTS_H
#define T1M_GAME_EFFECTS_H

#include "game/types.h"
#include <stdint.h>

// clang-format off
#define SoundEffect             ((int32_t       (*)(int32_t sfx_num, PHD_3DPOS *pos, uint32_t flags))0x0042AA30)
#define SoundEffects            ((void          (*)())0x0041A2A0)
#define StopSoundEffect         ((void          (*)(int32_t sfx_num, PHD_3DPOS *pos))0x0042B300)
// clang-format on

int32_t ItemNearLara(PHD_3DPOS* pos, int32_t distance);
int16_t DoBloodSplat(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t direction,
    int16_t room_num);
void ControlBlood1(int16_t fx_num);
void ControlExplosion1(int16_t fx_num);
void Richochet(GAME_VECTOR* pos);
void ControlRicochet1(int16_t fx_num);
void Twinkle(GAME_VECTOR* pos);
void ControlTwinkle(int16_t fx_num);
void ItemSparkle(ITEM_INFO* item, int meshmask);
void FxLaraBubbles(ITEM_INFO* item);
void ControlBubble1(int16_t fx_num);
void Splash(ITEM_INFO* item);
void ControlSplash1(int16_t fx_num);
void ControlWaterFall(int16_t item_num);
void FxFinishLevel(ITEM_INFO* item);
void FxTurn180(ITEM_INFO* item);
void FxDinoStomp(ITEM_INFO* item);
void FxLaraNormal(ITEM_INFO* item);
void FxEarthQuake(ITEM_INFO* item);
void FxFlood(ITEM_INFO* item);
void FxRaisingBlock(ITEM_INFO* item);
void FxChainBlock(ITEM_INFO* item);
void FxStairs2Slope(ITEM_INFO* item);
void FxSand(ITEM_INFO* item);
void FxPowerUp(ITEM_INFO* item);
void FxExplosion(ITEM_INFO* item);
void FxFlicker(ITEM_INFO* item);
void FxLaraHandsFree(ITEM_INFO* item);

void T1MInjectGameEffects();

#endif
