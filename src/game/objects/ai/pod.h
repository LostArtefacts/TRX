#pragma once

#include "global/types.h"

#include <stdint.h>

#define POD_EXPLODE_DIST (WALL_L * 4) // = 4096

typedef enum {
    POD_SET = 0,
    POD_EXPLODE = 1,
} POD_ANIM;

void Pod_Setup(OBJECT_INFO *obj);
void Pod_SetupBig(OBJECT_INFO *obj);
void Pod_Initialise(int16_t item_num);
void Pod_Control(int16_t item_num);
