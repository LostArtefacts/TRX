#include <trx/game/output/shaders/mesh.h>

#include <trx/config.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/output/lights/priv.h>
#include <trx/game/output/state.h>
#include <trx/game/output/textures.h>
#include <trx/game/output/utils.h>
#include <trx/gl/utils.h>
#include <trx/version.h>

#include <string.h>

#define M_VARIANT_COUNT 4
#define M_VARIANT_SUBDIVIDE 2
#define M_VARIANT_NO_DISCARD 1
// The most slots and lights a draw holds, however large a block the driver
// allows. A batched vertex holds its slot in the bits of gl_VertexID above
// the low 24, and gl_VertexID is a signed int, so 128 slots fit.
#define M_MAX_SLOTS 128
#define M_MAX_LIGHTS 256
// The binding points of the Instances and InstanceLights blocks, which follow
// the blocks that every shader shares.
#define M_INSTANCES_BINDING 4
#define M_INSTANCE_LIGHTS_BINDING 5
// The texture units that a batch reads its vertices from.
#define M_GEOM_DATA_UNIT 4
#define M_TEX_DATA_UNIT 5
#define M_SHADE_DATA_UNIT 6
#define M_GEOM_COLORS_UNIT 7
// How much a ring holds past one block before it starts over.
#define M_RING_SPAN (256 * 1024)

// A buffer that each upload takes a fresh stretch of, so that no upload
// writes where an earlier draw may still read: a driver waits for such a draw
// to finish before it writes. The ring starts over in new storage once it
// runs out.
typedef struct {
    GLuint buffer;
    size_t size;
    // How much the binding covers, which the block's size sets.
    size_t block_size;
    size_t align;
    size_t pos;
    // Where the binding starts.
    size_t bound;
} M_RING;

struct OUTPUT_MESH_SHADER {
    OUTPUT_SHADER *base[M_VARIANT_COUNT];

    bool is_alpha_discard_enabled[M_VARIANT_COUNT];
    bool is_pulling_vertices[M_VARIANT_COUNT];
    int32_t vertex_base[M_VARIANT_COUNT];
    OUTPUT_ATLAS_RECT env_map_rect[M_VARIANT_COUNT];

    int32_t max_slots;
    int32_t max_lights;

    // The instance of the draws that hold one, as slot 0 of the slot ring and
    // the lights at the start of the light ring hold it.
    M_RING slot_ring;
    M_RING light_ring;
    OUTPUT_MESH_SLOT slot;
    bool has_slot;
    OUTPUT_MESH_LIGHT *lights;
    int32_t light_count;

    bool is_pulling_vertices_wanted;
    int32_t vertex_base_wanted;
};

static bool m_SubdivisionSuspended;
static bool m_NoAlphaDiscard;

static const char *M_GetVariantPath(void)
{
    switch (Output_Lights_GetModel()->shader_variant) {
    case 1:
        return "meshes_tr3.glsl";
    case 2:
        return "meshes_tr4.glsl";
    default:
        return "meshes_tr12.glsl";
    }
}

static bool M_VariantSubdivides(const int32_t variant_idx)
{
    return (variant_idx & M_VARIANT_SUBDIVIDE) != 0;
}

static bool M_VariantOmitsDiscard(const int32_t variant_idx)
{
    return (variant_idx & M_VARIANT_NO_DISCARD) != 0;
}

static char *M_GetVariantDefines(
    const int32_t variant_idx, const int32_t max_slots,
    const int32_t max_lights)
{
    return String_Format(
        "%s%s#define MAX_INSTANCES %d\n#define MAX_INSTANCE_LIGHTS %d\n",
        M_VariantSubdivides(variant_idx) ? "#define SUBDIVIDE\n" : "",
        M_VariantOmitsDiscard(variant_idx) ? "#define NO_ALPHA_DISCARD\n" : "",
        max_slots, MAX(max_lights, 1));
}

static int32_t M_GetBlockCapacity(const size_t item_size, const int32_t cap)
{
    GLint block_size = 0;
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &block_size);
    return MIN(cap, block_size / (int32_t)item_size);
}

static int32_t M_GetMaxLights(void)
{
    if (Output_Lights_GetModel()->get_slot_light_count == nullptr) {
        return 0;
    }
    return M_GetBlockCapacity(sizeof(OUTPUT_MESH_LIGHT), M_MAX_LIGHTS);
}

static int32_t M_GetVariantIndex(void)
{
    int32_t idx = 0;
    if (g_Config.rendering.enable_affine_mapping && !m_SubdivisionSuspended) {
        idx |= M_VARIANT_SUBDIVIDE;
    }
    if (m_NoAlphaDiscard) {
        idx |= M_VARIANT_NO_DISCARD;
    }
    return idx;
}

