#pragma once

#include <trx/core/file.h>
#include <trx/game/game_flow/types.h>

void Demo_LoadData(TRX_FILE *file, size_t size);
uint32_t *Demo_GetData(void);

bool Demo_Start(int32_t level_num);
void Demo_End(void);
void Demo_Pause(void);
void Demo_Unpause(void);
void Demo_StopFlashing(void);

// Returns whether the current level is a TR3 demo. These come from the PS1
// release and stay in sync only where the game follows its rules.
bool Demo_UsesPS1Rules(void);

bool Demo_UpdateInput(void);
GF_COMMAND Demo_Control(void);
int32_t Demo_ChooseLevel(int32_t demo_num);
