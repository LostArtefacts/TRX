#include <trx/game/output/mesh_batcher/batcher.h>

#include <trx/core/memory.h>
#include <trx/debug.h>
#include <trx/game/output.h>
#include <trx/game/output/lights/priv.h>
#include <trx/game/output/shaders/mesh.h>
#include <trx/game/output/uniforms.h>
#include <trx/game/output/utils.h>
#include <trx/game/output/vertex_range.h>
#include <trx/gl/utils.h>

#include <uthash.h>

// Order two values as a sign. The differences here are wider than the int the
// comparator returns, and a truncated difference orders qsort's input
// inconsistently rather than merely imprecisely.
#define M_COMPARE(a_, b_) (((a_) > (b_)) - ((a_) < (b_)))

typedef float M_MESH_SHADE;

typedef struct {
    XYZW_F pos;
    XYZW_F normal;
    OUTPUT_USHORT flags;
    RGBA_8888 color;
} M_MESH_GEOM;

typedef struct {
    OUTPUT_UVW uvw;
    OUTPUT_TEXTURE_SIZE texture_size;
    float trapezoid_ratio[2];
    float reflectivity;
    float uv_scroll[2];
} M_MESH_TEXTURE;

typedef struct M_MESH_BUF_BINDING {
    OUTPUT_MESH *mesh;
    M_MESH_GEOM *geom_data;
    M_MESH_TEXTURE *tex_data;
    M_MESH_SHADE *shade_data;
    bool needs_room_lights;
    bool needs_cpu_light;
    bool needs_object_light;
    bool needs_own_light;
    int32_t vertex_start;
    int32_t vertex_count;

    int32_t opaque_index_start;
    int32_t opaque_index_count;
    int32_t blend_add_index_start;
    int32_t blend_add_index_count;
    int32_t transparent_index_start;
    int32_t transparent_index_count;
    int32_t transparent_face_count;
    int32_t *transparent_face_index_starts;
    int32_t *transparent_face_index_counts;

    UT_hash_handle hh;
} M_MESH_BUF_BINDING;

// One sorted draw in the transparent pass. A baked entry draws from the baked
// buffers with no model matrix of its own; it is either a face, or a whole
// faded object, which is the one kind that arrives with its depth already
// worked out. Every other entry names a face of a mesh still sitting in the
// shared buffers.
typedef struct {
    // As wide as the matrix it comes from: the camera-space depth carries the
    // W2V_SHIFT scale, so a level deep enough would wrap a 32-bit key and sort
    // its far geometry as the nearest.
    int64_t sort_key;
    const MESH_INSTANCE *inst;
    const OUTPUT_MESH_FACE *face;
    int32_t index_start;
    int32_t index_count;
    // Which faded group the entry stands for, when it stands for one rather
    // than for a face. See M_IsGroupEntry.
    int32_t group;
    bool baked;
} M_FACE_SORT;

// The solid part of one baked instance, contiguous in the baked buffers so it
// draws in one go.
typedef struct {
    const MESH_INSTANCE *inst;
    int32_t index_start;
    int32_t index_count;
} M_BAKED_RANGE;

// One faded object: a run of solid ranges that lay down their depth together
// and are blended together, and the depth it all sorts at.
typedef struct {
    int32_t range_start;
    int32_t range_count;
    int64_t depth_total;
} M_BAKED_GROUP;

typedef struct MESH_BATCHER {
    SCENE_SOURCE source;

    int32_t vertex_count;

    VECTOR *bindings;
    M_MESH_BUF_BINDING *binding_map;
    VECTOR *staged[SCENE_PASS_COUNT];

    OUTPUT_MESH_SHADER *shader;
    GLuint vao;
    struct {
        GLuint geom;
        GLuint tex;
        GLuint shade;
    } vbo;

    VECTOR *transparent_sort; // M_FACE_SORT
    struct {
        GLuint opaque;
        GLuint transparent;
        GLuint blend_add;
    } ebo;

    int32_t opaque_total_indices;
    int32_t blend_add_total_indices;
    int32_t transparent_total_indices;
    bool layout_dirty;

    // An instance drawn at partial coverage is baked into world space once a
    // frame and drawn from here. Sharing one model matrix is what lets the
    // instances of an object be drawn as an object, and lets what stays
    // sorted interleave with anything else transparent for free.
    struct {
        GLuint vao;
        GLuint geom;
        GLuint tex;
        GLuint shade;
        GLuint ebo;
    } baked;
    VECTOR *baked_geom; // M_MESH_GEOM
    VECTOR *baked_tex; // M_MESH_TEXTURE
    VECTOR *baked_shade; // M_MESH_SHADE
    VECTOR *baked_indices; // uint32_t
    VECTOR *baked_ranges; // M_BAKED_RANGE
    VECTOR *baked_groups; // M_BAKED_GROUP
    // The instance baked last, which is what a new one is joined to: an
    // object is staged a mesh at a time, so a run of them at one coverage is
    // one object.
    const MESH_INSTANCE *bake_tail;
} MESH_BATCHER;

static M_MESH_BUF_BINDING *M_GetBinding(
    const MESH_BATCHER *const batcher, const OUTPUT_MESH *const mesh)
{
    M_MESH_BUF_BINDING *bind = nullptr;
    HASH_FIND_PTR(batcher->binding_map, &mesh, bind);
    return bind;
}

static void M_FillGeometry(
    M_MESH_GEOM *const geom, const OUTPUT_MESH_VERTEX *const vertex)
{
    geom->pos.x = vertex->pos.x;
    geom->pos.y = vertex->pos.y;
    geom->pos.z = vertex->pos.z;
    geom->pos.w = vertex->pos.w;
    geom->normal.x = vertex->normal.x;
    geom->normal.y = vertex->normal.y;
    geom->normal.z = vertex->normal.z;
    geom->normal.w = vertex->light_table_idx;
    geom->color = vertex->color;
    geom->flags = vertex->flags;
}

