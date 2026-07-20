#include <trx/game/lara/skin/seam.h>

#include <trx/core/math/const.h>
#include <trx/core/math/geom.h>
#include <trx/core/utils.h>
#include <trx/debug.h>

static bool M_VertexMatches(const XYZ_32 a, const XYZ_32 b)
{
    return ABS(a.x - b.x) <= 1 && ABS(a.y - b.y) <= 1 && ABS(a.z - b.z) <= 1;
}

void Lara_Seam_FindSharedVertices(
    SEAM_VERTEX_PAIR *const pairs, int32_t *const pair_count,
    const int32_t max_pairs, const OBJECT_MESH *const mesh_a,
    const OBJECT_MESH *const mesh_b, const XYZ_32 *const pos_a,
    const XYZ_32 *const pos_b)
{
    *pair_count = 0;
    for (int32_t a = 0; a < mesh_a->num_vertices; a++) {
        const XYZ_32 a_world = {
            mesh_a->vertices[a].x + pos_a->x,
            mesh_a->vertices[a].y + pos_a->y,
            mesh_a->vertices[a].z + pos_a->z,
        };
        for (int32_t b = 0; b < mesh_b->num_vertices; b++) {
            const XYZ_32 b_world = {
                mesh_b->vertices[b].x + pos_b->x,
                mesh_b->vertices[b].y + pos_b->y,
                mesh_b->vertices[b].z + pos_b->z,
            };
            if (!M_VertexMatches(a_world, b_world)) {
                continue;
            }

            ASSERT(*pair_count < max_pairs);
            pairs[*pair_count].vertex_a = a;
            pairs[*pair_count].vertex_b = b;
            (*pair_count)++;
        }
    }
}

// Applies the exact inverse of "to"'s rotation to a world-space vector. The
// entries are quantized to 1/(1<<W2V_SHIFT), so the transpose is only an
// approximate inverse, and the leftover error reads as a hairline crack
// between welded meshes; the adjugate stays exact for whatever rotation the
// matrix holds.
static XYZ_F M_UndoRotation(
    const MATRIX *const to, const double dx, const double dy, const double dz)
{
    const double r00 = to->_00, r01 = to->_01, r02 = to->_02;
    const double r10 = to->_10, r11 = to->_11, r12 = to->_12;
    const double r20 = to->_20, r21 = to->_21, r22 = to->_22;

    const double c00 = r11 * r22 - r12 * r21;
    const double c01 = r12 * r20 - r10 * r22;
    const double c02 = r10 * r21 - r11 * r20;
    const double c10 = r02 * r21 - r01 * r22;
    const double c11 = r00 * r22 - r02 * r20;
    const double c12 = r01 * r20 - r00 * r21;
    const double c20 = r01 * r12 - r02 * r11;
    const double c21 = r02 * r10 - r00 * r12;
    const double c22 = r00 * r11 - r01 * r10;

    const double det = r00 * c00 + r01 * c01 + r02 * c02;
    return (XYZ_F) {
        (float)((c00 * dx + c10 * dy + c20 * dz) / det),
        (float)((c01 * dx + c11 * dy + c21 * dz) / det),
        (float)((c02 * dx + c12 * dy + c22 * dz) / det),
    };
}

XYZ_F Lara_Seam_TransformPos(
    const MATRIX *const from, const MATRIX *const to, const XYZ_32 v)
{
    const int64_t dx = from->_00 * v.x + from->_01 * v.y + from->_02 * v.z
        + from->_03 - to->_03;
    const int64_t dy = from->_10 * v.x + from->_11 * v.y + from->_12 * v.z
        + from->_13 - to->_13;
    const int64_t dz = from->_20 * v.x + from->_21 * v.y + from->_22 * v.z
        + from->_23 - to->_23;
    return M_UndoRotation(to, dx, dy, dz);
}

XYZ_F Lara_Seam_TransformNormal(
    const MATRIX *const from, const MATRIX *const to, const XYZ_16 n)
{
    const int64_t wx = from->_00 * n.x + from->_01 * n.y + from->_02 * n.z;
    const int64_t wy = from->_10 * n.x + from->_11 * n.y + from->_12 * n.z;
    const int64_t wz = from->_20 * n.x + from->_21 * n.y + from->_22 * n.z;
    return M_UndoRotation(to, wx, wy, wz);
}
