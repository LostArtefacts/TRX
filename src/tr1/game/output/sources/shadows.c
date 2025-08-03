#include "game/output/sources/shadows.h"

#include "game/output.h"
#include "game/output/mesh_batcher/mesh_builder.h"

#include <libtrx/config.h>
#include <libtrx/memory.h>

typedef struct {
    MESH_BATCHER *batcher;
    OUTPUT_MESH *mesh_low;
    OUTPUT_MESH *mesh_high;
} M_PRIV;

static M_PRIV m_Priv;

static OUTPUT_MESH *M_GenerateShadow(MESH_BUILDER *builder, int32_t fidelity)
{
    const int32_t y = -5;
    const RGBA_8888 color = { 0, 0, 0, 128 };
    const OUTPUT_MESH_VERTEX center = {
        .pos = { 0.0f, (float)y, 0.0f },
        .normal = { 0.0f, 0.0f, 0.0f },
        .flags = VERT_FLAT_SHADED | VERT_NO_LIGHTING | VERT_NO_CAUSTICS,
        .uvw_idx = -1,
        .trapezoid_ratio = { 1.0f, 1.0f },
        .shade = SHADE_NEUTRAL,
        .color = color,
    };

    for (int32_t i = 0; i < fidelity; i++) {
        const int16_t angle1 = ((i + 0) * DEG_360 + DEG_180) / fidelity;
        const int16_t angle2 = ((i + 1) * DEG_360 + DEG_180) / fidelity;
        const int32_t size = WALL_L / 2;
        const int32_t x1 = (Math_Sin(angle1) * size) >> W2V_SHIFT;
        const int32_t z1 = (Math_Cos(angle1) * size) >> W2V_SHIFT;
        const int32_t x2 = (Math_Sin(angle2) * size) >> W2V_SHIFT;
        const int32_t z2 = (Math_Cos(angle2) * size) >> W2V_SHIFT;

        OUTPUT_MESH_VERTEX v1 = center;
        v1.pos.x = x1;
        v1.pos.z = z1;
        OUTPUT_MESH_VERTEX v2 = center;
        v2.pos.x = x2;
        v2.pos.z = z2;
        MeshBuilder_AddVertex(builder, &center);
        MeshBuilder_AddVertex(builder, &v1);
        MeshBuilder_AddVertex(builder, &v2);
        MeshBuilder_AddFace3(builder, true);
    }
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
    MeshBatcher_RemoveMesh(p->batcher, p->mesh_low);
    if (p->mesh_low != nullptr) {
        Output_Mesh_Destroy(p->mesh_low);
    }
    MeshBatcher_RemoveMesh(p->batcher, p->mesh_high);
    if (p->mesh_high != nullptr) {
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
    };
    // XXX: Mesh batcher currently collects the transparent faces for the
    // transparent pass in the opaque pass, so the shadow, even though
    // transparent, needs to be staged in the opaque pass to work.
    MeshBatcher_Stage(p->batcher, &inst, SCENE_PASS_MESHES);
}
