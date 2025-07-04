#pragma once

#include "../game_flow.h"
#include "./types.h"

extern void Lara_InitialiseMeshes(const GF_LEVEL *level);

void Lara_Control_Initialise(
    GF_LEVEL_TYPE level_type, LARA_EXTRA_STATE start_state);
void Lara_Control(void);