static void M_FillTexture(
    M_MESH_TEXTURE *const tex, const OUTPUT_MESH_VERTEX *const vertex)
{
    tex->reflectivity = vertex->reflectivity;
    if (vertex->uvw_idx < 0) {
        return;
    }
    tex->uvw = Output_Textures_GetUVW(vertex->uvw_idx);
    tex->texture_size = Output_Textures_GetAtlasSize(vertex->uvw_idx / 4);
    tex->trapezoid_ratio[0] = vertex->trapezoid_ratio[0];
    tex->trapezoid_ratio[1] = vertex->trapezoid_ratio[1];

    const OUTPUT_UV_SCROLL scroll =
        Output_Textures_GetUVScroll(vertex->uvw_idx / 4);
    tex->uv_scroll[0] = scroll.speed / 256.0f;
    tex->uv_scroll[1] = scroll.period / 256.0f;
}

static void M_FillShade(
    M_MESH_SHADE *const shade, const OUTPUT_MESH_VERTEX *const vertex)
{
    *shade = vertex->shade;
}

static void M_SyncRoom(
    const MESH_BATCHER *const batcher, const M_MESH_BUF_BINDING *const bind,
    const ROOM *const room)
{
    if (!bind->needs_room_lights) {
        return;
    }
    Output_Uniforms_UploadRoomLights(Output_GetUniforms(), room);
}

static void M_AnimateBinding(
    const MESH_BATCHER *const batcher, const M_MESH_BUF_BINDING *const bind)
{
    ASSERT(bind != nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, batcher->vbo.tex);
    const OUTPUT_MESH_VERTEX *const vertices =
        Vector_GetData(bind->mesh->vertices);
    for (int32_t i = 0; i < bind->mesh->animated_vertices->count; i++) {
        const OUTPUT_VERTEX_RANGE *const range =
            Vector_Get(bind->mesh->animated_vertices, i);
        for (int32_t j = range->vertex_start;
             j < range->vertex_start + range->vertex_count; j++) {
            M_FillTexture(&bind->tex_data[j], &vertices[j]);
        }
        TRX_GL_TRACK_DATA(
            glBufferSubData, GL_ARRAY_BUFFER,
            (bind->vertex_start + range->vertex_start) * sizeof(M_MESH_TEXTURE),
            range->vertex_count * sizeof(M_MESH_TEXTURE),
            &bind->tex_data[range->vertex_start]);
    }
}

static void M_UpdateMeshGeometry(
    const MESH_BATCHER *const batcher, const OUTPUT_MESH *const mesh)
{
    M_MESH_BUF_BINDING *const bind = M_GetBinding(batcher, mesh);
    const OUTPUT_MESH_VERTEX *const vertices = Vector_GetData(mesh->vertices);
    for (int32_t i = 0; i < bind->vertex_count; i++) {
        M_FillGeometry(&bind->geom_data[i], &vertices[i]);
    }
    TRX_GL_TRACK_SUBDATA(
        glBufferSubData, GL_ARRAY_BUFFER,
        bind->vertex_start * sizeof(M_MESH_GEOM),
        bind->vertex_count * sizeof(M_MESH_GEOM), bind->geom_data);
}

// An entry that names no face is not a face at all: it stands for a whole
// faded object, which is drawn in one go wherever it sorts.
static bool M_IsGroupEntry(const M_FACE_SORT *const sort_ptr)
{
    return sort_ptr->face == nullptr;
}

// Compare two faces by camera-space depth.
static int M_CompareFaceDepth(const void *const a, const void *const b)
{
    const M_FACE_SORT *const face_a = a;
    const M_FACE_SORT *const face_b = b;
    if (face_a->inst->sort_layer != face_b->inst->sort_layer) {
        return M_COMPARE(face_a->inst->sort_layer, face_b->inst->sort_layer);
    }
    if (face_a->sort_key != face_b->sort_key) {
        return M_COMPARE(face_b->sort_key, face_a->sort_key);
    }
    // The instances share one vector, so their addresses stand for the order
    // they were staged in.
    if (face_a->inst != face_b->inst) {
        return M_COMPARE(face_b->inst, face_a->inst);
    }
    // qsort orders entries it is told are equal however it likes, and an
    // instance can bring many faces at keys that meet. Settle them by where
    // they sit in the buffer, or the order they are drawn in changes from one
    // frame to the next.
    return M_COMPARE(face_b->index_start, face_a->index_start);
}

// Compute per-face view depth and sort the mesh's transparent ranges
// back-to-front.
static void M_SortTransparentFaces(const MESH_BATCHER *const batcher)
{
    const int n = batcher->transparent_sort->count;
    if (n == 0) {
        return;
    }
    M_FACE_SORT *const buf = Vector_GetData(batcher->transparent_sort);
    M_FACE_SORT *bptr = buf;
    for (int32_t i = 0; i < n; i++) {
        if (M_IsGroupEntry(bptr)) {
            // Not a face: its depth came from the instances it stands for.
            bptr++;
            continue;
        }
        // clang-format off
        bptr->sort_key = (
            bptr->inst->cwmatrix._20 * (int32_t)bptr->face->mesh_centroid.x +
            bptr->inst->cwmatrix._21 * (int32_t)bptr->face->mesh_centroid.y +
            bptr->inst->cwmatrix._22 * (int32_t)bptr->face->mesh_centroid.z +
            bptr->inst->cwmatrix._23);
        // clang-format on
        bptr++;
    }
    qsort(buf, n, sizeof(*buf), M_CompareFaceDepth);
}

static void M_DrawOpaqueVertices(
    const MESH_BATCHER *const batcher, const MESH_INSTANCE *const inst)
{
    M_MESH_BUF_BINDING *const bind = M_GetBinding(batcher, inst->mesh);
    const void *indices_offset =
        (void *)(intptr_t)(bind->opaque_index_start * sizeof(uint32_t));
    glDrawElementsBaseVertex(
        GL_TRIANGLES, bind->opaque_index_count, GL_UNSIGNED_INT,
        indices_offset, // Offset in EBO
        bind->vertex_start // Offset in VBO (baseVertex)
    );
    TRX_GL_CheckError();
    g_TRX_GL_Metrics.opaque_vert_count += bind->opaque_index_count;
}

static void M_DrawBlendAddVertices(
    const MESH_BATCHER *const batcher, const MESH_INSTANCE *const inst)
{
    M_MESH_BUF_BINDING *const bind = M_GetBinding(batcher, inst->mesh);
    const void *indices_offset =
        (void *)(intptr_t)(bind->blend_add_index_start * sizeof(uint32_t));
    glDrawElementsBaseVertex(
        GL_TRIANGLES, bind->blend_add_index_count, GL_UNSIGNED_INT,
        indices_offset, // Offset in EBO
        bind->vertex_start // Offset in VBO (baseVertex)
    );
    TRX_GL_CheckError();
    g_TRX_GL_Metrics.blend_add_vert_count += bind->blend_add_index_count;
}

