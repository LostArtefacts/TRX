#include <trx/core/file.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/debug.h>
#include <trx/game/const.h>
#include <trx/game/inject.h>
#include <trx/game/objects/common.h>

typedef struct {
    INJECTION_OBJECT_INFO obj_info;
    int16_t source_identifier;
    FACE_TYPE face_type;
    int16_t face_index;
    int32_t target_count;
    int16_t *targets;
} M_FACE_TEXTURE_EDIT;

typedef struct {
    int16_t index;
    XYZ_16 shift;
} M_VERTEX_EDIT;

typedef struct {
    FACE_TYPE face_type;
    int16_t face_index;
    uint16_t effects;
    bool reflective;
} M_FACE_EFFECTS_EDIT;

typedef struct {
    INJECTION_OBJECT_INFO obj_info;
    int16_t mesh_idx;
    XYZ_16 centre_shift;
    int32_t radius_shift;
    int32_t face_edit_count;
    int32_t vertex_edit_count;
    int32_t effects_edit_count;
    M_FACE_TEXTURE_EDIT *texture_edits;
    M_VERTEX_EDIT *vertex_edits;
    M_FACE_EFFECTS_EDIT *effects_edits;
} M_MESH_EDIT;

static BOUNDS_16 M_ReadBounds16(TRX_FILE *const file)
{
    BOUNDS_16 bounds = {};
    bounds.min.x = File_ReadS16(file);
    bounds.max.x = File_ReadS16(file);
    bounds.min.y = File_ReadS16(file);
    bounds.max.y = File_ReadS16(file);
    bounds.min.z = File_ReadS16(file);
    bounds.max.z = File_ReadS16(file);
    return bounds;
}

static uint16_t *M_GetMeshTexture(const M_FACE_TEXTURE_EDIT *const edit)
{
    const OBJECT *const obj = Object_Get(edit->obj_info.id);
    if (!obj->loaded) {
        return nullptr;
    }
    ASSERT(edit->source_identifier >= 0);
    ASSERT(edit->source_identifier < obj->mesh_count);

    const OBJECT_MESH *const mesh =
        Object_GetMesh(obj->mesh_idx + edit->source_identifier);

    if (edit->face_type == FT_TEXTURED_QUAD) {
        ASSERT(edit->face_index >= 0);
        ASSERT(edit->face_index < mesh->tex_face4s.count);
        FACE *const face = &mesh->tex_face4s.data[edit->face_index];
        return &face->texture_idx;
    }
    if (edit->face_type == FT_TEXTURED_TRIANGLE) {
        ASSERT(edit->face_index >= 0);
        ASSERT(edit->face_index < mesh->tex_face3s.count);
        FACE *const face = &mesh->tex_face3s.data[edit->face_index];
        return &face->texture_idx;
    }

    if (edit->face_type == FT_COLOURED_QUAD) {
        ASSERT(edit->face_index >= 0);
        ASSERT(edit->face_index < mesh->flat_face4s.count);
        FACE *const face = &mesh->flat_face4s.data[edit->face_index];
        return &face->palette_idx;
    }
    if (edit->face_type == FT_COLOURED_TRIANGLE) {
        ASSERT(edit->face_index >= 0);
        ASSERT(edit->face_index < mesh->flat_face3s.count);
        FACE *const face = &mesh->flat_face3s.data[edit->face_index];
        return &face->palette_idx;
    }

    return nullptr;
}

static void M_ApplyFaceEdit(
    const M_FACE_TEXTURE_EDIT *const edit, FACE *const faces,
    const int32_t face_count, const uint16_t texture)
{
    for (int32_t i = 0; i < edit->target_count; i++) {
        ASSERT(edit->targets[i] >= 0);
        ASSERT(edit->targets[i] < face_count);
        FACE *const face = &faces[edit->targets[i]];
        face->texture_idx = texture;
    }
}

