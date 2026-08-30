#include <trx/core/benchmark.h>
#include <trx/core/file.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/game_buf.h>
#include <trx/game/inject.h>
#include <trx/game/level/context.h>
#include <trx/game/level/sections/append.h>
#include <trx/game/level/sections/read.h>
#include <trx/game/objects.h>

static void M_ReadVertex(XYZ_16 *const vertex, TRX_FILE *const file)
{
    vertex->x = File_ReadS16(file);
    vertex->y = File_ReadS16(file);
    vertex->z = File_ReadS16(file);
}

static RESULT M_ReadFace(
    const LEVEL_FORMAT_LOADER *const loader, FACE *const face,
    const size_t vertex_count, const int32_t num_vertices, TRX_FILE *const file)
{
    face->vertex_count = vertex_count;
    for (size_t i = 0; i < vertex_count; i++) {
        face->vertices[i] = File_ReadU16(file);
        FAIL_IF(
            face->vertices[i] >= num_vertices,
            "face names vertex %d of the %d the mesh holds", face->vertices[i],
            num_vertices);
        face->texture_zw[i].z = 1.0f;
        face->texture_zw[i].w = 1.0f;
    }
    const uint16_t texture_idx = File_ReadU16(file);
    face->texture_idx = texture_idx & 0x7FFF;
    face->double_sided = (texture_idx & 0x8000) != 0;
    face->effects = 0;
    face->enable_reflections = false;
    face->semi_transparent = false;
    if (loader->game_version == 4) {
        face->effects = File_ReadU16(file);
    }
    return OK;
}

static RESULT M_ReadObjectMesh(OBJECT_MESH *const mesh, TRX_FILE *const file)
{
    const LEVEL_FORMAT_LOADER *const loader = Level_Context_Get()->loader;
    M_ReadVertex(&mesh->center, file);
    if (loader->game_version == 4) {
        mesh->radius = File_ReadS32(file);
    } else {
        mesh->radius = File_ReadS16(file);
        File_Skip(file, sizeof(int16_t));
    }

    mesh->enable_reflections = false;
    mesh->enable_caustics = false;
    mesh->depth_adjustment = 0.005;

    {
        mesh->num_vertices = File_ReadCountS16(file);
        mesh->vertices =
            GameBuf_Alloc(sizeof(XYZ_16) * mesh->num_vertices, GBUF_MESHES);
        for (int32_t i = 0; i < mesh->num_vertices; i++) {
            M_ReadVertex(&mesh->vertices[i], file);
        }
    }

    {
        mesh->num_lights = File_ReadS16(file);
        if (mesh->num_lights > 0) {
            mesh->lighting.normals =
                GameBuf_Alloc(sizeof(XYZ_16) * mesh->num_lights, GBUF_MESHES);
            for (int32_t i = 0; i < mesh->num_lights; i++) {
                M_ReadVertex(&mesh->lighting.normals[i], file);
            }
        } else {
            mesh->lighting.lights = GameBuf_Alloc(
                sizeof(int16_t) * ABS(mesh->num_lights), GBUF_MESHES);
            for (int32_t i = 0; i < ABS(mesh->num_lights); i++) {
                mesh->lighting.lights[i] = File_ReadS16(file);
            }
        }
    }

    {
        const int32_t quad_face_size = loader->game_version == 4 ? 12 : 10;
        const int32_t tri_face_size = loader->game_version == 4 ? 10 : 8;
        mesh->tex_face4s.count = File_ReadCountS16(file);
        size_t pos = File_Pos(file);
        File_Skip(file, quad_face_size * mesh->tex_face4s.count);
        mesh->tex_face3s.count = File_ReadCountS16(file);
        File_Skip(file, tri_face_size * mesh->tex_face3s.count);
        if (loader->game_version == 4) {
            mesh->flat_face4s.count = 0;
            mesh->flat_face3s.count = 0;
        } else {
            mesh->flat_face4s.count = File_ReadCountS16(file);
            File_Skip(file, quad_face_size * mesh->flat_face4s.count);
            mesh->flat_face3s.count = File_ReadCountS16(file);
        }
        File_Seek(file, pos, FILE_SEEK_SET);

        const int32_t total_faces = mesh->tex_face4s.count
            + mesh->tex_face3s.count + mesh->flat_face4s.count
            + mesh->flat_face3s.count;
        FAIL_IF(
            total_faces > INT16_MAX,
            "mesh holds %d faces, more than the %d "
            "one may",
            total_faces, INT16_MAX);

        mesh->tex_faces.count = mesh->tex_face4s.count + mesh->tex_face3s.count;
        mesh->flat_faces.count =
            mesh->flat_face4s.count + mesh->flat_face3s.count;
        mesh->all_faces.count = mesh->tex_faces.count + mesh->flat_faces.count;
        FACE *face_ptr =
            GameBuf_Alloc(sizeof(FACE) * mesh->all_faces.count, GBUF_MESHES);

        mesh->all_faces.data = face_ptr;
        mesh->tex_faces.data = face_ptr;
        mesh->tex_face4s.data = face_ptr;
        for (int32_t i = 0; i < mesh->tex_face4s.count; i++) {
            MUST(M_ReadFace(loader, face_ptr++, 4, mesh->num_vertices, file));
        }
        File_Skip(file, 2);

        mesh->tex_face3s.data = face_ptr;
        for (int32_t i = 0; i < mesh->tex_face3s.count; i++) {
            MUST(M_ReadFace(loader, face_ptr++, 3, mesh->num_vertices, file));
        }
        if (loader->layout != LEVEL_FORMAT_LAYOUT_TR4) {
            File_Skip(file, 2);

            mesh->flat_faces.data = face_ptr;
            mesh->flat_face4s.data = face_ptr;
            for (int32_t i = 0; i < mesh->flat_face4s.count; i++) {
                MUST(M_ReadFace(
                    loader, face_ptr++, 4, mesh->num_vertices, file));
            }
            File_Skip(file, 2);

            mesh->flat_face3s.data = face_ptr;
            for (int32_t i = 0; i < mesh->flat_face3s.count; i++) {
                MUST(M_ReadFace(
                    loader, face_ptr++, 3, mesh->num_vertices, file));
            }
            File_Skip(file, 2);
        } else if (File_Pos(file) % 4 != 0) {
            File_Skip(file, 2);
        }
    }
    return OK;
}