static void M_DrawOpaqueInstance(
    MESH_BATCHER *const batcher, const MESH_INSTANCE *const inst)
{
    M_MESH_BUF_BINDING *const bind = M_GetBinding(batcher, inst->mesh);
    ASSERT(bind != nullptr);

    M_SyncRoom(batcher, bind, inst->room);
    if (bind->needs_object_light) {
        Output_Lights_UploadCPULight(&inst->light_info);
    } else if (bind->needs_own_light) {
        Output_Lights_UploadOwnLight(&inst->light_info);
    }
    Output_MeshShader_UploadModelMatrix(batcher->shader, &inst->wmatrix);
    Output_MeshShader_UploadTint(batcher->shader, inst->tint);

    if (inst->enable_scissor) {
        Output_EnableScissor(
            inst->scissor.x, inst->scissor.y, inst->scissor.width,
            inst->scissor.height);
    }

    Output_MeshShader_UploadWaterEffect(batcher->shader, inst->water_effect);
    if (inst->wibble && inst->wibble_fill) {
        Output_MeshShader_UploadWibbleEffect(batcher->shader, false);
        glDepthMask(GL_FALSE);
        M_DrawOpaqueVertices(batcher, inst);
        glDepthMask(GL_TRUE);
    }
    Output_MeshShader_UploadWibbleEffect(batcher->shader, inst->wibble);
    M_DrawOpaqueVertices(batcher, inst);

    if (inst->enable_scissor) {
        Output_DisableScissor();
    }
}

static void M_DrawBlendAddInstance(
    MESH_BATCHER *const batcher, const MESH_INSTANCE *const inst)
{
    M_MESH_BUF_BINDING *const bind = M_GetBinding(batcher, inst->mesh);
    ASSERT(bind != nullptr);

    M_SyncRoom(batcher, bind, inst->room);
    if (bind->needs_object_light) {
        Output_Lights_UploadCPULight(&inst->light_info);
    } else if (bind->needs_own_light) {
        Output_Lights_UploadOwnLight(&inst->light_info);
    }
    Output_MeshShader_UploadModelMatrix(batcher->shader, &inst->wmatrix);
    Output_MeshShader_UploadTint(batcher->shader, inst->tint);
    Output_MeshShader_UploadWaterEffect(batcher->shader, inst->water_effect);
    Output_MeshShader_UploadWibbleEffect(batcher->shader, false);

    if (inst->enable_scissor) {
        Output_EnableScissor(
            inst->scissor.x, inst->scissor.y, inst->scissor.width,
            inst->scissor.height);
    }
    M_DrawBlendAddVertices(batcher, inst);
    if (inst->enable_scissor) {
        Output_DisableScissor();
    }
}

// The matrix carries the W2V_SHIFT scale, and the shader divides it out when
// it uploads. Baking has to land on the same place the shader would have put
// the vertex, so it divides it out too.
static XYZ_F M_TransformPoint(
    const MATRIX *const m, const XYZW_F *const pos, const bool translate)
{
    const double x = pos->x;
    const double y = pos->y;
    const double z = pos->z;
    const double w = translate ? 1.0 : 0.0;
    const double scale = 1.0 / (double)(1 << W2V_SHIFT);
    return (XYZ_F) {
        .x = (float)((m->_00 * x + m->_01 * y + m->_02 * z + m->_03 * w)
                     * scale),
        .y = (float)((m->_10 * x + m->_11 * y + m->_12 * z + m->_13 * w)
                     * scale),
        .z = (float)((m->_20 * x + m->_21 * y + m->_22 * z + m->_23 * w)
                     * scale),
    };
}

static int32_t M_BakeIndices(
    MESH_BATCHER *const batcher, const int32_t vertex_base,
    const uint32_t *const indices, const int32_t index_count)
{
    const int32_t index_start = batcher->baked_indices->count;
    for (int32_t i = 0; i < index_count; i++) {
        const uint32_t index = vertex_base + indices[i];
        Vector_Add(batcher->baked_indices, &index);
    }
    return index_start;
}

// The object a faded instance belongs to. An object's meshes are staged one
// after another at the one coverage, so a run of them is an object. Two that
// happen to run together only share where they sort - each still settles what
// of itself shows with its own depth.
static int32_t M_GetBakeGroup(
    MESH_BATCHER *const batcher, const MESH_INSTANCE *const inst)
{
    const MESH_INSTANCE *const tail = batcher->bake_tail;
    const bool joins = tail != nullptr && batcher->baked_groups->count > 0
        && tail->room == inst->room && tail->tint.a == inst->tint.a
        && tail->sort_layer == inst->sort_layer;
    batcher->bake_tail = inst;
    if (!joins) {
        Vector_Add(
            batcher->baked_groups,
            &(M_BAKED_GROUP) {
                .range_start = batcher->baked_ranges->count,
            });
    }
    return batcher->baked_groups->count - 1;
}