static OUTPUT_SHADER *M_GetVariantBase(
    const OUTPUT_MESH_SHADER *const shader, const int32_t variant_idx)
{
    return shader->base[variant_idx];
}

// TR4 samples the env map out of the atlas, so the variant needs to know which
// patch of it to use. The rect only changes on level load; the TR1-3 variants
// don't declare the uniforms at all.
static void M_UploadEnvMapRect(
    OUTPUT_MESH_SHADER *const shader, const int32_t variant_idx)
{
    const OUTPUT_ATLAS_RECT rect = Output_Textures_GetEnvMapRect();
    if (memcmp(&shader->env_map_rect[variant_idx], &rect, sizeof(rect)) == 0) {
        return;
    }
    shader->env_map_rect[variant_idx] = rect;

    OUTPUT_SHADER *const base = M_GetVariantBase(shader, variant_idx);
    GLint loc = -1;
    if (Output_Shader_TryLookupUniform(base, "uEnvMapUV0", &loc)) {
        TRX_GL_TRACK_UNIFORM(glUniform2f, loc, rect.uv0[0], rect.uv0[1]);
    }
    if (Output_Shader_TryLookupUniform(base, "uEnvMapUV1", &loc)) {
        TRX_GL_TRACK_UNIFORM(glUniform2f, loc, rect.uv1[0], rect.uv1[1]);
    }
    if (Output_Shader_TryLookupUniform(base, "uEnvMapLayer", &loc)) {
        TRX_GL_TRACK_UNIFORM(glUniform1i, loc, rect.layer);
    }
}

static void M_UploadPulling(
    OUTPUT_MESH_SHADER *const shader, const int32_t variant_idx)
{
    OUTPUT_SHADER *const base = M_GetVariantBase(shader, variant_idx);
    if (shader->is_pulling_vertices[variant_idx]
        != shader->is_pulling_vertices_wanted) {
        TRX_GL_TRACK_UNIFORM(
            glUniform1i, Output_Shader_LookupUniform(base, "uPullVertices"),
            shader->is_pulling_vertices_wanted);
        shader->is_pulling_vertices[variant_idx] =
            shader->is_pulling_vertices_wanted;
    }
    if (shader->vertex_base[variant_idx] != shader->vertex_base_wanted) {
        TRX_GL_TRACK_UNIFORM(
            glUniform1i, Output_Shader_LookupUniform(base, "uVertexBase"),
            shader->vertex_base_wanted);
        shader->vertex_base[variant_idx] = shader->vertex_base_wanted;
    }
}

static void M_InitRing(
    M_RING *const ring, const size_t block_size, const size_t align)
{
    *ring = (M_RING) {
        .size = block_size + M_RING_SPAN,
        .block_size = block_size,
        .align = align,
    };
    glGenBuffers(1, &ring->buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, ring->buffer);
    glBufferData(GL_UNIFORM_BUFFER, ring->size, nullptr, GL_STREAM_DRAW);
}

static void M_FreeRing(M_RING *const ring)
{
    if (ring->buffer != 0) {
        glDeleteBuffers(1, &ring->buffer);
        ring->buffer = 0;
    }
}

static void M_BindRing(const M_RING *const ring, const GLuint binding)
{
    glBindBufferRange(
        GL_UNIFORM_BUFFER, binding, ring->buffer, ring->bound,
        ring->block_size);
}

static void M_PutRing(
    M_RING *const ring, const void *const data, const size_t size)
{
    size_t offset = (ring->pos + ring->align - 1) / ring->align * ring->align;
    glBindBuffer(GL_UNIFORM_BUFFER, ring->buffer);
    if (offset + ring->block_size > ring->size) {
        glBufferData(GL_UNIFORM_BUFFER, ring->size, nullptr, GL_STREAM_DRAW);
        offset = 0;
    }
    void *const target = glMapBufferRange(
        GL_UNIFORM_BUFFER, offset, size,
        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT
            | GL_MAP_UNSYNCHRONIZED_BIT);
    memcpy(target, data, size);
    glUnmapBuffer(GL_UNIFORM_BUFFER);
    g_TRX_GL_Metrics.buffer_total_bytes += size;
    g_TRX_GL_Metrics.buffer_transfer_count++;
    ring->pos = offset + size;
    ring->bound = offset;
}

static void M_BindSingle(const OUTPUT_MESH_SHADER *const shader)
{
    M_BindRing(&shader->slot_ring, M_INSTANCES_BINDING);
    if (shader->max_lights > 0) {
        M_BindRing(&shader->light_ring, M_INSTANCE_LIGHTS_BINDING);
    }
}

