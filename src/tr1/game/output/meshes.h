#pragma once

#include "game/output/shader.h"

// clang-format off
#define VERT_NO_CAUSTICS 0b0000'0001 // = 0x01
// clang-format on

void Output_Meshes_Init(void);
void Output_Meshes_Shutdown(void);
void Output_Meshes_RenderBegin(void);
OUTPUT_SHADER *Output_Meshes_GetShader(void);
