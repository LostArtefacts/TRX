#pragma once

#include <trx/game/effects/types.h>
#include <trx/game/objects/ids.h>

#include <stddef.h>

void Effect_InitialiseArray(void);
void Effect_Control(void);

EFFECT *Effect_Get(int16_t effect_num);
int16_t Effect_GetIndex(const EFFECT *effect);
int16_t Effect_GetInOrderNum(int16_t effect_num);
int16_t Effect_GetActiveNum(void);
int16_t Effect_Create(OBJECT_ID object_id, int16_t room_num);

// Allocates zeroed private data for an effect. Reuses the effect slot's block
// when it is large enough.
void *Effect_AllocPriv(int16_t effect_num, size_t size);
void Effect_Destroy(int16_t effect_num);
void Effect_UpdateRoom(int16_t effect_num, int16_t room_num);
