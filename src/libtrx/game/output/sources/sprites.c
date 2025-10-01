#include "game/output/sources/sprites.h"

#include "game/output/mesh_batcher/mesh_builder.h"
#include "game/output/scene_compositor.h"
#include "game/output/textures.h"
#include "memory.h"
#include "utils.h"

#include <string.h>

typedef struct {
    SCENE_SOURCE source;
    MESH_BATCHER *batcher;
    OUTPUT_MESH **meshes;
    size_t mesh_count;

    MATRIX last_matrix;
    int32_t stack;
} M_PRIV;

static M_PRIV m_Priv;

static void M_RenderBegin(const SCENE_SOURCE *const src)
{
    M_PRIV *const p = &m_Priv;
    memset(&p->last_matrix, 0, sizeof(MATRIX));
    p->stack = 0;
}

static void M_PrepareMeshes(M_PRIV *const p)
{
    p->mesh_count = Output_GetSpriteTextureCount();
    p->meshes = Memory_Alloc(sizeof(*p->meshes) * p->mesh_count);
    MESH_BUILDER *const builder = MeshBuilder_Create();
    for (int32_t i = 0; i < (int32_t)p->mesh_count; i++) {
        ROOM_VERTEX fake_vert = {};
        const ROOM fake_room = { .mesh = { .vertices = &fake_vert } };
        ROOM_SPRITE fake_sprite = { .texture = (uint16_t)i, .vertex = 0 };
        MeshBuilder_AddRoomSprite(builder, &fake_sprite, &fake_room, 0.0f);
        OUTPUT_MESH *const mesh = MeshBuilder_Seal(builder);
        MeshBatcher_AddMesh(p->batcher, mesh);
        p->meshes[i] = mesh;
    }
    MeshBuilder_Destroy(builder);
}

static void M_FreeMeshes(M_PRIV *const p)
{
    if (p->meshes != nullptr) {
        for (size_t i = 0; i < p->mesh_count; i++) {
            MeshBatcher_RemoveMesh(p->batcher, p->meshes[i]);
            Output_Mesh_Destroy(p->meshes[i]);
        }
        Memory_FreePointer(&p->meshes);
    }
}

static void M_UpdateShades(MESH_INSTANCE *inst, void *user_data)
{
    int16_t shade = (int16_t)(intptr_t)user_data;
    CLAMP(shade, 0, SHADE_MAX);
    OUTPUT_MESH_VERTEX *const vertices = Vector_GetData(inst->mesh->vertices);
    for (int32_t i = 0; i < inst->mesh->vertices->count; i++) {
        vertices[i].shade = shade;
    }
}

void OutputSource_Sprites_Init(MESH_BATCHER *batcher)
{
    m_Priv.batcher = batcher;
    m_Priv.source.render_begin = M_RenderBegin;
    SceneCompositor_AddSource(&m_Priv.source);
}

void OutputSource_Sprites_Shutdown(void)
{
    M_FreeMeshes(&m_Priv);
}

void OutputSource_Sprites_ObserveLevelLoad(void)
{
    M_FreeMeshes(&m_Priv);
    M_PrepareMeshes(&m_Priv);
}

void OutputSource_Sprites_ObserveLevelUnload(void)
{
    M_FreeMeshes(&m_Priv);
}

void OutputSource_Sprites_Stage(int32_t sprite_idx, int16_t shade, RGB_F tint)
{
    M_PRIV *const p = &m_Priv;
    OUTPUT_MESH *mesh = p->meshes[sprite_idx];

    if (memcmp(&p->last_matrix, g_MatrixPtr, sizeof(MATRIX)) == 0) {
        p->stack++;
    } else {
        p->stack = 0;
    }
    p->last_matrix = *g_MatrixPtr;

    const MESH_INSTANCE inst = {
        .mesh = mesh,
        .matrix = *g_MatrixPtr,
        .depth_adjust = p->stack * -0.005f,
        .tint = tint,
        .wibble = false,
        .water_effect = false,
        .update_light_func = M_UpdateShades,
        .update_light_func_data = (void *)(intptr_t)shade,
    };

    MeshBatcher_Stage(p->batcher, &inst, SCENE_PASS_MESHES);
    MeshBatcher_Stage(p->batcher, &inst, SCENE_PASS_TRANSPARENT);
}