static void M_PutSlot(
    OUTPUT_MESH_SHADER *const shader, const OUTPUT_MESH_SLOT *const slot)
{
    if (shader->has_slot && memcmp(&shader->slot, slot, sizeof(*slot)) == 0) {
        return;
    }
    shader->slot = *slot;
    shader->has_slot = true;
    M_PutRing(&shader->slot_ring, slot, sizeof(*slot));
    if (!shader->is_pulling_vertices_wanted) {
        M_BindRing(&shader->slot_ring, M_INSTANCES_BINDING);
    }
}

static void M_PutLights(
    OUTPUT_MESH_SHADER *const shader, const OUTPUT_MESH_LIGHT *const lights,
    const int32_t count)
{
    if (count == 0 || shader->max_lights == 0) {
        return;
    }
    ASSERT(count <= shader->max_lights);
    const size_t size = count * sizeof(OUTPUT_MESH_LIGHT);
    if (count <= shader->light_count
        && memcmp(shader->lights, lights, size) == 0) {
        return;
    }
    memcpy(shader->lights, lights, size);
    shader->light_count = count;
    M_PutRing(&shader->light_ring, lights, size);
    if (!shader->is_pulling_vertices_wanted) {
        M_BindRing(&shader->light_ring, M_INSTANCE_LIGHTS_BINDING);
    }
}

RESULT Output_MeshShader_Create(OUTPUT_MESH_SHADER **const out_shader)
{
    *out_shader = nullptr;
    OUTPUT_MESH_SHADER *const shader = Memory_Alloc(sizeof(*shader));
    shader->max_slots =
        M_GetBlockCapacity(sizeof(OUTPUT_MESH_SLOT), M_MAX_SLOTS);
    shader->max_lights = M_GetMaxLights();
    for (int32_t i = 0; i < M_VARIANT_COUNT; i++) {
        shader->env_map_rect[i] = (OUTPUT_ATLAS_RECT) { .layer = -1 };

        const bool subdivides = M_VariantSubdivides(i);
        char *const defines =
            M_GetVariantDefines(i, shader->max_slots, shader->max_lights);
        const RESULT result = Output_Shader_CreateEx(
            M_GetVariantPath(), defines, subdivides, &shader->base[i]);
        Memory_Free(defines);
        MUST(result);
        Output_Shader_Bind(shader->base[i]);
        TRX_GL_TRACK_UNIFORM(
            glUniform1i,
            Output_Shader_LookupUniform(shader->base[i], "uTexAtlas"), 0);
        // TR4 reflections sample the atlas, so only the TR1-3 variants declare
        // the framebuffer-capture env map.
        GLint loc = -1;
        if (Output_Shader_TryLookupUniform(
                shader->base[i], "uTexEnvMap", &loc)) {
            TRX_GL_TRACK_UNIFORM(glUniform1i, loc, 1);
        }
        TRX_GL_TRACK_UNIFORM(
            glUniform1i,
            Output_Shader_LookupUniform(shader->base[i], "uGeomData"),
            M_GEOM_DATA_UNIT);
        TRX_GL_TRACK_UNIFORM(
            glUniform1i,
            Output_Shader_LookupUniform(shader->base[i], "uTexData"),
            M_TEX_DATA_UNIT);
        TRX_GL_TRACK_UNIFORM(
            glUniform1i,
            Output_Shader_LookupUniform(shader->base[i], "uShadeData"),
            M_SHADE_DATA_UNIT);
        TRX_GL_TRACK_UNIFORM(
            glUniform1i,
            Output_Shader_LookupUniform(shader->base[i], "uGeomColors"),
            M_GEOM_COLORS_UNIT);
    }

    GLint align = 1;
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &align);
    M_InitRing(
        &shader->slot_ring, shader->max_slots * sizeof(OUTPUT_MESH_SLOT),
        MAX(align, 1));
    if (shader->max_lights > 0) {
        shader->lights =
            Memory_Alloc(shader->max_lights * sizeof(OUTPUT_MESH_LIGHT));
        M_InitRing(
            &shader->light_ring, shader->max_lights * sizeof(OUTPUT_MESH_LIGHT),
            MAX(align, 1));
    }
    M_BindSingle(shader);
    TRX_GL_CheckError();

    *out_shader = shader;
    return OK;
}

void Output_MeshShader_Bind(OUTPUT_MESH_SHADER *const shader)
{
    const int32_t variant_idx = M_GetVariantIndex();
    Output_Shader_Bind(M_GetVariantBase(shader, variant_idx));
    M_UploadEnvMapRect(shader, variant_idx);
    M_UploadPulling(shader, variant_idx);
}

