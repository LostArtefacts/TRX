#include "game/output/sources/shadows.h"

#include "config.h"
#include "game/output.h"
#include "game/output/mesh_batcher/mesh_builder.h"
#include "memory.h"

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
    const RGBA_8888 color = { 0, 0, 0, 128 };
    const OUTPUT_MESH_VERTEX center = {
        .pos = { 0.0f, (float)y, 0.0f, 0.0f },
        .normal = { 0.0f, 0.0f, 0.0f },
        .flags = VERT_FLAT_SHADED | VERT_NO_LIGHTING | VERT_NO_CAUSTICS,
        .uvw_idx = -1,
        .trapezoid_ratio = { 1.0f, 1.0f },
        .shade = SHADE_NEUTRAL,
        .color = color,
    };

    MeshBuilder_AddVertex(builder, &center);
    for (int32_t i = 0; i <= fidelity; i++) {
        const int16_t angle = ((i * DEG_360) + DEG_180) / fidelity;
        const int32_t size = WALL_L / 2;
        const int32_t x = (Math_Sin(angle) * size) >> W2V_SHIFT;
        const int32_t z = (Math_Cos(angle) * size) >> W2V_SHIFT;
        OUTPUT_MESH_VERTEX edge = center;
        edge.pos.x = x;
        edge.pos.z = z;
        MeshBuilder_AddVertex(builder, &edge);
    }
    MeshBuilder_AddFan(builder, true);
    return MeshBuilder_Seal(builder);
}

static int32_t M_TransparentSort(
    const MESH_INSTANCE *const inst, const OUTPUT_MESH_FACE *const face)
{
    return 0; // Always draw shadows last.
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
        MeshBatcher_RemoveMesh(p->batcher, p->mesh_low);
        Output_Mesh_Destroy(p->mesh_low);
    }
    if (p->mesh_high != nullptr) {
        MeshBatcher_RemoveMesh(p->batcher, p->mesh_high);
        Output_Mesh_Destroy(p->mesh_high);
    }
}

void OutputSource_Shadows_StageShadow(void)
{
    M_PRIV *const p = &m_Priv;
    OUTPUT_MESH *const mesh =
        g_Config.visuals.enable_round_shadow ? p->mesh_high : p->mesh_low;
    const MESH_INSTANCE inst = {
        .mesh = mesh,
        .matrix = *g_MatrixPtr,
        .tint = { 1.0f, 1.0f, 1.0f },
        .transparent_sort_func = M_TransparentSort,
    };
    // XXX: Mesh batcher currently collects the transparent faces for the
    // transparent pass in the opaque pass, so the shadow, even though
    // transparent, needs to be staged in the opaque pass to work.
    MeshBatcher_Stage(p->batcher, &inst, SCENE_PASS_MESHES);
}