// Put an instance's vertices where the shader would have put them, so that
// what is drawn from them needs no model matrix of its own.
static void M_BakeInstance(
    MESH_BATCHER *const batcher, const M_MESH_BUF_BINDING *const bind,
    const MESH_INSTANCE *const inst)
{
    const int32_t vertex_base = batcher->baked_geom->count;
    for (int32_t i = 0; i < bind->vertex_count; i++) {
        M_MESH_GEOM geom = bind->geom_data[i];
        const XYZ_F pos = M_TransformPoint(&inst->wmatrix, &geom.pos, true);
        const XYZ_F normal =
            M_TransformPoint(&inst->wmatrix, &geom.normal, false);
        geom.pos.x = pos.x;
        geom.pos.y = pos.y;
        geom.pos.z = pos.z;
        geom.normal.x = normal.x;
        geom.normal.y = normal.y;
        geom.normal.z = normal.z;
        Vector_Add(batcher->baked_geom, &geom);
        Vector_Add(batcher->baked_tex, &bind->tex_data[i]);
        Vector_Add(batcher->baked_shade, &bind->shade_data[i]);
    }

    // The solid part goes down whole. Which of it you can see is settled by
    // the depth buffer, so it needs no order among itself.
    const int32_t group_idx = M_GetBakeGroup(batcher, inst);
    M_BAKED_GROUP *const group = Vector_Get(batcher->baked_groups, group_idx);
    group->depth_total += inst->cwmatrix._23;
    const int32_t opaque_count = inst->mesh->opaque_vertex_indices->count;
    if (opaque_count > 0) {
        const int32_t index_start = M_BakeIndices(
            batcher, vertex_base,
            Vector_GetData(inst->mesh->opaque_vertex_indices), opaque_count);
        Vector_Add(
            batcher->baked_ranges,
            &(M_BAKED_RANGE) {
                .inst = inst,
                .index_start = index_start,
                .index_count = opaque_count,
            });
        group->range_count++;
    }

    // The faces that were already transparent stay sorted: they are see
    // through in their own right, so the solid surface behind them has to
    // show, and one of them cannot stand in for another.
    for (int32_t i = 0; i < inst->mesh->transparent_faces->count; i++) {
        const OUTPUT_MESH_FACE *const face =
            Vector_Get(inst->mesh->transparent_faces, i);
        const int32_t index_start = M_BakeIndices(
            batcher, vertex_base, (const uint32_t *)face->vertex_indices,
            face->vertex_count);
        Vector_Add(
            batcher->transparent_sort,
            &(M_FACE_SORT) {
                .inst = inst,
                .face = face,
                .index_start = index_start,
                .index_count = face->vertex_count,
                .baked = true,
            });
    }
}

static void M_UploadBaked(const MESH_BATCHER *const batcher)
{
    if (batcher->baked_geom->count == 0) {
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, batcher->baked.geom);
    TRX_GL_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER,
        batcher->baked_geom->count * sizeof(M_MESH_GEOM),
        Vector_GetData(batcher->baked_geom), GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, batcher->baked.tex);
    TRX_GL_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER,
        batcher->baked_tex->count * sizeof(M_MESH_TEXTURE),
        Vector_GetData(batcher->baked_tex), GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, batcher->baked.shade);
    TRX_GL_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER,
        batcher->baked_shade->count * sizeof(M_MESH_SHADE),
        Vector_GetData(batcher->baked_shade), GL_STREAM_DRAW);

    // The element binding belongs to whichever vertex array is current, so
    // the baked one goes on first and keeps the change to itself.
    glBindVertexArray(batcher->baked.vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batcher->baked.ebo);
    TRX_GL_TRACK_DATA(
        glBufferData, GL_ELEMENT_ARRAY_BUFFER,
        batcher->baked_indices->count * sizeof(uint32_t),
        Vector_GetData(batcher->baked_indices), GL_STREAM_DRAW);
}

static void M_OpaquePass(MESH_BATCHER *const batcher)
{
    VECTOR *const staged = batcher->staged[SCENE_PASS_OPAQUE];

    glBindVertexArray(batcher->vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batcher->ebo.opaque);
    for (int32_t i = 0; i < staged->count; i++) {
        MESH_INSTANCE *const inst = Vector_Get(staged, i);
        const M_MESH_BUF_BINDING *const bind =
            M_GetBinding(batcher, inst->mesh);

        // Face opacity is fixed when the mesh is built, so the routing has to
        // happen per instance: at partial coverage the whole instance is
        // baked, and it leaves this pass entirely.
        if (inst->tint.a < 1.0f) {
            M_BakeInstance(batcher, bind, inst);
            continue;
        }

        if (inst->mesh->opaque_vertex_indices->count != 0) {
            Output_AdjustDepth(0.0f, inst->depth_adjust * 2.0f / 0.005f);
            M_DrawOpaqueInstance(batcher, inst);
        }

        // Accumulate transparent polygons and faces.
        for (int32_t j = 0; j < inst->mesh->transparent_faces->count; j++) {
            Vector_Add(
                batcher->transparent_sort,
                &(M_FACE_SORT) {
                    .inst = inst,
                    .face = Vector_Get(inst->mesh->transparent_faces, j),
                    .index_start = bind->transparent_index_start
                        + bind->transparent_face_index_starts[j],
                    .index_count = bind->transparent_face_index_counts[j],
                });
        }
    }

    Output_AdjustDepth(0.0f, 0.0f);
}

// Put an instance's state up. The bake took its matrix away, so an item's
// instances of one object differ in nothing, and the uploads below drop what
// they already hold.
static void M_ApplyBakedState(
    MESH_BATCHER *const batcher, const MESH_INSTANCE *const inst,
    const M_MESH_BUF_BINDING *const bind, const MESH_INSTANCE **const prev)
{
    if (*prev == nullptr
        || memcmp(
               &(*prev)->light_info, &inst->light_info,
               sizeof(inst->light_info))
            != 0) {
        if (bind->needs_object_light) {
            Output_Lights_UploadCPULight(&inst->light_info);
        } else if (bind->needs_own_light) {
            Output_Lights_UploadOwnLight(&inst->light_info);
        }
    }
    if (*prev == nullptr || (*prev)->room != inst->room) {
        M_SyncRoom(batcher, bind, inst->room);
    }
    Output_MeshShader_UploadModelMatrix(batcher->shader, &g_IDMatrix);
    Output_MeshShader_UploadTint(batcher->shader, inst->tint);
    Output_MeshShader_UploadWaterEffect(batcher->shader, inst->water_effect);
    Output_MeshShader_UploadWibbleEffect(batcher->shader, inst->wibble);
    Output_AdjustDepth(0.0f, inst->depth_adjust * 2.0f / 0.005f);
    *prev = inst;
}

static void M_DrawBakedGroupRanges(
    MESH_BATCHER *const batcher, const M_BAKED_GROUP *const group)
{
    const MESH_INSTANCE *prev = nullptr;
    for (int32_t i = 0; i < group->range_count; i++) {
        const M_BAKED_RANGE *const range =
            Vector_Get(batcher->baked_ranges, group->range_start + i);
        const M_MESH_BUF_BINDING *const bind =
            M_GetBinding(batcher, range->inst->mesh);
        ASSERT(bind != nullptr);
        M_ApplyBakedState(batcher, range->inst, bind, &prev);
        glDrawElements(
            GL_TRIANGLES, range->index_count, GL_UNSIGNED_INT,
            (void *)(intptr_t)(range->index_start * sizeof(uint32_t)));
        g_TRX_GL_Metrics.trans_vert_count += range->index_count;
    }
}

