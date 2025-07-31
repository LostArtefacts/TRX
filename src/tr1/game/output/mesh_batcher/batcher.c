#include "game/output/mesh_batcher/batcher.h"

#include "game/output.h"
#include "game/output/textures.h"
#include "game/output/utils.h"
#include "game/output/vertex_range.h"

#include <libtrx/debug.h>
#include <libtrx/game/level/const.h>
#include <libtrx/memory.h>

#include <uthash.h>

typedef OUTPUT_SHORT M_MESH_SHADE;

typedef struct {
    XYZ_F pos;
    XYZ_F normal;
    OUTPUT_USHORT flags;
    RGBA_8888 color;
} M_MESH_GEOM;

typedef struct {
    OUTPUT_UVW uvw;
    OUTPUT_TEXTURE_SIZE texture_size;
    float trapezoid_ratio[2];
} M_MESH_TEXTURE;

typedef struct M_MESH_BUF_BINDING {
    OUTPUT_MESH *mesh;
    M_MESH_GEOM *geom_data;
    M_MESH_TEXTURE *tex_data;
    M_MESH_SHADE *shade_data;
    int32_t vertex_start;
    int32_t vertex_count;
    UT_hash_handle hh;
} M_MESH_BUF_BINDING;

typedef struct MESH_BATCHER {
    SCENE_SOURCE source;

    int32_t vertex_count;

    VECTOR *bindings;
    M_MESH_BUF_BINDING *binding_map;
    VECTOR *staged[SCENE_PASS_COUNT];

    OUTPUT_SHADER *shader;
    GLuint vao;
    GLuint geom_vbo;
    GLuint tex_vbo;
    GLuint shade_vbo;
} MESH_BATCHER;

static M_MESH_BUF_BINDING *M_GetBinding(
    const MESH_BATCHER *batcher, const OUTPUT_MESH *mesh);

static void M_FillGeometry(M_MESH_GEOM *tex, const OUTPUT_MESH_VERTEX *vertex);
static void M_FillTexture(
    M_MESH_TEXTURE *tex, const OUTPUT_MESH_VERTEX *vertex);
static void M_FillShade(M_MESH_SHADE *shade, const OUTPUT_MESH_VERTEX *vertex);

static void M_DrawInstance(const MESH_BATCHER *batcher, MESH_INSTANCE *inst);
static void M_AnimateBinding(
    const MESH_BATCHER *batcher, const M_MESH_BUF_BINDING *bind);

static void M_RenderBegin(const SCENE_SOURCE *source);
static void M_RenderPass(const SCENE_SOURCE *source, SCENE_PASS pass);
static bool M_IsDirty(const SCENE_SOURCE *source, SCENE_PASS pass);
static void M_AnimateTextures(const SCENE_SOURCE *source);

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
    geom->pos = vertex->pos;
    geom->normal = vertex->normal;
    geom->color = vertex->color;
    geom->flags = vertex->flags;
}

