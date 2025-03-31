#pragma once

#include "game/output/shader.h"

void Output_Meshes_Init(void);
void Output_Meshes_Shutdown(void);
void Output_Meshes_RenderBegin(void);
OUTPUT_SHADER *Output_Meshes_GetShader(void);