// Geometry at partial coverage is one object at one coverage, not a heap of
// see-through faces. Sorting its faces cannot order a part against another it
// is buried in, so the depth buffer does it instead: the solid part lays down
// its depth, and then only the surface that depth kept is blended, once. It
// happens where the object sorts, so what is in front of it still comes
// after.
static void M_DrawBakedGroup(
    MESH_BATCHER *const batcher, const int32_t group_idx)
{
    const M_BAKED_GROUP *const group =
        Vector_Get(batcher->baked_groups, group_idx);
    if (group->range_count == 0) {
        return;
    }

    Output_MeshShader_UploadAlphaDiscard(batcher->shader, true);

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    M_DrawBakedGroupRanges(batcher, group);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_EQUAL);
    glEnable(GL_BLEND);
    M_DrawBakedGroupRanges(batcher, group);

    glDepthFunc(GL_LESS);
    Output_MeshShader_UploadAlphaDiscard(batcher->shader, false);
}

// One entry per object, so it takes its place among the transparent faces the
// way anything else does.
static void M_StageBakedGroups(MESH_BATCHER *const batcher)
{
    for (int32_t i = 0; i < batcher->baked_groups->count; i++) {
        const M_BAKED_GROUP *const group = Vector_Get(batcher->baked_groups, i);
        if (group->range_count == 0) {
            continue;
        }
        const M_BAKED_RANGE *const first =
            Vector_Get(batcher->baked_ranges, group->range_start);
        Vector_Add(
            batcher->transparent_sort,
            &(M_FACE_SORT) {
                .sort_key = group->depth_total / group->range_count,
                .inst = first->inst,
                .face = nullptr,
                .group = i,
                .baked = true,
            });
    }
}

static void M_BlendPass(MESH_BATCHER *const batcher, const SCENE_PASS pass)
{
    VECTOR *const staged = batcher->staged[pass];

    glBindVertexArray(batcher->vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batcher->ebo.blend_add);
    for (int32_t i = 0; i < staged->count; i++) {
        const MESH_INSTANCE *const inst = Vector_Get(staged, i);
        const M_MESH_BUF_BINDING *const bind =
            M_GetBinding(batcher, inst->mesh);

        if (inst->mesh->blend_add_vertex_indices->count != 0) {
            Output_AdjustDepth(0.0f, inst->depth_adjust * 2.0f / 0.005f);
            M_DrawBlendAddInstance(batcher, inst);
        }
    }

    Output_AdjustDepth(0.0f, 0.0f);
}

static void M_TransparentPass(MESH_BATCHER *const batcher)
{
    if (batcher->transparent_sort->count == 0) {
        return;
    }

    M_UploadBaked(batcher);

    GLuint bound_vao = 0;
    GLuint bound_ebo = 0;

    const MESH_INSTANCE *inst = nullptr;

    for (int32_t i = 0; i < batcher->transparent_sort->count; i++) {
        const M_FACE_SORT *const sort_ptr =
            Vector_Get(batcher->transparent_sort, i);

        if (!M_IsGroupEntry(sort_ptr) && sort_ptr->index_count == 0) {
            continue;
        }

        const GLuint vao = sort_ptr->baked ? batcher->baked.vao : batcher->vao;
        if (vao != bound_vao) {
            glBindVertexArray(vao);
            bound_vao = vao;
            bound_ebo = 0;
        }
        const GLuint ebo =
            sort_ptr->baked ? batcher->baked.ebo : batcher->ebo.transparent;
        if (ebo != bound_ebo) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            bound_ebo = ebo;
        }

        if (M_IsGroupEntry(sort_ptr)) {
            M_DrawBakedGroup(batcher, sort_ptr->group);
            // The group put its own state up as it went.
            inst = nullptr;
            continue;
        }

        if (sort_ptr->inst != inst) {
            inst = sort_ptr->inst;
            const M_MESH_BUF_BINDING *const bind =
                M_GetBinding(batcher, inst->mesh);
            ASSERT(bind != nullptr);
            if (bind->needs_object_light) {
                Output_Lights_UploadCPULight(&inst->light_info);
            } else if (bind->needs_own_light) {
                Output_Lights_UploadOwnLight(&inst->light_info);
            }
            Output_MeshShader_UploadModelMatrix(
                batcher->shader,
                sort_ptr->baked ? &g_IDMatrix : &inst->wmatrix);
            Output_MeshShader_UploadTint(batcher->shader, inst->tint);
            Output_MeshShader_UploadWaterEffect(
                batcher->shader, inst->water_effect);
            Output_MeshShader_UploadWibbleEffect(batcher->shader, inst->wibble);
            Output_AdjustDepth(0.0f, inst->depth_adjust * 2.0f / 0.005f);
            M_SyncRoom(batcher, bind, inst->room);
        }

        // indices live in the EBO starting at index_start
        glDrawElements(
            GL_TRIANGLES, sort_ptr->index_count, GL_UNSIGNED_INT,
            (void *)(intptr_t)(sort_ptr->index_start * sizeof(uint32_t)));

        g_TRX_GL_Metrics.trans_vert_count += sort_ptr->index_count;
    }

    Output_AdjustDepth(0.0f, 0.0f);
}

static void M_RenderBegin(const SCENE_SOURCE *const source)
{
    MESH_BATCHER *const batcher = source->priv;
    for (int32_t pass = 0; pass < SCENE_PASS_COUNT; pass++) {
        Vector_Clear(batcher->staged[pass]);
    }
    Vector_Clear(batcher->transparent_sort);
    Vector_Clear(batcher->baked_geom);
    Vector_Clear(batcher->baked_tex);
    Vector_Clear(batcher->baked_shade);
    Vector_Clear(batcher->baked_indices);
    Vector_Clear(batcher->baked_ranges);
    Vector_Clear(batcher->baked_groups);
    batcher->bake_tail = nullptr;
}

