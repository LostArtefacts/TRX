#pragma once

#include <libtrx/game/matrix.h>
#include <libtrx/game/objects/types.h>
#include <libtrx/game/output/objects.h>
#include <libtrx/game/output/types.h>

void Output_Meshes_InitObjects(void);
void Output_Meshes_ShutdownObjects(void);
void Output_Meshes_ObserveLevelLoadObjects(void);
void Output_Meshes_ObserveLevelUnloadObjects(void);

void Output_Meshes_RenderObjectMesh(
    const MATRIX *matrix, RGB_F tint, const OBJECT_MESH *mesh);
