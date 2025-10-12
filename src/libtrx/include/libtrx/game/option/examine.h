#pragma once

#include "../objects/types.h"

bool Option_Examine_CanExamine(OBJECT_ID obj_id);
void Option_Examine_Control(OBJECT_ID obj_id, bool is_busy);
void Option_Examine_Draw(void);
void Option_Examine_Close(void);
