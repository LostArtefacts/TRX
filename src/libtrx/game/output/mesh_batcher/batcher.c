#include "game/output/mesh_batcher/batcher.h"

#include "debug.h"
#include "game/output.h"
#include "game/output/shader.h"
#include "game/output/utils.h"
#include "game/output/vertex_range.h"
#include "memory.h"

#include <uthash.h>

typedef OUTPUT_SHORT M_MESH_SHADE;

typedef struct {
    XYZW_F pos;
    XYZ_F normal;
    OUTPUT_USHORT flags;
    RGBA_8888 color;
} M_MESH_GEOM;

typedef struct {
    OUTPUT_UVW uvw;
    OUTPUT_TEXTURE_SIZE texture_size;
    float trapezoid_ratio[2];
} M_MESH_TEXTURE;

typedef struct {
    M_MESH_GEOM geom;
    M_MESH_TEXTURE tex;
    M_MESH_SHADE shade;
} M_MESH_FULL;

typedef struct M_MESH_BUF_BINDING {
    OUTPUT_MESH *mesh;
    M_MESH_GEOM *geom_data;
    M_MESH_TEXTURE *tex_data;
    M_MESH_SHADE *shade_data;
    int32_t vertex_start;
    int32_t vertex_count;
    UT_hash_handle hh;
    GLuint opaque_ebo;
} M_MESH_BUF_BINDING;

typedef struct {
    int32_t sort_key;
    const MESH_INSTANCE *inst;
    const OUTPUT_MESH_FACE *face;
    int32_t vertex_start;
    int32_t vertex_count;
} M_FACE_SORT;

typedef struct MESH_BATCHER {
    SCENE_SOURCE source;

    int32_t vertex_count;

    VECTOR *bindings;
    M_MESH_BUF_BINDING *binding_map;
    VECTOR *staged[SCENE_PASS_COUNT];

    OUTPUT_SHADER *shader;
    GLuint partial_vao;
    GLuint geom_vbo;
    GLuint tex_vbo;
    GLuint shade_vbo;

    VECTOR *transparent_sort; // M_FACE_SORT
    VECTOR *transparent_vertices; // M_MESH_FULL
    GLuint full_vao;
    GLuint full_vbo;
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
    geom->normal = vertex->normal;
    geom->color = vertex->color;
    geom->flags = vertex->flags;
}

static void M_FillTexture(
    M_MESH_TEXTURE *const tex, const OUTPUT_MESH_VERTEX *const vertex)
{
    if (vertex->uvw_idx < 0) {
        return;
    }
    tex->uvw = Output_Textures_GetUVW(vertex->uvw_idx);
    tex->texture_size = Output_Textures_GetAtlasSize(vertex->uvw_idx / 4);
    tex->trapezoid_ratio[0] = vertex->trapezoid_ratio[0];
    tex->trapezoid_ratio[1] = vertex->trapezoid_ratio[1];
}

static void M_FillShade(
    M_MESH_SHADE *const shade, const OUTPUT_MESH_VERTEX *const vertex)
{
    *shade = vertex->shade;
}

static void M_AnimateBinding(
    const MESH_BATCHER *const batcher, const M_MESH_BUF_BINDING *const bind)
{
    ASSERT(bind != nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, batcher->tex_vbo);
    const OUTPUT_MESH_VERTEX *const vertices =
        Vector_GetData(bind->mesh->vertices);
    for (int32_t i = 0; i < bind->mesh->animated_vertices->count; i++) {
        const OUTPUT_VERTEX_RANGE *const range =
            Vector_Get(bind->mesh->animated_vertices, i);
        for (int32_t j = range->vertex_start;
             j < range->vertex_start + range->vertex_count; j++) {
            M_FillTexture(&bind->tex_data[j], &vertices[j]);
        }
        GFX_TRACK_DATA(
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
    GFX_TRACK_SUBDATA(
        glBufferSubData, GL_ARRAY_BUFFER,
        bind->vertex_start * sizeof(M_MESH_GEOM),
        bind->vertex_count * sizeof(M_MESH_GEOM), bind->geom_data);
}

// Compare two faces by camera-space depth.
static int M_CompareFaceDepth(const void *const a, const void *const b)
{
    const M_FACE_SORT *const face_a = a;
    const M_FACE_SORT *const face_b = b;
    if (face_b->sort_key == face_a->sort_key) {
        return face_b->inst - face_a->inst;
    }
    return face_b->sort_key - face_a->sort_key;
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
        if (bptr->inst->transparent_sort_func != nullptr) {
            bptr->sort_key =
                bptr->inst->transparent_sort_func(bptr->inst, bptr->face);
        } else {
            // clang-format off
            bptr->sort_key = (
                bptr->inst->matrix._20 * (int32_t)bptr->face->mesh_centroid.x +
                bptr->inst->matrix._21 * (int32_t)bptr->face->mesh_centroid.y +
                bptr->inst->matrix._22 * (int32_t)bptr->face->mesh_centroid.z +
                bptr->inst->matrix._23);
            // clang-format on
        }
        bptr++;
    }
    qsort(buf, n, sizeof(*buf), M_CompareFaceDepth);
}

static void M_DrawOpaqueVertices(
    const MESH_BATCHER *const batcher, const MESH_INSTANCE *const inst)
{
    M_MESH_BUF_BINDING *const bind = M_GetBinding(batcher, inst->mesh);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bind->opaque_ebo);
    glDrawElementsBaseVertex(
        GL_TRIANGLES, inst->mesh->opaque_vertex_indices->count, GL_UNSIGNED_INT,
        nullptr, bind->vertex_start);
    GFX_GL_CheckError();
    g_GFX_Metrics.opaque_vert_count += inst->mesh->opaque_vertex_indices->count;
}

