#pragma once

#include <trx/core/colors.h>
#include <trx/game/matrix.h>
#include <trx/game/objects/types.h>

void OutputSource_Sky_Init(void);
void OutputSource_Sky_Shutdown(void);

// Schedules a flat sky layer quad pair for drawing in the background pass.
// The model matrix positions the layer relative to the camera. The color is
// in the OG 128-neutral scale, handled by the shader's overbright path.
void OutputSource_Sky_StageLayer(
    const MATRIX *wmatrix, RGB_888 color, bool additive);

// Schedules the TR4 desert-level horizon fog gradient: the mesh's first 16
// quads redrawn as flat quads of the fog color, transparent at each quad's
// first two vertices and fully opaque at the other two, blending the mesh's
// bottom edge into the fogged-out terrain (OG's phd_PutPolygonSkyMesh).
void OutputSource_Sky_StageFogGradient(
    const MATRIX *wmatrix, const OBJECT_MESH *mesh, RGBA_F color);

// Drops the cached gradient geometry; must be called when the skybox mesh it
// was built from may no longer be valid (e.g. on level change).
void OutputSource_Sky_InvalidateFogGradient(void);
