#pragma once

#include "./types.h"

extern void Effect_InitialiseArray(void);
extern void Effect_Control(void);

extern EFFECT *Effect_Get(int16_t effect_num);
extern int16_t Effect_GetNum(const EFFECT *effect);
extern int16_t Effect_GetActiveNum(void);
extern int16_t Effect_Create(int16_t room_num);
extern void Effect_Kill(int16_t effect_num);
extern void Effect_NewRoom(int16_t effect_num, int16_t room_num);
extern void Effect_Draw(int16_t effect_num);