static void M_DrawOpaqueInstance(
    const MESH_BATCHER *const batcher, MESH_INSTANCE *const inst)
{
    M_MESH_BUF_BINDING *const bind = M_GetBinding(batcher, inst->mesh);
    ASSERT(bind != nullptr);

    Output_Shader_UploadViewModelMatrix(batcher->shader, &inst->matrix);
    Output_Shader_UploadTint(batcher->shader, inst->tint);

    if (inst->enable_scissor) {
        Output_EnableScissor(
            inst->scissor.x, inst->scissor.y, inst->scissor.width,
            inst->scissor.height);
    }

    if (inst->wibble) {
        Output_Shader_UploadWibbleEffect(batcher->shader, false);
        glDepthMask(GL_FALSE);
        M_DrawOpaqueVertices(batcher, inst);
        glDepthMask(GL_TRUE);
        Output_Shader_UploadWibbleEffect(batcher->shader, true);
        M_DrawOpaqueVertices(batcher, inst);
    } else {
        Output_Shader_UploadWibbleEffect(batcher->shader, false);
        M_DrawOpaqueVertices(batcher, inst);
    }

    if (inst->enable_scissor) {
        Output_DisableScissor();
    }
}

static void M_OpaquePass(
    const MESH_BATCHER *const batcher, const SCENE_PASS pass)
{
    float depth_adjust = 0.0f;
    VECTOR *const staged = batcher->staged[pass];

    glBindVertexArray(batcher->partial_vao);
    glBindBuffer(GL_ARRAY_BUFFER, batcher->shade_vbo);

    for (int32_t i = 0; i < staged->count; i++) {
        MESH_INSTANCE *const inst = Vector_Get(staged, i);
        const MATRIX *const m = &inst->matrix;
        const M_MESH_BUF_BINDING *const bind =
            M_GetBinding(batcher, inst->mesh);

        // Update lighting data. The updates are done on the models rather than
        // instances, so that we do not have to allocate vertex data memory for
        // instances on every stage.
        if (inst->update_light_func != nullptr) {
            inst->update_light_func(inst, inst->update_light_func_data);
            const OUTPUT_MESH_VERTEX *const vertices =
                Vector_GetData(inst->mesh->vertices);
            for (int32_t j = 0; j < bind->vertex_count; j++) {
                M_FillShade(&bind->shade_data[j], &vertices[j]);
            }
            GFX_TRACK_SUBDATA(
                glBufferSubData, GL_ARRAY_BUFFER,
                bind->vertex_start * sizeof(M_MESH_SHADE),
                bind->vertex_count * sizeof(M_MESH_SHADE), bind->shade_data);
        }

        if (inst->mesh->opaque_vertex_indices->count != 0) {
            Output_AdjustDepth(0.0f, inst->depth_adjust * 2.0f / 0.005f);
            M_DrawOpaqueInstance(batcher, inst);
        }

        // Accumulate transparent polygons and faces.
        for (int32_t j = 0; j < inst->mesh->transparent_faces->count; j++) {
            const OUTPUT_MESH_FACE *const face =
                Vector_Get(inst->mesh->transparent_faces, j);

            const int32_t vertex_start = batcher->transparent_vertices->count;
            for (int32_t k = 0; k < face->vertex_count; k++) {
                const int32_t l = face->vertex_indices[k];
                const M_MESH_FULL v = {
                    .geom = bind->geom_data[l],
                    .tex = bind->tex_data[l],
                    .shade = bind->shade_data[l],
                };
                Vector_Add(batcher->transparent_vertices, &v);
            }
            const int32_t vertex_count =
                batcher->transparent_vertices->count - vertex_start;

            Vector_Add(
                batcher->transparent_sort,
                &(M_FACE_SORT) {
                    .inst = inst,
                    .face = face,
                    .vertex_start = vertex_start,
                    .vertex_count = vertex_count,
                });
        }
    }
    Output_AdjustDepth(0.0f, 0.0f);
}