static void M_RenderPass(
    const SCENE_SOURCE *const source, const SCENE_PASS pass)
{
    MESH_BATCHER *const batcher = source->priv;

    if (pass == SCENE_PASS_OPAQUE) {
        M_OpaquePass(batcher);
    } else if (pass == SCENE_PASS_TRANSPARENT) {
        M_StageBakedGroups(batcher);
        M_SortTransparentFaces(batcher);
        M_TransparentPass(batcher);
    } else if (pass == SCENE_PASS_BLEND_SUB) {
        M_BlendPass(batcher, pass);
    } else if (pass == SCENE_PASS_BLEND_ADD) {
        M_BlendPass(batcher, pass);
    }
}

static bool M_IsDirty(const SCENE_SOURCE *const source, const SCENE_PASS pass)
{
    const MESH_BATCHER *const batcher = source->priv;
    return batcher->staged[pass]->count > 0
        || (batcher->transparent_sort->count > 0
            && pass == SCENE_PASS_TRANSPARENT);
}

static void M_RecalculateLayout(MESH_BATCHER *const batcher)
{
    batcher->vertex_count = 0;
    batcher->opaque_total_indices = 0;
    batcher->blend_add_total_indices = 0;
    batcher->transparent_total_indices = 0;

    for (int32_t i = 0; i < batcher->bindings->count; i++) {
        M_MESH_BUF_BINDING *const bind =
            *(M_MESH_BUF_BINDING **)Vector_Get(batcher->bindings, i);

        bind->vertex_start = batcher->vertex_count;
        batcher->vertex_count += bind->vertex_count;

        bind->opaque_index_start = batcher->opaque_total_indices;
        batcher->opaque_total_indices += bind->opaque_index_count;

        bind->blend_add_index_start = batcher->blend_add_total_indices;
        batcher->blend_add_total_indices += bind->blend_add_index_count;

        bind->transparent_index_start = batcher->transparent_total_indices;
        batcher->transparent_total_indices += bind->transparent_index_count;
    }
    batcher->layout_dirty = false;
}

static void M_AnimateTextures(const SCENE_SOURCE *const source)
{
    MESH_BATCHER *const batcher = source->priv;
    for (int32_t i = 0; i < batcher->bindings->count; i++) {
        M_MESH_BUF_BINDING *const bind =
            *(M_MESH_BUF_BINDING **)Vector_Get(batcher->bindings, i);
        M_AnimateBinding(batcher, bind);
    }
}

static void M_SetupVertexArray(
    const GLuint vao, const GLuint geom, const GLuint tex, const GLuint shade)
{
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, geom);

    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_POS);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_NORMAL);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_FLAGS);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_COLOR);
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_POS, 4, GL_FLOAT, GL_FALSE, sizeof(M_MESH_GEOM),
        (void *)(intptr_t)offsetof(M_MESH_GEOM, pos));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_NORMAL, 4, GL_FLOAT, GL_FALSE, sizeof(M_MESH_GEOM),
        (void *)(intptr_t)offsetof(M_MESH_GEOM, normal));
    glVertexAttribIPointer(
        OUTPUT_MESH_ATTR_FLAGS, 1, OUTPUT_USHORT_GL, sizeof(M_MESH_GEOM),
        (void *)(intptr_t)offsetof(M_MESH_GEOM, flags));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE,
        sizeof(M_MESH_GEOM), (void *)(intptr_t)offsetof(M_MESH_GEOM, color));

    glBindBuffer(GL_ARRAY_BUFFER, tex);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_UVW);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_TEXTURE_SIZE);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_TRAPEZOID_RATIO);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_REFLECTIVITY);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_UV_SCROLL);
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_UVW, 3, GL_FLOAT, GL_FALSE, sizeof(M_MESH_TEXTURE),
        (void *)(intptr_t)offsetof(M_MESH_TEXTURE, uvw));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_TEXTURE_SIZE, 4, GL_FLOAT, GL_FALSE,
        sizeof(M_MESH_TEXTURE),
        (void *)(intptr_t)offsetof(M_MESH_TEXTURE, texture_size));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_TRAPEZOID_RATIO, 2, GL_FLOAT, GL_FALSE,
        sizeof(M_MESH_TEXTURE),
        (void *)(intptr_t)offsetof(M_MESH_TEXTURE, trapezoid_ratio));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_REFLECTIVITY, 1, GL_FLOAT, GL_FALSE,
        sizeof(M_MESH_TEXTURE),
        (void *)(intptr_t)offsetof(M_MESH_TEXTURE, reflectivity));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_UV_SCROLL, 2, GL_FLOAT, GL_FALSE,
        sizeof(M_MESH_TEXTURE),
        (void *)(intptr_t)offsetof(M_MESH_TEXTURE, uv_scroll));

    glBindBuffer(GL_ARRAY_BUFFER, shade);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_SHADE);
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_SHADE, 1, GL_FLOAT, GL_FALSE, sizeof(M_MESH_SHADE), 0);
}

MESH_BATCHER *MeshBatcher_Create(void)
{
    MESH_BATCHER *const batcher = Memory_Alloc(sizeof(MESH_BATCHER));
    batcher->shader = Output_GetMeshShader();
    batcher->bindings = Vector_Create(sizeof(OUTPUT_MESH *));
    batcher->binding_map = nullptr;
    for (int32_t pass = 0; pass < SCENE_PASS_COUNT; pass++) {
        batcher->staged[pass] = Vector_Create(sizeof(MESH_INSTANCE));
    }
    batcher->source.render_begin = M_RenderBegin;
    batcher->source.render_pass = M_RenderPass;
    batcher->source.is_dirty = M_IsDirty;
    batcher->source.animate_textures = M_AnimateTextures;
    batcher->source.priv = batcher;

    batcher->transparent_sort = Vector_Create(sizeof(M_FACE_SORT));
    batcher->layout_dirty = true;

    batcher->baked_geom = Vector_Create(sizeof(M_MESH_GEOM));
    batcher->baked_tex = Vector_Create(sizeof(M_MESH_TEXTURE));
    batcher->baked_shade = Vector_Create(sizeof(M_MESH_SHADE));
    batcher->baked_indices = Vector_Create(sizeof(uint32_t));
    batcher->baked_ranges = Vector_Create(sizeof(M_BAKED_RANGE));
    batcher->baked_groups = Vector_Create(sizeof(M_BAKED_GROUP));

    glGenVertexArrays(1, &batcher->vao);
    glGenBuffers(3, &batcher->vbo.geom);

    M_SetupVertexArray(
        batcher->vao, batcher->vbo.geom, batcher->vbo.tex, batcher->vbo.shade);

    glGenBuffers(3, &batcher->ebo.opaque);

    glGenVertexArrays(1, &batcher->baked.vao);
    glGenBuffers(3, &batcher->baked.geom);
    glGenBuffers(1, &batcher->baked.ebo);
    M_SetupVertexArray(
        batcher->baked.vao, batcher->baked.geom, batcher->baked.tex,
        batcher->baked.shade);

    return batcher;
}

