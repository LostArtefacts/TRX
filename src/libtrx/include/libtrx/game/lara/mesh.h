#pragma once

#include "../game_flow.h"
#include "../objects.h"
#include "./enum.h"

void Lara_Mesh_Initialise(const GF_LEVEL *level);
void Lara_Mesh_SwapSingle(LARA_MESH mesh, OBJECT_ID obj_id);
void Lara_Mesh_SwapAll(OBJECT_ID obj_id);
void Lara_Mesh_Set(LARA_MESH mesh, OBJECT_MESH *mesh_ptr);
OBJECT_MESH *Lara_Mesh_Get(LARA_MESH mesh);
