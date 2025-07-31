#include "game/output/sources/sprites.h"

#include "game/output/mesh_batcher/batcher.h"
#include "game/output/mesh_batcher/mesh.h"
#include "game/output/textures.h"
#include "game/output/utils.h"

#include <libtrx/game/output/textures.h>
#include <libtrx/memory.h>
#include <libtrx/utils.h>
#include <libtrx/vector.h>

#include <stdint.h>

typedef struct {
    MESH_BATCHER *batcher;
    OUTPUT_MESH **meshes;
    size_t mesh_count;
} M_PRIV;

static M_PRIV m_Priv;

static void M_PrepareMeshes(M_PRIV *p);
static void M_FreeMeshes(M_PRIV *p);
static void M_UpdateShades(MESH_INSTANCE *inst, void *user_data);

void OutputSource_Sprites_Init(MESH_BATCHER *batcher)
{
    m_Priv.batcher = batcher;
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
    OUTPUT_MESH *mesh = m_Priv.meshes[sprite_idx];
    const MESH_INSTANCE inst = {
        .mesh = mesh,
        .matrix = *g_MatrixPtr,
        .tint = tint,
        .wibble = false,
        .water_effect = false,
        .update_light_func = M_UpdateShades,
        .update_light_func_data = (void *)(intptr_t)shade,
    };
    MeshBatcher_Stage(m_Priv.batcher, &inst, SCENE_PASS_MESHES);
    MeshBatcher_Stage(m_Priv.batcher, &inst, SCENE_PASS_TRANSPARENT);
}

static void M_PrepareMeshes(M_PRIV *p)
{
    p->mesh_count = Output_GetSpriteTextureCount();
    p->meshes = Memory_Alloc(sizeof(*p->meshes) * p->mesh_count);
    for (int32_t i = 0; i < (int32_t)p->mesh_count; i++) {
        OUTPUT_MESH *mesh = Output_Mesh_Create();
        ROOM_VERTEX fake_vert = {};
        ROOM_SPRITE fake_sprite = { .texture = (uint16_t)i, .vertex = 0 };
        Output_Mesh_AddRoomSprite(mesh, &fake_sprite, &fake_vert);
        Output_Mesh_Seal(mesh);
        MeshBatcher_AddMesh(p->batcher, mesh);
        p->meshes[i] = mesh;
    }
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
