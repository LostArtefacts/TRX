#pragma once

#include "./game_flow/types.h"

void Demo_InitialiseData(uint16_t data_length);
uint32_t *Demo_GetData(void);

extern bool Demo_Start(int32_t level_num);
extern void Demo_End(void);
extern void Demo_Pause(void);
extern void Demo_Unpause(void);
extern void Demo_StopFlashing(void);

extern bool Demo_GetInput(void);
extern GF_COMMAND Demo_Control(void);
extern int32_t Demo_ChooseLevel(int32_t demo_num);
