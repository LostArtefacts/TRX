#pragma once

#include <stdint.h>

#define SKIDOO_DRIVER_HITPOINTS 100

void SkidooDriver_Setup(void);
void __cdecl SkidooDriver_Initialise(int16_t item_num);
void __cdecl SkidooDriver_Control(int16_t item_num);
