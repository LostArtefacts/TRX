#pragma once

#include "global/types.h"

#include <stdint.h>

void Effect_InitialiseArray(void);
int16_t Effect_Create(int16_t room_num);
void Effect_Kill(int16_t fx_num);
void Effect_NewRoom(int16_t fx_num, int16_t room_num);
void Effect_Draw(int16_t fxnum);
