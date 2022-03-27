#pragma once

#include "global/types.h"

#include <stdint.h>

#define MUMMY_HITPOINTS 18

typedef enum {
    MUMMY_EMPTY = 0,
    MUMMY_STOP = 1,
    MUMMY_DEATH = 2,
} MUMMY_ANIM;

void Mummy_Setup(OBJECT_INFO *obj);
void Mummy_Initialise(int16_t item_num);
void Mummy_Control(int16_t item_num);
