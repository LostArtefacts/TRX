#include "game/output/mesh_batcher/mesh.h"

#include "game/output/vertex_range.h"
#include "memory.h"

OUTPUT_MESH *Output_Mesh_Create(void)
{
    OUTPUT_MESH *const mesh = Memory_Alloc(sizeof(OUTPUT_MESH));
    Memory_ArenaReset(&mesh->allocator);
    mesh->vertices = Vector_Create(sizeof(OUTPUT_MESH_VERTEX));
    mesh->animated_vertices = Vector_Create(sizeof(OUTPUT_VERTEX_RANGE));
    mesh->transparent_faces = Vector_Create(sizeof(OUTPUT_MESH_FACE));
    mesh->opaque_vertex_indices = Vector_Create(sizeof(uint32_t));
    mesh->sealed = false;
    return mesh;
}

void Output_Mesh_Destroy(OUTPUT_MESH *const mesh)
{
    if (mesh->animated_vertices != nullptr) {
        Vector_Free(mesh->animated_vertices);
    }
    Vector_Free(mesh->vertices);
    if (mesh->transparent_faces != nullptr) {
        Vector_Free(mesh->transparent_faces);
    }
    if (mesh->opaque_vertex_indices != nullptr) {
        Vector_Free(mesh->opaque_vertex_indices);
    }
    Memory_ArenaFree(&mesh->allocator);
    Memory_Free(mesh);
}