void Output_MeshShader_SuspendSubdivision(
    OUTPUT_MESH_SHADER *const shader, const bool is_suspended)
{
    m_SubdivisionSuspended = is_suspended;
    Output_MeshShader_Bind(shader);
}

void Output_MeshShader_Free(OUTPUT_MESH_SHADER *const shader)
{
    for (int32_t i = 0; i < M_VARIANT_COUNT; i++) {
        if (shader->base[i] != nullptr) {
            Output_Shader_Free(shader->base[i]);
        }
    }
    M_FreeRing(&shader->slot_ring);
    M_FreeRing(&shader->light_ring);
    Memory_FreePointer(&shader->lights);
    Memory_Free(shader);
}

void Output_MeshShader_UploadSlot(
    OUTPUT_MESH_SHADER *const shader, const OUTPUT_MESH_SLOT *const slot,
    const OUTPUT_MESH_LIGHT *const lights, const int32_t light_count)
{
    M_PutSlot(shader, slot);
    M_PutLights(shader, lights, light_count);
}

void Output_MeshShader_UploadLight(
    OUTPUT_MESH_SHADER *const shader, const float light[7][4])
{
    OUTPUT_MESH_SLOT slot = shader->slot;
    memcpy(slot.light, light, sizeof(slot.light));
    M_PutSlot(shader, &slot);
}

void Output_MeshShader_UploadModelMatrix(
    OUTPUT_MESH_SHADER *const shader, const MATRIX *const source)
{
    OUTPUT_MESH_SLOT slot = shader->slot;
    Output_FillMatrix(slot.model, source);
    M_PutSlot(shader, &slot);
}

int32_t Output_MeshShader_GetMaxSlots(const OUTPUT_MESH_SHADER *const shader)
{
    return shader->max_slots;
}

int32_t Output_MeshShader_GetMaxLights(const OUTPUT_MESH_SHADER *const shader)
{
    return shader->max_lights;
}

void Output_MeshShader_BindBatch(
    OUTPUT_MESH_SHADER *const shader, const OUTPUT_MESH_BATCH *const batch)
{
    glBindBufferRange(
        GL_UNIFORM_BUFFER, M_INSTANCES_BINDING, batch->slots,
        batch->slot_offset,
        (GLsizeiptr)shader->max_slots * sizeof(OUTPUT_MESH_SLOT));
    if (shader->max_lights > 0) {
        glBindBufferRange(
            GL_UNIFORM_BUFFER, M_INSTANCE_LIGHTS_BINDING, batch->lights,
            batch->light_offset,
            (GLsizeiptr)shader->max_lights * sizeof(OUTPUT_MESH_LIGHT));
    }
    glActiveTexture(GL_TEXTURE0 + M_GEOM_DATA_UNIT);
    glBindTexture(GL_TEXTURE_BUFFER, batch->geom_data);
    glActiveTexture(GL_TEXTURE0 + M_TEX_DATA_UNIT);
    glBindTexture(GL_TEXTURE_BUFFER, batch->tex_data);
    glActiveTexture(GL_TEXTURE0 + M_SHADE_DATA_UNIT);
    glBindTexture(GL_TEXTURE_BUFFER, batch->shade_data);
    glActiveTexture(GL_TEXTURE0 + M_GEOM_COLORS_UNIT);
    glBindTexture(GL_TEXTURE_BUFFER, batch->geom_colors);
    glActiveTexture(GL_TEXTURE0);

    shader->is_pulling_vertices_wanted = true;
    shader->vertex_base_wanted = batch->vertex_base;
    M_UploadPulling(shader, M_GetVariantIndex());
}

void Output_MeshShader_UnbindBatch(OUTPUT_MESH_SHADER *const shader)
{
    if (!shader->is_pulling_vertices_wanted) {
        return;
    }
    M_BindSingle(shader);
    shader->is_pulling_vertices_wanted = false;
    shader->vertex_base_wanted = 0;
    M_UploadPulling(shader, M_GetVariantIndex());
}

