#pragma once

#include "global/types.h"

#include <stdbool.h>

extern INPUT_STATE g_Input;
extern INPUT_STATE g_InputDB;
extern INPUT_STATE g_OldInputDB;
extern bool g_ConflictLayout[INPUT_ROLE_NUMBER_OF];

void Input_Init(void);
void Input_Shutdown(void);
void Input_Update(void);
