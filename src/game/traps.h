#ifndef T1M_GAME_TRAPS_H
#define T1M_GAME_TRAPS_H

#include "game/types.h"
#include <stdint.h>

// clang-format off
#define MidasCollision          ((void          (*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x004334C0)
#define InitialiseMovableBlock  ((void          (*)(int16_t item_num))0x0042B430)
#define MovableBlockControl     ((void          (*)(int16_t item_num))0x0042B460)
#define MovableBlockCollision   ((void          (*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x0042B5B0)
#define InitialiseRollingBlock  ((void          (*)(int16_t item_num))0x0042BB90)
#define RollingBlockControl     ((void          (*)(int16_t item_num))0x0042BBC0)
#define DrawMovableBlock        ((void          (*)(ITEM_INFO *item))0x0042BD60)
#define InitialiseRollingBall   ((void          (*)(int16_t item_num))0x0043A010)
#define RollingBallControl      ((void          (*)(int16_t item_num))0x0043A050)
#define RollingBallCollision    ((void          (*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x0043A2B0)
#define SpikeCollision          ((void          (*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x0043A520)
#define PendulumControl         ((void          (*)(int16_t item_num))0x0043A820)
#define FallingBlockControl     ((void          (*)(int16_t item_num))0x0043A970)
#define FallingBlockFloor       ((void          (*)(ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height))0x0043AA70)
#define FallingBlockCeiling     ((void          (*)(ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height))0x0043AAB0)
#define TeethTrapControl        ((void          (*)(int16_t item_num))0x0043AAF0)
#define FallingCeilingControl   ((void          (*)(int16_t item_num))0x0043ABC0)
#define InitialiseDamoclesSword ((void          (*)(int16_t item_num))0x0043AC60)
#define DamoclesSwordControl    ((void          (*)(int16_t item_num))0x0043ACA0)
#define DamoclesSwordCollision  ((void          (*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x0043ADD0)
#define DartEmitterControl      ((void          (*)(int16_t item_num))0x0043AEC0)
#define DartsControl            ((void          (*)(int16_t item_num))0x0043B060)
#define DartEffectControl       ((void          (*)(int16_t item_num))0x0043B1A0)
#define FlameEmitterControl     ((void          (*)(int16_t item_num))0x0043B1F0)
#define LavaEmitterControl      ((void          (*)(int16_t item_num))0x0043B520)
#define LavaControl             ((void          (*)(int16_t item_num))0x0043B5F0)
#define LavaWedgeControl        ((void          (*)(int16_t item_num))0x0043B710)
// clang-format on

void FlameControl(int16_t fx_num);
void LavaBurn(ITEM_INFO* item);

void T1MInjectGameTraps();

#endif