void Output_MeshShader_GetWaterEffectParams(
    const int32_t water_effect, float out[3])
{
    static const float m_ChoppyAmp[22] = {
        16.0f, 0.0f,   0.0f,   0.0f,   0.0f,   16.0f, 16.0f, 16.0f,
        16.0f, 53.0f,  53.0f,  53.0f,  53.0f,  90.0f, 90.0f, 90.0f,
        90.0f, 127.0f, 127.0f, 127.0f, 127.0f, 0.0f,
    };
    static const float m_ShimmerAmp[22] = {
        7.875f,   4.0f,     8.0f,     12.0f,    15.875f,  -3.875f,
        -7.875f,  -11.875f, -15.875f, -3.875f,  -7.875f,  -11.875f,
        -15.875f, -3.875f,  -7.875f,  -11.875f, -15.875f, -3.875f,
        -7.875f,  -11.875f, -15.875f, 0.0f,
    };
    static const float m_AbsIntensity[22] = {
        0.0f,  253.0f, 0.0f, 4.0f,  8.0f,  4.0f, 8.0f, 12.0f,
        16.0f, 4.0f,   8.0f, 12.0f, 16.0f, 4.0f, 8.0f, 12.0f,
        16.0f, 4.0f,   8.0f, 12.0f, 16.0f, 0.0f,
    };

    int32_t scheme = water_effect - 2;
    CLAMP(scheme, 0, 21);
    out[0] = m_ChoppyAmp[scheme];
    out[1] = m_ShimmerAmp[scheme];
    out[2] = m_AbsIntensity[scheme];
}

void Output_MeshShader_SetNoAlphaDiscard(
    OUTPUT_MESH_SHADER *const shader, const bool is_omitted)
{
    if (is_omitted == m_NoAlphaDiscard) {
        return;
    }
    m_NoAlphaDiscard = is_omitted;
    Output_MeshShader_Bind(shader);
}

void Output_MeshShader_UploadAlphaDiscard(
    OUTPUT_MESH_SHADER *const shader, const bool is_enabled)
{
    const int32_t variant_idx = M_GetVariantIndex();
    OUTPUT_SHADER *const base = M_GetVariantBase(shader, variant_idx);
    if (M_VariantOmitsDiscard(variant_idx)
        || is_enabled == shader->is_alpha_discard_enabled[variant_idx]) {
        return;
    }
    TRX_GL_TRACK_UNIFORM(
        glUniform1i, Output_Shader_LookupUniform(base, "uDiscardAlpha"),
        is_enabled);
    shader->is_alpha_discard_enabled[variant_idx] = is_enabled;
}

void Output_MeshShader_UploadWaterEffect(
    OUTPUT_MESH_SHADER *const shader, const int32_t water_effect)
{
    OUTPUT_MESH_SLOT slot = shader->slot;
    slot.water[2] = (float)water_effect;
    Output_MeshShader_GetWaterEffectParams(water_effect, slot.water_params);
    M_PutSlot(shader, &slot);
}

void Output_MeshShader_UploadWibbleEffect(
    OUTPUT_MESH_SHADER *const shader, const bool is_enabled)
{
    OUTPUT_MESH_SLOT slot = shader->slot;
    slot.water[3] = is_enabled ? 1.0f : 0.0f;
    M_PutSlot(shader, &slot);
}

void Output_MeshShader_UploadTint(
    OUTPUT_MESH_SHADER *const shader, const RGBA_F tint)
{
    OUTPUT_MESH_SLOT slot = shader->slot;
    slot.tint[0] = tint.r;
    slot.tint[1] = tint.g;
    slot.tint[2] = tint.b;
    slot.tint[3] = tint.a;
    M_PutSlot(shader, &slot);
}

void Output_MeshShader_UploadWaterLine(
    OUTPUT_MESH_SHADER *const shader, const bool is_enabled,
    const float world_y)
{
    OUTPUT_MESH_SLOT slot = shader->slot;
    slot.water[0] = is_enabled ? 1.0f : 0.0f;
    slot.water[1] = world_y;
    M_PutSlot(shader, &slot);
}

void Output_MeshShader_UploadSubmergedAmbient(
    OUTPUT_MESH_SHADER *const shader, const bool is_enabled, const RGB_F delta)
{
    OUTPUT_MESH_SLOT slot = shader->slot;
    slot.submerged[0] = delta.r;
    slot.submerged[1] = delta.g;
    slot.submerged[2] = delta.b;
    slot.submerged[3] = is_enabled ? 1.0f : 0.0f;
    M_PutSlot(shader, &slot);
}

void Output_MeshShader_UploadAmbientSpan(
    OUTPUT_MESH_SHADER *const shader, const bool is_enabled, const RGB_F from)
{
    OUTPUT_MESH_SLOT slot = shader->slot;
    slot.ambient_span[0] = from.r;
    slot.ambient_span[1] = from.g;
    slot.ambient_span[2] = from.b;
    slot.ambient_span[3] = is_enabled ? 1.0f : 0.0f;
    M_PutSlot(shader, &slot);
}