void MeshBatcher_Destroy(MESH_BATCHER *const batcher)
{
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (batcher->vao != 0) {
        glDeleteVertexArrays(1, &batcher->vao);
        batcher->vao = 0;
    }
    if (batcher->vbo.geom != 0) {
        glDeleteBuffers(3, &batcher->vbo.geom);
        batcher->vbo.geom = 0;
        batcher->vbo.tex = 0;
        batcher->vbo.shade = 0;
    }
    if (batcher->ebo.opaque != 0) {
        glDeleteBuffers(3, &batcher->ebo.opaque);
        batcher->ebo.opaque = 0;
        batcher->ebo.transparent = 0;
        batcher->ebo.blend_add = 0;
    }
    if (batcher->baked.vao != 0) {
        glDeleteVertexArrays(1, &batcher->baked.vao);
        batcher->baked.vao = 0;
    }
    if (batcher->baked.geom != 0) {
        glDeleteBuffers(3, &batcher->baked.geom);
        batcher->baked.geom = 0;
        batcher->baked.tex = 0;
        batcher->baked.shade = 0;
    }
    if (batcher->baked.ebo != 0) {
        glDeleteBuffers(1, &batcher->baked.ebo);
        batcher->baked.ebo = 0;
    }
    Vector_Free(batcher->baked_geom);
    Vector_Free(batcher->baked_tex);
    Vector_Free(batcher->baked_shade);
    Vector_Free(batcher->baked_indices);
    Vector_Free(batcher->baked_ranges);
    Vector_Free(batcher->baked_groups);
    batcher->baked_ranges = nullptr;
    batcher->baked_groups = nullptr;
    batcher->baked_geom = nullptr;
    batcher->baked_tex = nullptr;
    batcher->baked_shade = nullptr;
    batcher->baked_indices = nullptr;
    ASSERT(batcher->bindings->count == 0);
    if (batcher->bindings != nullptr) {
        Vector_Free(batcher->bindings);
        batcher->bindings = nullptr;
    }
    if (batcher->transparent_sort != nullptr) {
        Vector_Free(batcher->transparent_sort);
        batcher->transparent_sort = nullptr;
    }
    for (int32_t pass = 0; pass < SCENE_PASS_COUNT; pass++) {
        if (batcher->staged[pass] != nullptr) {
            Vector_Free(batcher->staged[pass]);
            batcher->staged[pass] = nullptr;
        }
    }
    Memory_Free(batcher);
}

void MeshBatcher_RemoveMesh(
    MESH_BATCHER *const batcher, OUTPUT_MESH *const mesh)
{
    M_MESH_BUF_BINDING *const bind = M_GetBinding(batcher, mesh);
    if (bind == nullptr) {
        return;
    }
    Memory_Free(bind->geom_data);
    Memory_Free(bind->tex_data);
    Memory_Free(bind->shade_data);
    Memory_Free(bind->transparent_face_index_starts);
    Memory_Free(bind->transparent_face_index_counts);
    Vector_Remove(batcher->bindings, &bind);
    HASH_DEL(batcher->binding_map, bind);
    batcher->layout_dirty = true;
    Memory_Free(bind);
}

void MeshBatcher_AddMesh(MESH_BATCHER *const batcher, OUTPUT_MESH *const mesh)
{
    ASSERT(mesh->sealed == 1);

    M_MESH_BUF_BINDING *const bind = Memory_Alloc(sizeof(M_MESH_BUF_BINDING));
    bind->mesh = mesh;
    bind->vertex_count = mesh->vertices->count;

    // 1. Copy Vertex Data
    const OUTPUT_MESH_VERTEX *const vertices = Vector_GetData(mesh->vertices);
    bind->geom_data = Memory_Alloc(sizeof(M_MESH_GEOM) * bind->vertex_count);
    bind->tex_data = Memory_Alloc(sizeof(M_MESH_TEXTURE) * bind->vertex_count);
    bind->shade_data = Memory_Alloc(sizeof(M_MESH_SHADE) * bind->vertex_count);
    for (int32_t i = 0; i < bind->vertex_count; i++) {
        M_FillGeometry(&bind->geom_data[i], &vertices[i]);
        M_FillTexture(&bind->tex_data[i], &vertices[i]);
        M_FillShade(&bind->shade_data[i], &vertices[i]);
        if ((vertices[i].flags & VERT_USE_DYNAMIC_LIGHT) != 0) {
            bind->needs_room_lights = true;
        }
        if ((vertices[i].flags & VERT_USE_OBJECT_LIGHT) != 0) {
            bind->needs_object_light = true;
            bind->needs_cpu_light = true;
        }
        if ((vertices[i].flags & VERT_USE_OWN_LIGHT) != 0) {
            bind->needs_own_light = true;
            bind->needs_cpu_light = true;
        }
    }

    // 2. Prepare index counts
    // Opaque
    bind->opaque_index_count = mesh->opaque_vertex_indices->count;
    // Blend/Add
    bind->blend_add_index_count = mesh->blend_add_vertex_indices->count;

    // Transparent
    bind->transparent_face_count = mesh->transparent_faces->count;
    bind->transparent_face_index_starts = nullptr;
    bind->transparent_face_index_counts = nullptr;
    bind->transparent_index_count = 0;
    if (bind->transparent_face_count > 0) {
        bind->transparent_face_index_starts =
            Memory_Alloc(sizeof(int32_t) * bind->transparent_face_count);
        bind->transparent_face_index_counts =
            Memory_Alloc(sizeof(int32_t) * bind->transparent_face_count);
        for (int32_t i = 0; i < bind->transparent_face_count; i++) {
            const OUTPUT_MESH_FACE *const face =
                Vector_Get(mesh->transparent_faces, i);
            bind->transparent_face_index_starts[i] =
                bind->transparent_index_count;
            bind->transparent_face_index_counts[i] = face->vertex_count;
            bind->transparent_index_count += face->vertex_count;
        }
    }

    // Prevent double add
    mesh->sealed = 2;

    Vector_Add(batcher->bindings, &bind);
    HASH_ADD_PTR(batcher->binding_map, mesh, bind);
    batcher->layout_dirty = true;
}

