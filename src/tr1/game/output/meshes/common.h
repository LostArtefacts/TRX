#pragma once

#include "game/output/shader.h"

// clang-format off
#define VERT_NO_CAUSTICS 0b0000'0001 // = 0x01
#define VERT_FLAT_SHADED 0b0000'0010 // = 0x02
#define VERT_REFLECTIVE  0b0000'0100 // = 0x04
#define VERT_NO_LIGHTING 0b0000'1000 // = 0x08
// clang-format on

void Output_Meshes_Init(void);
void Output_Meshes_Shutdown(void);
void Output_Meshes_UploadProjectionMatrix(void);
void Output_Meshes_ObserveLevelLoad(void);
void Output_Meshes_ObserveLevelUnload(void);
void Output_Meshes_RenderBegin(void);
OUTPUT_SHADER *Output_Meshes_GetShader(void);
