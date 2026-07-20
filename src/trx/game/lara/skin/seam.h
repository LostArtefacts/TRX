#pragma once

#include <trx/game/matrix.h>
#include <trx/game/objects/types.h>

#include <stdint.h>

#define SEAM_MAX_VERTEX_PAIRS 32

// A vertex of mesh A that coincides, at bind pose, with a vertex of mesh B.
typedef struct {
    uint16_t vertex_a;
    uint16_t vertex_b;
} SEAM_VERTEX_PAIR;

// Records every pair of vertices, one from each mesh, that land on the same
// world position when the meshes are placed at pos_a and pos_b. Used to find
// the ring two adjacent meshes share so it can be welded shut at runtime.
void Lara_Seam_FindSharedVertices(
    SEAM_VERTEX_PAIR *pairs, int32_t *pair_count, int32_t max_pairs,
    const OBJECT_MESH *mesh_a, const OBJECT_MESH *mesh_b, const XYZ_32 *pos_a,
    const XYZ_32 *pos_b);

// Takes a point local to the "from" bone and answers where it sits in the "to"
// bone's local space: apply "from" to reach the world position, then undo "to".
// The camera baked into both matrices cancels. The result stays float on
// purpose - snapping it to whole units is what shows up as a hairline crack
// between meshes; the undoing is exact for the same reason.
XYZ_F Lara_Seam_TransformPos(const MATRIX *from, const MATRIX *to, XYZ_32 v);

// The direction-only counterpart to Lara_Seam_TransformPos: rotates a normal
// out of the "from" bone's frame and into the "to" bone's, dropping the
// translation. The two rotations cancel to identity when the meshes are aligned
// and diverge as they bend, which is how much the welded ring's lighting has to
// turn to line up with the frame it is drawn in.
XYZ_F Lara_Seam_TransformNormal(const MATRIX *from, const MATRIX *to, XYZ_16 n);