void MeshBatcher_Seal(MESH_BATCHER *const batcher)
{
    if (batcher->layout_dirty) {
        M_RecalculateLayout(batcher);
    }

    glBindBuffer(GL_ARRAY_BUFFER, batcher->vbo.geom);
    TRX_GL_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER,
        batcher->vertex_count * sizeof(M_MESH_GEOM), nullptr,
        GL_DYNAMIC_DRAW); // allow updating mesh flags

    glBindBuffer(GL_ARRAY_BUFFER, batcher->vbo.tex);
    TRX_GL_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER,
        batcher->vertex_count * sizeof(M_MESH_TEXTURE), nullptr,
        GL_DYNAMIC_DRAW); // allow animating textures

    glBindBuffer(GL_ARRAY_BUFFER, batcher->vbo.shade);
    TRX_GL_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER,
        batcher->vertex_count * sizeof(M_MESH_SHADE), nullptr, GL_DYNAMIC_DRAW);

    // Upload vertex data
    for (int32_t i = 0; i < batcher->bindings->count; i++) {
        M_MESH_BUF_BINDING *const bind =
            *(M_MESH_BUF_BINDING **)Vector_Get(batcher->bindings, i);
        glBindBuffer(GL_ARRAY_BUFFER, batcher->vbo.geom);
        TRX_GL_TRACK_SUBDATA(
            glBufferSubData, GL_ARRAY_BUFFER,
            bind->vertex_start * sizeof(M_MESH_GEOM),
            bind->vertex_count * sizeof(M_MESH_GEOM), bind->geom_data);
        glBindBuffer(GL_ARRAY_BUFFER, batcher->vbo.tex);
        TRX_GL_TRACK_SUBDATA(
            glBufferSubData, GL_ARRAY_BUFFER,
            bind->vertex_start * sizeof(M_MESH_TEXTURE),
            bind->vertex_count * sizeof(M_MESH_TEXTURE), bind->tex_data);
        glBindBuffer(GL_ARRAY_BUFFER, batcher->vbo.shade);
        TRX_GL_TRACK_SUBDATA(
            glBufferSubData, GL_ARRAY_BUFFER,
            bind->vertex_start * sizeof(M_MESH_SHADE),
            bind->vertex_count * sizeof(M_MESH_SHADE), bind->shade_data);
    }

    // Allocate CPU scratch memory for the combined indices
    uint32_t *opaque_indices =
        Memory_Alloc(batcher->opaque_total_indices * sizeof(uint32_t));
    uint32_t *blend_indices =
        Memory_Alloc(batcher->blend_add_total_indices * sizeof(uint32_t));
    uint32_t *transparent_indices =
        Memory_Alloc(batcher->transparent_total_indices * sizeof(uint32_t));

    // Flatten the data
    for (int32_t i = 0; i < batcher->bindings->count; i++) {
        M_MESH_BUF_BINDING *const bind =
            *(M_MESH_BUF_BINDING **)Vector_Get(batcher->bindings, i);

        // Copy Opaque Indices
        if (bind->opaque_index_count > 0) {
            memcpy(
                &opaque_indices[bind->opaque_index_start],
                Vector_GetData(bind->mesh->opaque_vertex_indices),
                bind->opaque_index_count * sizeof(uint32_t));
        }

        // Copy Blend Indices
        if (bind->blend_add_index_count > 0) {
            memcpy(
                &blend_indices[bind->blend_add_index_start],
                Vector_GetData(bind->mesh->blend_add_vertex_indices),
                bind->blend_add_index_count * sizeof(uint32_t));
        }

        // Copy Transparent Indices
        if (bind->transparent_index_count > 0) {
            for (int32_t j = 0; j < bind->transparent_face_count; j++) {
                const OUTPUT_MESH_FACE *const face =
                    Vector_Get(bind->mesh->transparent_faces, j);
                const int32_t dst_start = bind->transparent_index_start
                    + bind->transparent_face_index_starts[j];
                for (int32_t k = 0; k < face->vertex_count; k++) {
                    transparent_indices[dst_start + k] =
                        bind->vertex_start + face->vertex_indices[k];
                }
            }
        }
    }

    // Upload to GPU
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batcher->ebo.opaque);
    TRX_GL_TRACK_DATA(
        glBufferData, GL_ELEMENT_ARRAY_BUFFER,
        batcher->opaque_total_indices * sizeof(uint32_t), opaque_indices,
        GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batcher->ebo.blend_add);
    TRX_GL_TRACK_DATA(
        glBufferData, GL_ELEMENT_ARRAY_BUFFER,
        batcher->blend_add_total_indices * sizeof(uint32_t), blend_indices,
        GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batcher->ebo.transparent);
    TRX_GL_TRACK_DATA(
        glBufferData, GL_ELEMENT_ARRAY_BUFFER,
        batcher->transparent_total_indices * sizeof(uint32_t),
        transparent_indices, GL_STATIC_DRAW);

    Memory_FreePointer(&opaque_indices);
    Memory_FreePointer(&blend_indices);
    Memory_FreePointer(&transparent_indices);
}

void MeshBatcher_UpdateMeshGeometry(
    const MESH_BATCHER *const batcher, const OUTPUT_MESH *const mesh)
{
    if (mesh == nullptr) {
        return;
    }
    glBindBuffer(GL_ARRAY_BUFFER, batcher->vbo.geom);
    M_UpdateMeshGeometry(batcher, mesh);
}

void MeshBatcher_Stage(
    MESH_BATCHER *const batcher, const MESH_INSTANCE *const inst,
    const SCENE_PASS pass)
{
    if (inst->mesh == nullptr) {
        return;
    }
    Vector_Add(batcher->staged[pass], inst);
}

const SCENE_SOURCE *MeshBatcher_AsSource(const MESH_BATCHER *const batcher)
{
    return &batcher->source;
}
