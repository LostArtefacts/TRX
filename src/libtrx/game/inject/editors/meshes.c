#include "game/const.h"
#include "game/inject.h"
#include "game/objects/common.h"
#include "log.h"
#include "memory.h"

typedef struct {
    GAME_OBJECT_ID object_id;
    int16_t source_identifier;
    FACE_TYPE face_type;
    int16_t face_index;
    int32_t target_count;
    int16_t *targets;
} FACE_EDIT;

typedef struct {
    int16_t index;
    XYZ_16 shift;
} VERTEX_EDIT;

typedef struct {
    GAME_OBJECT_ID object_id;
    int16_t mesh_idx;
    XYZ_16 centre_shift;
    int32_t radius_shift;
    int32_t face_edit_count;
    int32_t vertex_edit_count;
    FACE_EDIT *face_edits;
    VERTEX_EDIT *vertex_edits;
} MESH_EDIT;

static void M_MeshEdits(const INJECTION *injection, int32_t data_count);
static void M_ApplyMeshEdit(const MESH_EDIT *edit);
static void M_ApplyFace4Edit(
    const FACE_EDIT *edit, FACE4 *faces, uint16_t texture);
static void M_ApplyFace3Edit(
    const FACE_EDIT *edit, FACE3 *faces, uint16_t texture);
static uint16_t *M_GetMeshTexture(const FACE_EDIT *edit);

static void M_MeshEdits(
    const INJECTION *const injection, const int32_t data_count)
{
    for (int32_t i = 0; i < data_count; i++) {
        MESH_EDIT edit = {
            .object_id = VFile_ReadS32(injection->fp),
            .mesh_idx = VFile_ReadS16(injection->fp),
            .centre_shift.x = VFile_ReadS16(injection->fp),
            .centre_shift.y = VFile_ReadS16(injection->fp),
            .centre_shift.z = VFile_ReadS16(injection->fp),
            .radius_shift = VFile_ReadS32(injection->fp),
        };

        edit.face_edit_count = VFile_ReadS32(injection->fp);
        edit.face_edits =
            Memory_Alloc(sizeof(FACE_EDIT) * edit.face_edit_count);
        for (int32_t j = 0; j < edit.face_edit_count; j++) {
            FACE_EDIT *const face_edit = &edit.face_edits[j];
            face_edit->object_id = VFile_ReadS32(injection->fp);
            face_edit->source_identifier = VFile_ReadS16(injection->fp);
            face_edit->face_type = VFile_ReadS32(injection->fp);
            face_edit->face_index = VFile_ReadS16(injection->fp);

            face_edit->target_count = VFile_ReadS32(injection->fp);
            face_edit->targets =
                Memory_Alloc(sizeof(int16_t) * face_edit->target_count);
            VFile_Read(
                injection->fp, face_edit->targets,
                sizeof(int16_t) * face_edit->target_count);
        }

        edit.vertex_edit_count = VFile_ReadS32(injection->fp);
        edit.vertex_edits =
            Memory_Alloc(sizeof(VERTEX_EDIT) * edit.vertex_edit_count);
        for (int32_t i = 0; i < edit.vertex_edit_count; i++) {
            VERTEX_EDIT *vertex_edit = &edit.vertex_edits[i];
            vertex_edit->index = VFile_ReadS16(injection->fp);
            vertex_edit->shift.x = VFile_ReadS16(injection->fp);
            vertex_edit->shift.y = VFile_ReadS16(injection->fp);
            vertex_edit->shift.z = VFile_ReadS16(injection->fp);
        }

        M_ApplyMeshEdit(&edit);

        for (int32_t j = 0; j < edit.face_edit_count; j++) {
            FACE_EDIT *face_edit = &edit.face_edits[j];
            Memory_FreePointer(&face_edit->targets);
        }

        Memory_FreePointer(&edit.face_edits);
        Memory_FreePointer(&edit.vertex_edits);
    }
}

