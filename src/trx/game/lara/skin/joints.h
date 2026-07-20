#pragma once

#include <trx/game/lara/skin/types.h>

void Lara_Joints_Initialise(const LARA_SKIN_OUTFIT *outfit);
void Lara_Joints_StashMatrix(LARA_MESH mesh_idx, bool interpolated);
void Lara_Joints_Draw(LARA_MESH mesh_idx, CLIP clip, bool interpolated);

// The matrix a body mesh was staged with this frame, post-interpolation. Null
// until the body has drawn, or when joints are inactive.
const MATRIX *Lara_Joints_GetMeshMatrix(LARA_MESH mesh_idx);