static void M_ApplyMeshEdit(const M_MESH_EDIT *const edit)
{
    OBJECT_MESH *mesh;
    if (edit->obj_info.type == OBJ_TYPE_OBJECT) {
        const OBJECT *const obj = Object_Get(edit->obj_info.id);
        if (!obj->loaded) {
            return;
        }
        ASSERT(edit->mesh_idx >= 0);
        ASSERT(edit->mesh_idx < obj->mesh_count);

        mesh = Object_GetMesh(obj->mesh_idx + edit->mesh_idx);
    } else if (edit->obj_info.type == OBJ_TYPE_STATIC3D) {
        if (edit->obj_info.id < 0
            || edit->obj_info.id >= Object_GetStaticObjects3DCount()) {
            return;
        }
        const STATIC_OBJECT_3D *const obj =
            Object_Get3DStatic(edit->obj_info.id);
        mesh = Object_GetMesh(obj->mesh_idx);
    } else {
        LOG_WARNING("Invalid mesh edit type %d", edit->obj_info.type);
        return;
    }

    mesh->center.x += edit->centre_shift.x;
    mesh->center.y += edit->centre_shift.y;
    mesh->center.z += edit->centre_shift.z;
    mesh->radius += edit->radius_shift;

    for (int32_t i = 0; i < edit->vertex_edit_count; i++) {
        const M_VERTEX_EDIT *const vertex_edit = &edit->vertex_edits[i];
        if (vertex_edit->index < 0
            || vertex_edit->index >= mesh->num_vertices) {
            const int32_t object_id = edit->obj_info.type == OBJ_TYPE_OBJECT
                ? Object_IDToSlot(edit->obj_info.id)
                : edit->obj_info.id;
            LOG_ERROR(
                "Invalid mesh vertex edit: obj_type=%d obj_id=%d mesh_idx=%d "
                "vertex_idx=%d num_vertices=%d",
                edit->obj_info.type, object_id, edit->mesh_idx,
                vertex_edit->index, mesh->num_vertices);
        }
        ASSERT(vertex_edit->index >= 0);
        ASSERT(vertex_edit->index < mesh->num_vertices);
        XYZ_16 *const vertex = &mesh->vertices[vertex_edit->index];
        vertex->x += vertex_edit->shift.x;
        vertex->y += vertex_edit->shift.y;
        vertex->z += vertex_edit->shift.z;
    }

    // Find each face we are interested in and replace its texture
    // or palette reference with the one selected from each edit's
    // instructions.
    for (int32_t i = 0; i < edit->face_edit_count; i++) {
        const M_FACE_TEXTURE_EDIT *const face_edit = &edit->texture_edits[i];
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
            M_ApplyFaceEdit(
                face_edit, mesh->tex_face4s.data, mesh->tex_face4s.count,
                texture);
            break;
        case FT_TEXTURED_TRIANGLE:
            M_ApplyFaceEdit(
                face_edit, mesh->tex_face3s.data, mesh->tex_face3s.count,
                texture);
            break;
        case FT_COLOURED_QUAD:
            M_ApplyFaceEdit(
                face_edit, mesh->flat_face4s.data, mesh->flat_face4s.count,
                texture);
            break;
        case FT_COLOURED_TRIANGLE:
            M_ApplyFaceEdit(
                face_edit, mesh->flat_face3s.data, mesh->flat_face3s.count,
                texture);
            break;
        }
    }

    for (int32_t i = 0; i < edit->effects_edit_count; i++) {
        const M_FACE_EFFECTS_EDIT *const effects_edit = &edit->effects_edits[i];
        FACE *face = nullptr;
        switch (effects_edit->face_type) {
        case FT_TEXTURED_QUAD:
            face = &mesh->tex_face4s.data[effects_edit->face_index];
            break;
        case FT_TEXTURED_TRIANGLE:
            face = &mesh->tex_face3s.data[effects_edit->face_index];
            break;
        case FT_COLOURED_QUAD:
            face = &mesh->flat_face4s.data[effects_edit->face_index];
            break;
        case FT_COLOURED_TRIANGLE:
            face = &mesh->flat_face3s.data[effects_edit->face_index];
            break;
        default:
            continue;
        }

        face->effects = effects_edit->effects;
        face->enable_reflections = effects_edit->reflective;
    }
}

static void M_ReadTextureEdits(
    M_MESH_EDIT *const edit, const INJECTION *const injection)
{
    edit->face_edit_count = File_ReadS32(injection->fp);
    if (edit->face_edit_count <= 0) {
        return;
    }

    edit->texture_edits =
        Memory_Alloc(sizeof(M_FACE_TEXTURE_EDIT) * edit->face_edit_count);
    for (int32_t j = 0; j < edit->face_edit_count; j++) {
        M_FACE_TEXTURE_EDIT *const face_edit = &edit->texture_edits[j];
        face_edit->obj_info = Inject_ReadObjectPtr(injection);
        face_edit->source_identifier = File_ReadS16(injection->fp);
        face_edit->face_type = File_ReadS32(injection->fp);
        face_edit->face_index = File_ReadS16(injection->fp);

        face_edit->target_count = File_ReadS32(injection->fp);
        face_edit->targets =
            Memory_Alloc(sizeof(int16_t) * face_edit->target_count);
        File_ReadData(
            injection->fp, face_edit->targets,
            sizeof(int16_t) * face_edit->target_count);
    }
}