static void M_TransparentPass(const MESH_BATCHER *const batcher)
{
    glBindVertexArray(batcher->full_vao);
    glBindBuffer(GL_ARRAY_BUFFER, batcher->full_vbo);
    GFX_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER,
        batcher->transparent_vertices->count * sizeof(M_MESH_FULL),
        Vector_GetData(batcher->transparent_vertices), GL_STATIC_DRAW);

    const MESH_INSTANCE *inst = nullptr;
    for (int32_t i = 0; i < batcher->transparent_sort->count; i++) {
        const M_FACE_SORT *const sort_ptr =
            Vector_Get(batcher->transparent_sort, i);
        if (sort_ptr->face->vertex_count == 0) {
            continue;
        }
        if (sort_ptr->inst != inst) {
            inst = sort_ptr->inst;
            Output_Shader_UploadViewModelMatrix(batcher->shader, &inst->matrix);
            Output_Shader_UploadTint(batcher->shader, inst->tint);
            Output_Shader_UploadWibbleEffect(batcher->shader, inst->wibble);
            Output_AdjustDepth(0.0f, inst->depth_adjust * 2.0f / 0.005f);
        }
        glDrawArrays(
            GL_TRIANGLES, sort_ptr->vertex_start, sort_ptr->vertex_count);
        g_GFX_Metrics.trans_vert_count += sort_ptr->face->vertex_count;
    }
    Output_AdjustDepth(0.0f, 0.0f);
}

static void M_RenderBegin(const SCENE_SOURCE *const source)
{
    MESH_BATCHER *const batcher = source->priv;
    for (int32_t pass = 0; pass < SCENE_PASS_COUNT; pass++) {
        Vector_Clear(batcher->staged[pass]);
    }
    Vector_Clear(batcher->transparent_vertices);
    Vector_Clear(batcher->transparent_sort);
}

static void M_RenderPass(
    const SCENE_SOURCE *const source, const SCENE_PASS pass)
{
    MESH_BATCHER *const batcher = source->priv;

    if (pass == SCENE_PASS_MESHES || pass == SCENE_PASS_SKYBOX) {
        M_OpaquePass(batcher, pass);
    } else if (pass == SCENE_PASS_TRANSPARENT) {
        M_SortTransparentFaces(batcher);
        M_TransparentPass(batcher);
    }
}

