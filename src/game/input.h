#ifndef T1M_GAME_INPUT_H
#define T1M_GAME_INPUT_H

#include "global/types.h"

extern INPUT_STATE g_Input;
extern INPUT_STATE g_InputDB;
extern INPUT_STATE g_OldInputDB;
extern bool g_ConflictLayout[INPUT_KEY_NUMBER_OF];

void Input_Init();
void Input_Update();

#endif
