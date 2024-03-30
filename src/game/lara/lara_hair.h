#pragma once

#include "global/types.h"

typedef struct HAIR_SEGMENT {
    XYZ_32 pos;
    XYZ_16 rot;
} HAIR_SEGMENT;

bool Lara_Hair_IsActive(void);
void Lara_Hair_Initialise(void);
void Lara_Hair_SetLaraType(GAME_OBJECT_ID lara_type);
void Lara_Hair_Control(void);
void Lara_Hair_Draw(void);