static bool M_IsDirty(const SCENE_SOURCE *const source, const SCENE_PASS pass)
{
    const MESH_BATCHER *const batcher = source->priv;
    return batcher->staged[pass]->count > 0
        || (batcher->transparent_vertices->count > 0
            && pass == SCENE_PASS_TRANSPARENT);
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
    batcher->transparent_vertices = Vector_Create(sizeof(M_MESH_FULL));

    glGenVertexArrays(1, &batcher->partial_vao);
    glGenBuffers(1, &batcher->geom_vbo);
    glGenBuffers(1, &batcher->tex_vbo);
    glGenBuffers(1, &batcher->shade_vbo);
    glGenBuffers(1, &batcher->full_vbo);

    glBindVertexArray(batcher->partial_vao);
    glBindBuffer(GL_ARRAY_BUFFER, batcher->geom_vbo);

    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_POS);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_NORMAL);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_FLAGS);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_COLOR);
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_POS, 4, GL_FLOAT, GL_FALSE, sizeof(M_MESH_GEOM),
        (void *)(intptr_t)offsetof(M_MESH_GEOM, pos));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(M_MESH_GEOM),
        (void *)(intptr_t)offsetof(M_MESH_GEOM, normal));
    glVertexAttribIPointer(
        OUTPUT_MESH_ATTR_FLAGS, 1, OUTPUT_USHORT_GL, sizeof(M_MESH_GEOM),
        (void *)(intptr_t)offsetof(M_MESH_GEOM, flags));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE,
        sizeof(M_MESH_GEOM), (void *)(intptr_t)offsetof(M_MESH_GEOM, color));

    glBindBuffer(GL_ARRAY_BUFFER, batcher->tex_vbo);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_UVW);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_TEXTURE_SIZE);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_TRAPEZOID_RATIO);
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

    glBindBuffer(GL_ARRAY_BUFFER, batcher->shade_vbo);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_SHADE);
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_SHADE, 1, OUTPUT_SHORT_GL, GL_FALSE,
        sizeof(M_MESH_SHADE), 0);

    glGenVertexArrays(1, &batcher->full_vao);
    glBindVertexArray(batcher->full_vao);
    glBindBuffer(GL_ARRAY_BUFFER, batcher->full_vbo);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_POS);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_NORMAL);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_FLAGS);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_COLOR);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_UVW);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_TEXTURE_SIZE);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_TRAPEZOID_RATIO);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_SHADE);
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_POS, 4, GL_FLOAT, GL_FALSE, sizeof(M_MESH_FULL),
        (void *)(intptr_t)offsetof(M_MESH_FULL, geom.pos));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(M_MESH_FULL),
        (void *)(intptr_t)offsetof(M_MESH_FULL, geom.normal));
    glVertexAttribIPointer(
        OUTPUT_MESH_ATTR_FLAGS, 1, OUTPUT_USHORT_GL, sizeof(M_MESH_FULL),
        (void *)(intptr_t)offsetof(M_MESH_FULL, geom.flags));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE,
        sizeof(M_MESH_FULL),
        (void *)(intptr_t)offsetof(M_MESH_FULL, geom.color));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_UVW, 3, GL_FLOAT, GL_FALSE, sizeof(M_MESH_FULL),
        (void *)(intptr_t)offsetof(M_MESH_FULL, tex.uvw));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_TEXTURE_SIZE, 4, GL_FLOAT, GL_FALSE,
        sizeof(M_MESH_FULL),
        (void *)(intptr_t)offsetof(M_MESH_FULL, tex.texture_size));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_TRAPEZOID_RATIO, 2, GL_FLOAT, GL_FALSE,
        sizeof(M_MESH_FULL),
        (void *)(intptr_t)offsetof(M_MESH_FULL, tex.trapezoid_ratio));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_SHADE, 1, OUTPUT_SHORT_GL, GL_FALSE,
        sizeof(M_MESH_FULL), (void *)(intptr_t)offsetof(M_MESH_FULL, shade));

    return batcher;
}

void MeshBatcher_Destroy(MESH_BATCHER *const batcher)
{
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (batcher->partial_vao != 0) {
        glDeleteVertexArrays(1, &batcher->partial_vao);
        batcher->partial_vao = 0;
    }
    if (batcher->full_vao != 0) {
        glDeleteVertexArrays(1, &batcher->full_vao);
        batcher->full_vao = 0;
    }
    if (batcher->geom_vbo != 0) {
        glDeleteBuffers(1, &batcher->geom_vbo);
        batcher->geom_vbo = 0;
    }
    if (batcher->tex_vbo != 0) {
        glDeleteBuffers(1, &batcher->tex_vbo);
        batcher->tex_vbo = 0;
    }
    if (batcher->shade_vbo != 0) {
        glDeleteBuffers(1, &batcher->shade_vbo);
        batcher->shade_vbo = 0;
    }
    if (batcher->full_vbo != 0) {
        glDeleteBuffers(1, &batcher->full_vbo);
        batcher->full_vbo = 0;
    }
    ASSERT(batcher->bindings->count == 0);
    if (batcher->bindings != nullptr) {
        Vector_Free(batcher->bindings);
        batcher->bindings = nullptr;
    }
    if (batcher->transparent_sort != nullptr) {
        Vector_Free(batcher->transparent_sort);
        batcher->transparent_sort = nullptr;
    }
    if (batcher->transparent_vertices != nullptr) {
        Vector_Free(batcher->transparent_vertices);
        batcher->transparent_vertices = nullptr;
    }
    for (int32_t pass = 0; pass < SCENE_PASS_COUNT; pass++) {
        if (batcher->staged[pass] != nullptr) {
            Vector_Free(batcher->staged[pass]);
            batcher->staged[pass] = nullptr;
        }
    }
    ASSERT(batcher->vertex_count == 0);
    Memory_Free(batcher);
}

void MeshBatcher_RemoveMesh(
    MESH_BATCHER *const batcher, OUTPUT_MESH *const mesh)
{
    M_MESH_BUF_BINDING *const bind = M_GetBinding(batcher, mesh);
    if (bind == nullptr) {
        return;
    }
    if (bind->opaque_ebo != 0) {
        glDeleteBuffers(1, &bind->opaque_ebo);
        bind->opaque_ebo = 0;
    }
    Memory_Free(bind->geom_data);
    Memory_Free(bind->tex_data);
    Memory_Free(bind->shade_data);
    Vector_Remove(batcher->bindings, &bind);
    HASH_DEL(batcher->binding_map, bind);
    if (batcher->bindings->count == 0) {
        batcher->vertex_count = 0;
    }
    Memory_Free(bind);
}

