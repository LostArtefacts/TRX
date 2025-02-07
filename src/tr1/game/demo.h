#pragma once

#include <libtrx/game/demo.h>

bool Demo_Start(int32_t level_num);
void Demo_End(void);
void Demo_Pause(void);
void Demo_Unpause(void);
void Demo_StopFlashing(void);

GF_COMMAND Demo_Control(void);
int32_t Demo_ChooseLevel(int32_t demo_num);
