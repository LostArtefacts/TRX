#ifndef T1M_GAME_TRAPS_H
#define T1M_GAME_TRAPS_H

#include "game/types.h"
#include <stdint.h>

// clang-format off
#define InitialiseLightning     ((void          __cdecl(*)(int16_t item_num))0x00429B00)
#define DrawLightning           ((void          __cdecl(*)(ITEM_INFO *item))0x00429620)
#define LightningControl        ((void          __cdecl(*)(int16_t item_num))0x00429B80)
#define LightningCollision      ((void          __cdecl(*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x00429E30)
#define InitialiseThorsHandle   ((void          __cdecl(*)(int16_t item_num))0x00429EA0)
#define ThorsHandleControl      ((void          __cdecl(*)(int16_t item_num))0x00429F30)
#define ThorsHandleCollision    ((void          __cdecl(*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x0042A1F0)
#define ThorsHeadCollision      ((void          __cdecl(*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x0042A240)
#define MidasCollision          ((void          __cdecl(*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x004334C0)
#define InitialiseMovableBlock  ((void          __cdecl(*)(int16_t item_num))0x0042B430)
#define MovableBlockControl     ((void          __cdecl(*)(int16_t item_num))0x0042B460)
#define MovableBlockCollision   ((void          __cdecl(*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x0042B5B0)
#define InitialiseRollingBlock  ((void          __cdecl(*)(int16_t item_num))0x0042BB90)
#define RollingBlockControl     ((void          __cdecl(*)(int16_t item_num))0x0042BBC0)
#define DrawMovableBlock        ((void          __cdecl(*)(ITEM_INFO *item))0x0042BD60)
#define InitialiseRollingBall   ((void          __cdecl(*)(int16_t item_num))0x0043A010)
#define RollingBallControl      ((void          __cdecl(*)(int16_t item_num))0x0043A050)
#define RollingBallCollision    ((void          __cdecl(*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x0043A2B0)
#define SpikeCollision          ((void          __cdecl(*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x0043A520)
#define PendulumControl         ((void          __cdecl(*)(int16_t item_num))0x0043A820)
#define FallingBlockControl     ((void          __cdecl(*)(int16_t item_num))0x0043A970)
#define FallingBlockFloor       ((void          __cdecl(*)(ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height))0x0043AA70)
#define FallingBlockCeiling     ((void          __cdecl(*)(ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height))0x0043AAB0)
#define TeethTrapControl        ((void          __cdecl(*)(int16_t item_num))0x0043AAF0)
#define FallingCeilingControl   ((void          __cdecl(*)(int16_t item_num))0x0043ABC0)
#define InitialiseDamoclesSword ((void          __cdecl(*)(int16_t item_num))0x0043AC60)
#define DamoclesSwordControl    ((void          __cdecl(*)(int16_t item_num))0x0043ACA0)
#define DamoclesSwordCollision  ((void          __cdecl(*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x0043ADD0)
#define DartEmitterControl      ((void          __cdecl(*)(int16_t item_num))0x0043AEC0)
#define DartsControl            ((void          __cdecl(*)(int16_t item_num))0x0043B060)
#define DartEffectControl       ((void          __cdecl(*)(int16_t item_num))0x0043B1A0)
#define FlameEmitterControl     ((void          __cdecl(*)(int16_t item_num))0x0043B1F0)
#define FlameControl            ((void          __cdecl(*)(int16_t item_num))0x0043B2A0)
#define LavaEmitterControl      ((void          __cdecl(*)(int16_t item_num))0x0043B520)
#define LavaControl             ((void          __cdecl(*)(int16_t item_num))0x0043B5F0)
#define LavaWedgeControl        ((void          __cdecl(*)(int16_t item_num))0x0043B710)
// clang-format on

#endif