void MeshBatcher_AddMesh(MESH_BATCHER *const batcher, OUTPUT_MESH *const mesh)
{
    ASSERT(mesh->sealed == 1);

    M_MESH_BUF_BINDING *const bind = Memory_Alloc(sizeof(M_MESH_BUF_BINDING));
    bind->mesh = mesh;
    bind->vertex_count = mesh->vertices->count;
    const OUTPUT_MESH_VERTEX *const vertices = Vector_GetData(mesh->vertices);

    bind->geom_data = Memory_Alloc(sizeof(M_MESH_GEOM) * bind->vertex_count);
    bind->tex_data = Memory_Alloc(sizeof(M_MESH_TEXTURE) * bind->vertex_count);
    bind->shade_data = Memory_Alloc(sizeof(M_MESH_SHADE) * bind->vertex_count);
    for (int32_t i = 0; i < bind->vertex_count; i++) {
        M_FillGeometry(&bind->geom_data[i], &vertices[i]);
        M_FillTexture(&bind->tex_data[i], &vertices[i]);
        M_FillShade(&bind->shade_data[i], &vertices[i]);
    }

    bind->vertex_start = batcher->vertex_count;
    batcher->vertex_count += bind->vertex_count;

    // Prevent the same mesh from being added twice to a mesh batcher
    mesh->sealed = 2;

    Vector_Add(batcher->bindings, &bind);
    HASH_ADD_PTR(batcher->binding_map, mesh, bind);
}

void MeshBatcher_Seal(MESH_BATCHER *const batcher)
{
    glBindBuffer(GL_ARRAY_BUFFER, batcher->geom_vbo);
    GFX_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER,
        batcher->vertex_count * sizeof(M_MESH_GEOM), nullptr,
        GL_DYNAMIC_DRAW); // allow updating mesh flags

    glBindBuffer(GL_ARRAY_BUFFER, batcher->tex_vbo);
    GFX_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER,
        batcher->vertex_count * sizeof(M_MESH_TEXTURE), nullptr,
        GL_DYNAMIC_DRAW); // allow animating textures

    glBindBuffer(GL_ARRAY_BUFFER, batcher->shade_vbo);
    GFX_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER,
        batcher->vertex_count * sizeof(M_MESH_SHADE), nullptr,
        GL_DYNAMIC_DRAW); // shades are always dynamic for now

    for (int32_t i = 0; i < batcher->bindings->count; i++) {
        M_MESH_BUF_BINDING *const bind =
            *(M_MESH_BUF_BINDING **)Vector_Get(batcher->bindings, i);
        glBindBuffer(GL_ARRAY_BUFFER, batcher->geom_vbo);
        GFX_TRACK_SUBDATA(
            glBufferSubData, GL_ARRAY_BUFFER,
            bind->vertex_start * sizeof(M_MESH_GEOM),
            bind->vertex_count * sizeof(M_MESH_GEOM), bind->geom_data);
        glBindBuffer(GL_ARRAY_BUFFER, batcher->tex_vbo);
        GFX_TRACK_SUBDATA(
            glBufferSubData, GL_ARRAY_BUFFER,
            bind->vertex_start * sizeof(M_MESH_TEXTURE),
            bind->vertex_count * sizeof(M_MESH_TEXTURE), bind->tex_data);
        glBindBuffer(GL_ARRAY_BUFFER, batcher->shade_vbo);
        GFX_TRACK_SUBDATA(
            glBufferSubData, GL_ARRAY_BUFFER,
            bind->vertex_start * sizeof(M_MESH_SHADE),
            bind->vertex_count * sizeof(M_MESH_SHADE), bind->shade_data);

        glGenBuffers(1, &bind->opaque_ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bind->opaque_ebo);
        GFX_TRACK_DATA(
            glBufferData, GL_ELEMENT_ARRAY_BUFFER,
            bind->mesh->opaque_vertex_indices->count * sizeof(uint32_t),
            Vector_GetData(bind->mesh->opaque_vertex_indices), GL_STATIC_DRAW);
    }
}

void MeshBatcher_UpdateMeshGeometry(
    const MESH_BATCHER *const batcher, const OUTPUT_MESH *const mesh)
{
    if (mesh == nullptr) {
        return;
    }
    glBindBuffer(GL_ARRAY_BUFFER, batcher->geom_vbo);
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
