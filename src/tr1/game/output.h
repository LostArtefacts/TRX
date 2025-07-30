#pragma once

#include "game/output/draw.h"
#include "game/output/func.h"
#include "game/output/scene_compositor.h"
#include "game/output/shader.h"
#include "game/output/state.h"
#include "global/types.h"

#include <libtrx/game/output.h>

#include <stddef.h>
#include <stdint.h>

void Output_ApplyLevelSettings(void);
void Output_ApplyRenderSettings(void);

void Output_ClearDepthBuffer(void);

OUTPUT_SHADER *Output_GetMeshShader(void);