static void M_ReadVertexEdits(
    M_MESH_EDIT *const edit, const INJECTION *const injection)
{
    edit->vertex_edit_count = File_ReadS32(injection->fp);
    if (edit->vertex_edit_count <= 0) {
        return;
    }

    edit->vertex_edits =
        Memory_Alloc(sizeof(M_VERTEX_EDIT) * edit->vertex_edit_count);
    for (int32_t j = 0; j < edit->vertex_edit_count; j++) {
        M_VERTEX_EDIT *const vertex_edit = &edit->vertex_edits[j];
        vertex_edit->index = File_ReadS16(injection->fp);
        vertex_edit->shift.x = File_ReadS16(injection->fp);
        vertex_edit->shift.y = File_ReadS16(injection->fp);
        vertex_edit->shift.z = File_ReadS16(injection->fp);
    }
}

static void M_ReadEffectsEdits(
    M_MESH_EDIT *const edit, const INJECTION *const injection)
{
    if (injection->version < INJ_VERSION_9) {
        return;
    }

    edit->effects_edit_count = File_ReadS32(injection->fp);
    if (edit->effects_edit_count <= 0) {
        return;
    }

    edit->effects_edits =
        Memory_Alloc(sizeof(M_FACE_EFFECTS_EDIT) * edit->effects_edit_count);
    for (int32_t j = 0; j < edit->effects_edit_count; j++) {
        M_FACE_EFFECTS_EDIT *const effects_edit = &edit->effects_edits[j];
        effects_edit->face_type = File_ReadS32(injection->fp);
        effects_edit->face_index = File_ReadS16(injection->fp);
        effects_edit->effects = File_ReadU16(injection->fp);
        effects_edit->reflective = File_ReadU8(injection->fp);
    }
}

static void M_MeshEdits(
    const INJECTION_CONTEXT *const ctx, const INJECTION *const injection,
    const int32_t data_count)
{
    for (int32_t i = 0; i < data_count; i++) {
        M_MESH_EDIT edit = {};
        edit.obj_info = Inject_ReadObjectPtr(injection);
        edit.mesh_idx = File_ReadS16(injection->fp);
        edit.centre_shift.x = File_ReadS16(injection->fp);
        edit.centre_shift.y = File_ReadS16(injection->fp);
        edit.centre_shift.z = File_ReadS16(injection->fp);
        edit.radius_shift = File_ReadS32(injection->fp);

        M_ReadTextureEdits(&edit, injection);
        M_ReadVertexEdits(&edit, injection);
        M_ReadEffectsEdits(&edit, injection);

        if (ctx->mode != INJECTION_MODE_STATS) {
            M_ApplyMeshEdit(&edit);
        }

        for (int32_t j = 0; j < edit.face_edit_count; j++) {
            M_FACE_TEXTURE_EDIT *const face_edit = &edit.texture_edits[j];
            Memory_FreePointer(&face_edit->targets);
        }

        Memory_FreePointer(&edit.texture_edits);
        Memory_FreePointer(&edit.vertex_edits);
        Memory_FreePointer(&edit.effects_edits);
    }
}

static void M_Object3DEdits(
    const INJECTION_CONTEXT *const ctx, const INJECTION *const injection,
    const int32_t data_count)
{
    for (int32_t i = 0; i < data_count; i++) {
        const int32_t obj_id = File_ReadS32(injection->fp);
        const bool collidable = File_ReadU8(injection->fp) == 1;
        const bool visible = File_ReadU8(injection->fp) == 1;
        const BOUNDS_16 collision_bounds = M_ReadBounds16(injection->fp);
        const BOUNDS_16 draw_bounds = M_ReadBounds16(injection->fp);

        if (obj_id < 0 || obj_id >= Object_GetStaticObjects3DCount()) {
            continue;
        }
        STATIC_OBJECT_3D *const obj = Object_Get3DStatic(obj_id);
        if (!obj->loaded) {
            continue;
        }

        obj->collidable = collidable;
        obj->visible = visible;
        obj->collision_bounds = collision_bounds;
        obj->draw_bounds = draw_bounds;
    }
}

REGISTER_INJECT_EDITOR(IDT_MESH_EDITS, M_MeshEdits)
REGISTER_INJECT_EDITOR(IDT_OBJECT_3D_EDITS, M_Object3DEdits)
