#pragma once

#include "global/types.h"

extern XYZ_32 g_KeyHolePosition;

void KeyHole_Setup(OBJECT_INFO *obj);
bool KeyHole_Trigger(int16_t item_num);
