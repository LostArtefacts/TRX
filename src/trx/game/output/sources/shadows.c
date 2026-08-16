#include <trx/game/output/sources/shadows.h>

#include <trx/config.h>
#include <trx/core/memory.h>
#include <trx/game/output.h>
#include <trx/game/output/mesh_batcher/mesh_builder.h>
#include <trx/version.h>

typedef struct {
    MESH_BATCHER *batcher;
    OUTPUT_MESH *mesh_low;
    OUTPUT_MESH *mesh_high;
} M_PRIV;

static M_PRIV m_Priv;

static OUTPUT_MESH *M_GenerateShadow(
    MESH_BUILDER *const builder, const int32_t fidelity)
{
    const int32_t y = -5;
    const RGBA_8888 color = { 0, 0, 0, g_TRVersion == 4 ? 0x4F : 128 };
    const OUTPUT_MESH_VERTEX center = {
        .pos = { 0.0f, (float)y, 0.0f, 0.0f },
        .normal = { 0.0f, 0.0f, 0.0f },
        .flags = VERT_FLAT_SHADED | VERT_NO_LIGHTING | VERT_NO_WIBBLE,
        .uvw_idx = -1,
        .trapezoid_ratio = { 1.0f, 1.0f },
        .reflectivity = 1.0f,
        .shade = SHADE_NEUTRAL,
        .color = color,
    };

    MeshBuilder_AddVertex(builder, &center);
    for (int32_t i = 0; i <= fidelity; i++) {
        const int16_t angle = ((i * DEG_360) + DEG_180) / fidelity;
        const int32_t size = WALL_L / 2;
        const XYZ_32 point = XYZ_32_RotateYaw((XYZ_32) { .z = size }, angle);
        const int32_t x = point.x;
        const int32_t z = point.z;
        OUTPUT_MESH_VERTEX edge = center;
        edge.pos.x = x;
        edge.pos.z = z;
        MeshBuilder_AddVertex(builder, &edge);
    }
    // The shadow never writes depth: it lies on the floor and occludes
    // nothing, and its depth-writing copy would be a second sorted draw for an
    // instance drawn at partial coverage.
    MeshBuilder_AddFan(builder, SCENE_PASS_TRANSPARENT, false, false);
    return MeshBuilder_Seal(builder);
}

void OutputSource_Shadows_Init(MESH_BATCHER *const batcher)
{
    M_PRIV *const p = &m_Priv;
    p->batcher = batcher;

    // Build low- and high-fidelity circular shadow meshes.
    MESH_BUILDER *const builder = MeshBuilder_Create();
    p->mesh_low = M_GenerateShadow(builder, 8);
    p->mesh_high = M_GenerateShadow(builder, 32);
    MeshBuilder_Destroy(builder);

    MeshBatcher_AddMesh(p->batcher, p->mesh_low);
    MeshBatcher_AddMesh(p->batcher, p->mesh_high);
}

void OutputSource_Shadows_Shutdown(void)
{
    M_PRIV *const p = &m_Priv;
    if (p->mesh_low != nullptr) {
        if (p->batcher != nullptr) {
            MeshBatcher_RemoveMesh(p->batcher, p->mesh_low);
        }
        Output_Mesh_Destroy(p->mesh_low);
        p->mesh_low = nullptr;
    }
    if (p->mesh_high != nullptr) {
        if (p->batcher != nullptr) {
            MeshBatcher_RemoveMesh(p->batcher, p->mesh_high);
        }
        Output_Mesh_Destroy(p->mesh_high);
        p->mesh_high = nullptr;
    }
    p->batcher = nullptr;
}

void OutputSource_Shadows_StageShadow(void)
{
    M_PRIV *const p = &m_Priv;
    OUTPUT_MESH *const mesh = g_Config.visuals.shadow_type == SHADOW_TYPE_CIRCLE
        ? p->mesh_high
        : p->mesh_low;
    const MESH_INSTANCE inst = {
        .mesh = mesh,
        .cwmatrix = *g_MatrixPtr,
        .wmatrix = *g_WMatrixPtr,
        // The shadow is black, so the tint only reaches it through its alpha:
        // an item drawn at partial coverage takes its shadow along with it.
        .tint = Output_GetTint(),
        .sort_layer = -1,
        .room = Output_GetCurrentRoom(),
    };
    // XXX: Mesh batcher currently collects the transparent faces for the
    // transparent pass in the opaque pass, so the shadow, even though
    // transparent, needs to be staged in the opaque pass to work.
    MeshBatcher_Stage(p->batcher, &inst, SCENE_PASS_OPAQUE);
}
