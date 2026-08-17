#pragma once

#include <trx/game/output/shaders/generic.h>

typedef OUTPUT_SHADER OUTPUT_UI_SHADER;

RESULT Output_UIShader_Create(OUTPUT_UI_SHADER **out_shader);
void Output_UIShader_Free(OUTPUT_UI_SHADER *shader);
void Output_UIShader_Bind(const OUTPUT_UI_SHADER *shader);