static void M_FillTexture(
    M_MESH_TEXTURE *const tex, const OUTPUT_MESH_VERTEX *const vertex)
{
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

static void M_DrawInstance(
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

    if (inst->update_light_func != nullptr) {
        inst->update_light_func(inst, inst->update_light_func_data);
        MeshBatcher_UpdateMeshShades(batcher, inst->mesh);
    }

    if (inst->wibble) {
        Output_Shader_UploadWibbleEffect(batcher->shader, false);
        glDepthMask(GL_FALSE);
        glDrawArrays(GL_TRIANGLES, bind->vertex_start, bind->vertex_count);
        g_GFX_Metrics.opaque_vert_count += bind->vertex_count;
        glDepthMask(GL_TRUE);
        Output_Shader_UploadWibbleEffect(batcher->shader, true);
        glDrawArrays(GL_TRIANGLES, bind->vertex_start, bind->vertex_count);
        g_GFX_Metrics.opaque_vert_count += bind->vertex_count;
    } else {
        Output_Shader_UploadWibbleEffect(batcher->shader, false);
        glDrawArrays(GL_TRIANGLES, bind->vertex_start, bind->vertex_count);
        g_GFX_Metrics.opaque_vert_count += bind->vertex_count;
    }

    if (inst->enable_scissor) {
        Output_DisableScissor();
    }
}

static void M_RenderBegin(const SCENE_SOURCE *const source)
{
    MESH_BATCHER *const batcher = source->priv;
    for (int32_t pass = 0; pass < SCENE_PASS_COUNT; pass++) {
        Vector_Clear(batcher->staged[pass]);
    }
}

static void M_RenderPass(
    const SCENE_SOURCE *const source, const SCENE_PASS pass)
{
    MESH_BATCHER *const batcher = source->priv;
    glBindVertexArray(batcher->vao);

    for (int32_t i = 0; i < batcher->staged[pass]->count; i++) {
        MESH_INSTANCE *const inst = Vector_Get(batcher->staged[pass], i);
        M_DrawInstance(batcher, inst);
    }
}

static bool M_IsDirty(const SCENE_SOURCE *const source, const SCENE_PASS pass)
{
    const MESH_BATCHER *const batcher = source->priv;
    return batcher->staged[pass]->count > 0;
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

    glGenVertexArrays(1, &batcher->vao);
    glGenBuffers(1, &batcher->geom_vbo);
    glGenBuffers(1, &batcher->tex_vbo);
    glGenBuffers(1, &batcher->shade_vbo);

    glBindVertexArray(batcher->vao);

    glBindBuffer(GL_ARRAY_BUFFER, batcher->geom_vbo);

    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_POS);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_NORMAL);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_FLAGS);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_COLOR);
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_POS, 3, GL_FLOAT, GL_FALSE, sizeof(M_MESH_GEOM),
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
    ASSERT(batcher->bindings->count == 0);
    if (batcher->bindings != nullptr) {
        Vector_Free(batcher->bindings);
        batcher->bindings = nullptr;
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
    ASSERT(mesh->sealed);

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
    }
}

void MeshBatcher_UpdateMeshShades(
    const MESH_BATCHER *const batcher, const OUTPUT_MESH *const mesh)
{
    M_MESH_BUF_BINDING *const bind = M_GetBinding(batcher, mesh);
    const OUTPUT_MESH_VERTEX *const vertices = Vector_GetData(mesh->vertices);
    for (int32_t i = 0; i < bind->vertex_count; i++) {
        M_FillShade(&bind->shade_data[i], &vertices[i]);
    }
    glBindBuffer(GL_ARRAY_BUFFER, batcher->shade_vbo);
    GFX_TRACK_SUBDATA(
        glBufferSubData, GL_ARRAY_BUFFER,
        bind->vertex_start * sizeof(M_MESH_SHADE),
        bind->vertex_count * sizeof(M_MESH_SHADE), bind->shade_data);
}

void MeshBatcher_UpdateMeshGeometry(
    const MESH_BATCHER *const batcher, const OUTPUT_MESH *const mesh)
{
    M_MESH_BUF_BINDING *const bind = M_GetBinding(batcher, mesh);
    const OUTPUT_MESH_VERTEX *const vertices = Vector_GetData(mesh->vertices);
    for (int32_t i = 0; i < bind->vertex_count; i++) {
        M_FillGeometry(&bind->geom_data[i], &vertices[i]);
    }
    glBindBuffer(GL_ARRAY_BUFFER, batcher->geom_vbo);
    GFX_TRACK_SUBDATA(
        glBufferSubData, GL_ARRAY_BUFFER,
        bind->vertex_start * sizeof(M_MESH_GEOM),
        bind->vertex_count * sizeof(M_MESH_GEOM), bind->geom_data);
}

void MeshBatcher_Stage(
    MESH_BATCHER *const batcher, const MESH_INSTANCE *const inst,
    const SCENE_PASS pass)
{
    Vector_Add(batcher->staged[pass], inst);
}

const SCENE_SOURCE *MeshBatcher_AsSource(const MESH_BATCHER *const batcher)
{
    return &batcher->source;
}
