#pragma once

#include <libtrx/game/objects/types.h>

bool Option_Examine_CanExamine(GAME_OBJECT_ID obj_id);
bool Option_Examine_IsActive(void);
void Option_Examine_Control(GAME_OBJECT_ID obj_id, bool is_busy);
void Option_Examine_Draw(void);
void Option_Examine_Shutdown(void);
