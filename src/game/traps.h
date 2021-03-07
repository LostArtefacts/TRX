#ifndef T1M_GAME_TRAPS_H
#define T1M_GAME_TRAPS_H

#include "game/types.h"
#include <stdint.h>

void InitialiseRollingBall(int16_t item_num);
void RollingBallControl(int16_t item_num);
void RollingBallCollision(
    int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll);
void SpikeCollision(int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll);
void TrapDoorControl(int16_t item_num);
void TrapDoorFloor(
    ITEM_INFO* item, int32_t x, int32_t y, int32_t z, int16_t* height);
void TrapDoorCeiling(
    ITEM_INFO* item, int32_t x, int32_t y, int32_t z, int16_t* height);
int32_t OnTrapDoor(ITEM_INFO* item, int32_t x, int32_t z);
void PendulumControl(int16_t item_num);
void FallingBlockControl(int16_t item_num);
void FallingBlockFloor(
    ITEM_INFO* item, int32_t x, int32_t y, int32_t z, int16_t* height);
void FallingBlockCeiling(
    ITEM_INFO* item, int32_t x, int32_t y, int32_t z, int16_t* height);
void TeethTrapControl(int16_t item_num);
void FallingCeilingControl(int16_t item_num);
void InitialiseDamoclesSword(int16_t item_num);
void DamoclesSwordControl(int16_t item_num);
void DamoclesSwordCollision(
    int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll);
void DartEmitterControl(int16_t item_num);
void DartsControl(int16_t item_num);
void DartEffectControl(int16_t fx_num);
void FlameEmitterControl(int16_t item_num);
void FlameControl(int16_t fx_num);
void LavaBurn(ITEM_INFO* item);
void LavaEmitterControl(int16_t item_num);
void LavaControl(int16_t fx_num);
void LavaWedgeControl(int16_t item_num);

void T1MInjectGameTraps();

#endif