static void M_ApplyMeshEdit(const MESH_EDIT *const edit)
{
    OBJECT_MESH *mesh;
    if (edit->object_id < O_NUMBER_OF) {
        const OBJECT *const obj = Object_Get(edit->object_id);
        if (!obj->loaded) {
            return;
        }

        mesh = Object_GetMesh(obj->mesh_idx + edit->mesh_idx);
    } else if (edit->object_id - O_NUMBER_OF < MAX_STATIC_OBJECTS) {
        const STATIC_OBJECT_3D *const obj =
            Object_Get3DStatic(edit->object_id - O_NUMBER_OF);
        mesh = Object_GetMesh(obj->mesh_idx);
    } else {
        LOG_WARNING("Invalid object ID %d", edit->object_id);
        return;
    }

    mesh->center.x += edit->centre_shift.x;
    mesh->center.y += edit->centre_shift.y;
    mesh->center.z += edit->centre_shift.z;
    mesh->radius += edit->radius_shift;

    for (int32_t i = 0; i < edit->vertex_edit_count; i++) {
        const VERTEX_EDIT *const vertex_edit = &edit->vertex_edits[i];
        XYZ_16 *const vertex = &mesh->vertices[vertex_edit->index];
        vertex->x += vertex_edit->shift.x;
        vertex->y += vertex_edit->shift.y;
        vertex->z += vertex_edit->shift.z;
    }

    // Find each face we are interested in and replace its texture
    // or palette reference with the one selected from each edit's
    // instructions.
    for (int32_t i = 0; i < edit->face_edit_count; i++) {
        const FACE_EDIT *const face_edit = &edit->face_edits[i];
        uint16_t texture;
        if (face_edit->source_identifier < 0) {
            texture = Inject_GetPaletteIndex(-face_edit->source_identifier);
        } else {
            const uint16_t *const tex_ptr = M_GetMeshTexture(face_edit);
            if (tex_ptr == nullptr) {
                continue;
            }
            texture = *tex_ptr;
        }

        switch (face_edit->face_type) {
        case FT_TEXTURED_QUAD:
            M_ApplyFace4Edit(face_edit, mesh->tex_face4s, texture);
            break;
        case FT_TEXTURED_TRIANGLE:
            M_ApplyFace3Edit(face_edit, mesh->tex_face3s, texture);
            break;
        case FT_COLOURED_QUAD:
            M_ApplyFace4Edit(face_edit, mesh->flat_face4s, texture);
            break;
        case FT_COLOURED_TRIANGLE:
            M_ApplyFace3Edit(face_edit, mesh->flat_face3s, texture);
            break;
        }
    }
}

static void M_ApplyFace4Edit(
    const FACE_EDIT *const edit, FACE4 *const faces, const uint16_t texture)
{
    for (int32_t i = 0; i < edit->target_count; i++) {
        FACE4 *const face = &faces[edit->targets[i]];
        face->texture_idx = texture;
    }
}

static void M_ApplyFace3Edit(
    const FACE_EDIT *const edit, FACE3 *const faces, const uint16_t texture)
{
    for (int32_t i = 0; i < edit->target_count; i++) {
        FACE3 *const face = &faces[edit->targets[i]];
        face->texture_idx = texture;
    }
}

static uint16_t *M_GetMeshTexture(const FACE_EDIT *const edit)
{
    const OBJECT *const obj = Object_Get(edit->object_id);
    if (!obj->loaded) {
        return nullptr;
    }

    const OBJECT_MESH *const mesh =
        Object_GetMesh(obj->mesh_idx + edit->source_identifier);

    if (edit->face_type == FT_TEXTURED_QUAD) {
        FACE4 *const face = &mesh->tex_face4s[edit->face_index];
        return &face->texture_idx;
    }

    if (edit->face_type == FT_TEXTURED_TRIANGLE) {
        FACE3 *const face = &mesh->tex_face3s[edit->face_index];
        return &face->texture_idx;
    }

    if (edit->face_type == FT_COLOURED_QUAD) {
        FACE4 *const face = &mesh->flat_face4s[edit->face_index];
        return &face->palette_idx;
    }

    if (edit->face_type == FT_COLOURED_TRIANGLE) {
        FACE3 *const face = &mesh->flat_face3s[edit->face_index];
        return &face->palette_idx;
    }

    return nullptr;
}

REGISTER_INJECT_EDITOR(IDT_MESH_EDITS, M_MeshEdits)
