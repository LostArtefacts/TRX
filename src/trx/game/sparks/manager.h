#pragma once

#include <trx/core/handle.h>
#include <trx/core/result.h>
#include <trx/game/sparks/types.h>

typedef struct JSON_READ_IO JSON_READ_IO;
typedef struct JSON_WRITE_IO JSON_WRITE_IO;

void Sparks_Reset(void);
void Sparks_Control(void);
void Sparks_Draw(void);

// Writes and reads one spark. A sprite spark stores its object and sprite
// offset. The owning pool stores the on flag and attachment.
void Sparks_SaveSpark(JSON_WRITE_IO *io, const SPARK *spark);
RESULT Sparks_LoadSpark(JSON_READ_IO *io, SPARK *spark);

// Writes each live spark, or nothing while enhanced saves are off.
void Sparks_Save(JSON_WRITE_IO *io);

// Reads saved sparks. Reports failure if the save names a sprite the level does
// not carry. Leaves unnamed slots as Sparks_Reset set them.
RESULT Sparks_Load(JSON_READ_IO *io);

void Sparks_DetachEffect(int16_t effect_num);
void Sparks_DetachItem(int16_t item_num);

XYZ_32 Sparks_GetWorldPos(const SPARK *spark);

SPARK *Sparks_GetFreeSpark(void);
SPARK *Sparks_GetSpark(int32_t idx);
int32_t Sparks_GetMaxCount(void);

// Identity of the spark holding a pool slot. Taking the slot for another spark
// retires the handles that named the previous one, as does a level change.
TRX_HANDLE Sparks_GetHandle(const SPARK *spark);
// Resolves a handle to a live spark, or nullptr where the slot has been taken
// for another spark, or holds one whose life has run out.
SPARK *Sparks_FromHandle(TRX_HANDLE handle);
SPARK *Sparks_InitialiseSpriteSpark(SPARK_SPRITE_TYPE type);
int32_t Sparks_GetSpriteIndex(SPARK_SPRITE_TYPE offset);
void Sparks_Sync(SPARK *spark);
void Sparks_FinishSetup(SPARK *spark);

int8_t Sparks_AllocDynamic(uint8_t flags);
void Sparks_FreeDynamic(int8_t idx);

XZ_32 Sparks_GetSmokeWind(void);
void Sparks_SetSmokeWind(XZ_32 wind);
int32_t Sparks_GetHairWindZ(void);
