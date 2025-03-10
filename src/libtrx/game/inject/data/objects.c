#include "debug.h"
#include "game/inject.h"
#include "game/objects/common.h"
#include "vector.h"

static void M_HandleObjectData(INJECTION_CHUNK chunk);
static void M_AlignTextureReferences(
    const OBJECT *obj, VECTOR *processed_meshes, int32_t tex_info_base);

static void M_HandleObjectData(const INJECTION_CHUNK chunk)
{
    ASSERT(chunk.num_blocks == 1);
    const INJECTION_DATA_TYPE data_type = VFile_ReadS32(chunk.injection->fp);
    ASSERT(data_type == IDT_OBJECTS);
    const int32_t data_count = VFile_ReadS32(chunk.injection->fp);
    VFile_Skip(chunk.injection->fp, sizeof(int32_t));

    VECTOR *const processed_meshes = Vector_Create(sizeof(OBJECT_MESH *));
    const LEVEL_INFO cached_info = Inject_GetCachedInfo();

    for (int32_t i = 0; i < data_count; i++) {
        const GAME_OBJECT_ID obj_id = VFile_ReadS32(chunk.injection->fp);
        OBJECT *const obj = Object_Get(obj_id);

        const int16_t num_meshes = VFile_ReadS16(chunk.injection->fp);
        const int16_t mesh_idx = VFile_ReadS16(chunk.injection->fp);
        const int32_t bone_idx =
            VFile_ReadS32(chunk.injection->fp) / ANIM_BONE_SIZE;

        // Omitted mesh data indicates that we wish to retain what's already
        // defined in level data to avoid duplicate texture page usage.
        if (!obj->loaded || num_meshes != 0) {
            obj->mesh_count = num_meshes;
            obj->mesh_idx = mesh_idx + cached_info.mesh_ptr_count;
            obj->bone_idx = bone_idx + cached_info.anims.bone_count;
        }

        obj->frame_ofs = VFile_ReadU32(chunk.injection->fp);
        obj->frame_base = nullptr;
        obj->anim_idx = VFile_ReadS16(chunk.injection->fp);
        if (obj->anim_idx != -1) {
            obj->anim_idx += cached_info.anims.anim_count;
        }
        obj->loaded = true;

        if (num_meshes != 0) {
            M_AlignTextureReferences(
                obj, processed_meshes, cached_info.textures.object_count);
        }
    }

    Vector_Free(processed_meshes);
}

static void M_AlignTextureReferences(
    const OBJECT *const obj, VECTOR *const processed_meshes,
    const int32_t tex_info_base)
{
    for (int32_t i = 0; i < obj->mesh_count; i++) {
        OBJECT_MESH *const mesh = Object_GetMesh(obj->mesh_idx + i);
        if (Vector_Contains(processed_meshes, (void *)mesh)) {
            continue;
        }
        Vector_Add(processed_meshes, (void *)mesh);

        for (int32_t j = 0; j < mesh->num_tex_face4s; j++) {
            mesh->tex_face4s[j].texture_idx += tex_info_base;
        }

        for (int32_t j = 0; j < mesh->num_tex_face3s; j++) {
            mesh->tex_face3s[j].texture_idx += tex_info_base;
        }

        for (int32_t j = 0; j < mesh->num_flat_face4s; j++) {
            FACE4 *const face = &mesh->flat_face4s[j];
            face->palette_idx = Inject_GetPaletteIndex(face->palette_idx);
        }

        for (int32_t j = 0; j < mesh->num_flat_face3s; j++) {
            FACE3 *const face = &mesh->flat_face3s[j];
            face->palette_idx = Inject_GetPaletteIndex(face->palette_idx);
        }
    }
}

REGISTER_INJECTOR(ICT_OBJECT_DATA, M_HandleObjectData)
