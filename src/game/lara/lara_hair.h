#pragma once

#include "global/types.h"

#include <stdbool.h>

void Lara_Hair_Initialise(void);
void Lara_Hair_SetLaraType(GAME_OBJECT_ID lara_type);
void Lara_Hair_Control(bool in_cutscene);
void Lara_Hair_Draw(void);