RESULT Level_Section_ReadObjectMeshes(
    LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_meshes = File_ReadCountS32(file);
    LOG_INFO("object mesh data: %d", num_meshes);

    const size_t data_start_pos = File_Pos(file);
    File_Skip(file, num_meshes * sizeof(int16_t));

    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    info->mesh_ptr_count = File_ReadCountS32(file);
    LOG_INFO("object mesh offsets: %d", info->mesh_ptr_count);
    const int32_t alloc_size = info->mesh_ptr_count * sizeof(int32_t);
    int32_t *mesh_offsets = Memory_Alloc(alloc_size);
    File_ReadData(file, mesh_offsets, alloc_size);

    const size_t end_pos = File_Pos(file);
    File_Seek(file, data_start_pos, FILE_SEEK_SET);

    Object_InitialiseMeshes(
        info->mesh_ptr_count + Inject_GetDataCount(IDT_MESH_POINTERS));
    const RESULT mesh_result = Level_Section_AppendObjectMeshes(
        info->mesh_ptr_count, mesh_offsets, file);
    if (!IS_OK(mesh_result)) {
        Memory_FreePointer(&mesh_offsets);
        return mesh_result;
    }

    File_Seek(file, end_pos, FILE_SEEK_SET);
    Memory_FreePointer(&mesh_offsets);

    Benchmark_End(&benchmark, nullptr);
    return OK;
}

RESULT Level_Section_AppendObjectMeshes(
    const int32_t num_offsets, const int32_t *const offsets,
    TRX_FILE *const file)
{
#define L_ALIGN 2

    // Savegames identify meshes by their file pointer values divided by 2.
    // (Historically, meshes were stored in int16_t[] arrays, so the so-called
    // "pointers" are array indices into that layout.)
    //
    // Original level meshes work fine under this scheme, but injected meshes
    // are different, as they come from separate files and bring their own
    // pointer values. To prevent conflicts, calling
    // Level_Section_AppendObjectMeshes() for injected content must assign
    // unique pseudo-pointers.
    //
    // Rules for injected meshes:
    // - Pointers do not need to match real file offsets.
    // - They only need to be unique and preserve ordering.
    //
    // Only the original level data requires true offset congruence so that old
    // savegames remain compatible. For everything else, simple linear indexing
    // is sufficient.
    int32_t base_index = 0;
    if (Object_GetMeshCount() > 0) {
        // NOTE(Dash): Not assuming offsets are strictly increasing, so we scan
        // all meshes and pick the max.
        for (int32_t i = 0; i < Object_GetMeshCount(); i++) {
            base_index =
                MAX(base_index, Object_GetMeshOffset(Object_GetMesh(i)));
        }
        base_index += L_ALIGN;
    }

    // Construct and store distinct meshes only e.g. Lara's hips are referenced
    // by several pointers as a dummy mesh.
    VECTOR *const unique_offsets =
        Vector_CreateAtCapacity(sizeof(int32_t), num_offsets);
    int32_t pointer_map[num_offsets];
    for (int32_t i = 0; i < num_offsets; i++) {
        const int32_t pointer = offsets[i] + base_index;
        const int32_t index = Vector_IndexOf(unique_offsets, (void *)&pointer);
        if (index == -1) {
            pointer_map[i] = unique_offsets->count;
            Vector_Add(unique_offsets, (void *)&pointer);
        } else {
            pointer_map[i] = index;
        }
    }

    OBJECT_MESH *const meshes =
        GameBuf_Alloc(sizeof(OBJECT_MESH) * unique_offsets->count, GBUF_MESHES);
    size_t start_pos = File_Pos(file);
    for (int32_t i = 0; i < unique_offsets->count; i++) {
        const int32_t pointer = *(const int32_t *)Vector_Get(unique_offsets, i);
        File_Seek(file, start_pos + pointer - base_index, FILE_SEEK_SET);
        MUST(M_ReadObjectMesh(&meshes[i], file), "mesh %d", i);

        // The original data position is required for backward compatibility
        // with savegame files, specifically for Lara's mesh pointers.
        Object_SetMeshOffset(&meshes[i], pointer / L_ALIGN);
    }

    for (int32_t i = 0; i < num_offsets; i++) {
        Object_StoreMesh(&meshes[pointer_map[i]]);
    }
#undef L_ALIGN

    LOG_INFO("%d unique meshes constructed", unique_offsets->count);

    Vector_Free(unique_offsets);
    return OK;
}
