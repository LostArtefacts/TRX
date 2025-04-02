#pragma once

#include <libtrx/game/matrix.h>
#include <libtrx/game/objects/types.h>
#include <libtrx/game/output/objects.h>
#include <libtrx/game/output/types.h>

void Output_Objects_Init(void);
void Output_Objects_Shutdown(void);
void Output_Objects_ObserveLevelLoad(void);
void Output_Objects_ObserveLevelUnload(void);

void Output_Objects_RenderMesh(
    const MATRIX *matrix, RGB_F tint, const OBJECT_MESH *mesh);
