#pragma once

#include <stdint.h>

void Bird_SetupEagle(void);
void Bird_SetupCrow(void);

void __cdecl Bird_Initialise(int16_t item_num);
void __cdecl Bird_Control(int16_t item_num);
