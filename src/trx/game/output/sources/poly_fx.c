#include <trx/game/output/sources/poly_fx.h>

#include <trx/debug.h>
#include <trx/game/output.h>
#include <trx/game/output/scene_compositor.h>
#include <trx/game/output/shaders/mesh.h>
#include <trx/game/output/textures.h>
#include <trx/game/output/utils.h>
#include <trx/game/sparks.h>
#include <trx/utils.h>
#include <trx/vector.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    XYZW_F pos;
    XYZW_F normal;
    OUTPUT_UVW uvw;
    OUTPUT_TEXTURE_SIZE texture_size;
    float trapezoid_ratio[2];
    OUTPUT_USHORT flags;
    RGBA_8888 color;
    float shade;
} M_VERTEX;

typedef struct {
    int32_t sprite_idx;
    XYZ_32 world_pos[4];
    float disp[4][2];
    RGBA_8888 color[4];
    uint16_t flags;
} M_QUAD;

typedef struct {
    int32_t sort_key;
    const M_QUAD *quad;
} M_QUAD_SORT;

typedef struct {
    SCENE_SOURCE source;
    OUTPUT_MESH_SHADER *shader;
    VECTOR *scheduled_transparent; // M_QUAD
    VECTOR *scheduled_blend_add; // M_QUAD
    VECTOR *sorted; // M_QUAD_SORT
    VECTOR *vertices; // M_VERTEX
    GLuint vao;
    GLuint vbo;
} M_PRIV;

static M_PRIV m_Priv;

static int M_CompareQuadDepth(const void *const a, const void *const b)
{
    const M_QUAD_SORT *const prim_a = a;
    const M_QUAD_SORT *const prim_b = b;
    if (prim_b->sort_key == prim_a->sort_key) {
        return (intptr_t)prim_b->quad - (intptr_t)prim_a->quad;
    }
    return prim_b->sort_key - prim_a->sort_key;
}

static int32_t M_GetViewDepth(const XYZ_32 pos)
{
    // clang-format off
    return
        g_ViewMatrix._20 * pos.x +
        g_ViewMatrix._21 * pos.y +
        g_ViewMatrix._22 * pos.z +
        g_ViewMatrix._23;
    // clang-format on
}

static void M_SortPrims(M_PRIV *const p, const SCENE_PASS pass)
{
    Vector_Clear(p->sorted);

    const VECTOR *const quads = pass == SCENE_PASS_BLEND_ADD
        ? p->scheduled_blend_add
        : p->scheduled_transparent;

    for (int32_t i = 0; i < quads->count; i++) {
        const M_QUAD *const quad = Vector_Get(quads, i);
        const XYZ_32 centroid = {
            .x = (quad->world_pos[0].x + quad->world_pos[1].x
                  + quad->world_pos[2].x + quad->world_pos[3].x)
                / 4,
            .y = (quad->world_pos[0].y + quad->world_pos[1].y
                  + quad->world_pos[2].y + quad->world_pos[3].y)
                / 4,
            .z = (quad->world_pos[0].z + quad->world_pos[1].z
                  + quad->world_pos[2].z + quad->world_pos[3].z)
                / 4,
        };

        const M_QUAD_SORT sort = {
            .sort_key = M_GetViewDepth(centroid),
            .quad = quad,
        };
        Vector_Add(p->sorted, &sort);
    }

    if (p->sorted->count > 1) {
        qsort(
            Vector_GetData(p->sorted), p->sorted->count, sizeof(M_QUAD_SORT),
            M_CompareQuadDepth);
    }
}

static void M_EmitQuadVertices(M_PRIV *const p, const M_QUAD *const quad)
{
    const uint16_t flags = quad->flags;

    int32_t uvw_idx[4];
    OUTPUT_UVW uvw[4];
    OUTPUT_TEXTURE_SIZE texture_size[4];
    for (int32_t i = 0; i < 4; i++) {
        if (quad->sprite_idx >= 0) {
            uvw_idx[i] =
                Output_Textures_GetSpriteUVWIndex(quad->sprite_idx, (int16_t)i);
            uvw[i] = Output_Textures_GetUVW(uvw_idx[i]);
            texture_size[i] = Output_Textures_GetAtlasSize(uvw_idx[i] / 4);
        } else {
            uvw_idx[i] = 0;
            uvw[i] = (OUTPUT_UVW) { 0.0f, 0.0f, 0.0f };
            texture_size[i] = (OUTPUT_TEXTURE_SIZE) { 0.0f, 0.0f, 0.0f, 0.0f };
        }
    }

    const int32_t tri_idx[2][OUTPUT_QUAD_VERTICES] = {
        { 0, 1, 2, 0, 2, 3 }, // front
        { 0, 2, 1, 0, 3, 2 }, // back
    };

    for (int32_t side = 0; side < 2; side++) {
        for (int32_t i = 0; i < OUTPUT_QUAD_VERTICES; i++) {
            const int32_t corner = tri_idx[side][i];
            const M_VERTEX v = {
                .pos = {
                    .x = (float)quad->world_pos[corner].x,
                    .y = (float)quad->world_pos[corner].y,
                    .z = (float)quad->world_pos[corner].z,
                    .w = 0.0f,
                },
                .normal = {
                    .x = quad->disp[corner][0],
                    .y = quad->disp[corner][1],
                    .z = 0.0f,
                    .w = 0.0f,
                },
                .uvw = uvw[corner],
                .texture_size = texture_size[corner],
                .trapezoid_ratio = { 1.0f, 1.0f },
                .flags = flags,
                .color = quad->color[corner],
                .shade = (float)SHADE_NEUTRAL,
            };
            Vector_Add(p->vertices, &v);
        }
    }
}

static void M_RenderBegin(const SCENE_SOURCE *const source)
{
    M_PRIV *const p = &m_Priv;
    Vector_Clear(p->scheduled_transparent);
    Vector_Clear(p->scheduled_blend_add);
    Vector_Clear(p->vertices);
    Vector_Clear(p->sorted);
}

static void M_RenderPass(
    const SCENE_SOURCE *const source, const SCENE_PASS pass)
{
    M_PRIV *const p = &m_Priv;
    if (pass != SCENE_PASS_TRANSPARENT && pass != SCENE_PASS_BLEND_ADD) {
        return;
    }

    Vector_Clear(p->vertices);
    Vector_Clear(p->sorted);

    M_SortPrims(p, pass);
    for (int32_t i = 0; i < p->sorted->count; i++) {
        const M_QUAD_SORT *const sort = Vector_Get(p->sorted, i);
        M_EmitQuadVertices(p, sort->quad);
    }

    glBindVertexArray(p->vao);
    glBindBuffer(GL_ARRAY_BUFFER, p->vbo);

    GFX_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER, p->vertices->count * sizeof(M_VERTEX),
        Vector_GetData(p->vertices), GL_DYNAMIC_DRAW);

    Output_MeshShader_UploadTint(p->shader, Output_GetTint());
    Output_MeshShader_UploadWaterEffect(p->shader, 0);
    Output_MeshShader_UploadWibbleEffect(p->shader, false);
    Output_MeshShader_UploadModelMatrix(p->shader, &g_IDMatrix);
    glDrawArrays(GL_TRIANGLES, 0, p->vertices->count);
    GFX_GL_CheckError();
}

static bool M_IsDirty(const SCENE_SOURCE *const source, const SCENE_PASS pass)
{
    const M_PRIV *const p = &m_Priv;
    if (pass == SCENE_PASS_TRANSPARENT) {
        return p->scheduled_transparent->count > 0;
    }
    if (pass == SCENE_PASS_BLEND_ADD) {
        return p->scheduled_blend_add->count > 0;
    }
    return false;
}

void OutputSource_PolyFX_Init(void)
{
    M_PRIV *const p = &m_Priv;
    p->shader = Output_GetMeshShader();
    p->scheduled_transparent = Vector_Create(sizeof(M_QUAD));
    p->scheduled_blend_add = Vector_Create(sizeof(M_QUAD));
    p->sorted = Vector_Create(sizeof(M_QUAD_SORT));
    p->vertices = Vector_Create(sizeof(M_VERTEX));
    p->source.render_begin = M_RenderBegin;
    p->source.render_pass = M_RenderPass;
    p->source.is_dirty = M_IsDirty;
    SceneCompositor_AddSource(&p->source);

    glGenVertexArrays(1, &p->vao);
    glBindVertexArray(p->vao);

    glGenBuffers(1, &p->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, p->vbo);

    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_POS);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_NORMAL);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_UVW);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_TEXTURE_SIZE);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_TRAPEZOID_RATIO);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_FLAGS);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_COLOR);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_SHADE);

    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_POS, 4, GL_FLOAT, GL_FALSE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, pos));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_NORMAL, 4, GL_FLOAT, GL_FALSE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, normal));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_UVW, 3, GL_FLOAT, GL_FALSE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, uvw));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_TEXTURE_SIZE, 4, GL_FLOAT, GL_FALSE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, texture_size));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_TRAPEZOID_RATIO, 2, GL_FLOAT, GL_FALSE,
        sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, trapezoid_ratio));
    glVertexAttribIPointer(
        OUTPUT_MESH_ATTR_FLAGS, 1, OUTPUT_USHORT_GL, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, flags));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, color));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_SHADE, 1, GL_FLOAT, GL_FALSE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, shade));
}

void OutputSource_PolyFX_Shutdown(void)
{
    M_PRIV *const p = &m_Priv;
    if (p->scheduled_transparent != nullptr) {
        Vector_Free(p->scheduled_transparent);
        p->scheduled_transparent = nullptr;
    }
    if (p->scheduled_blend_add != nullptr) {
        Vector_Free(p->scheduled_blend_add);
        p->scheduled_blend_add = nullptr;
    }
    if (p->sorted != nullptr) {
        Vector_Free(p->sorted);
        p->sorted = nullptr;
    }
    if (p->vertices != nullptr) {
        Vector_Free(p->vertices);
        p->vertices = nullptr;
    }
    if (p->vao != 0) {
        glDeleteVertexArrays(1, &p->vao);
        p->vao = 0;
    }
    if (p->vbo != 0) {
        glDeleteBuffers(1, &p->vbo);
        p->vbo = 0;
    }
}

static void M_StageQuad(
    const int32_t sprite_idx, const XYZ_32 world_pos[4], const float disp[4][2],
    const RGBA_8888 color[4], const uint16_t flags, VECTOR *const target)
{
    M_QUAD quad;
    quad.sprite_idx = sprite_idx;
    memcpy(quad.world_pos, world_pos, sizeof(quad.world_pos));
    if (disp != nullptr) {
        memcpy(quad.disp, disp, sizeof(quad.disp));
    } else {
        memset(quad.disp, 0, sizeof(quad.disp));
    }
    memcpy(quad.color, color, sizeof(quad.color));
    quad.flags = flags;
    Vector_Add(target, &quad);
}

void OutputSource_PolyFX_StageSpriteQuadWorldTransparent(
    const int32_t sprite_idx, const XYZ_32 world_pos[4],
    const RGBA_8888 color[4])
{
    M_PRIV *const p = &m_Priv;
    M_StageQuad(
        sprite_idx, world_pos, nullptr, color,
        VERT_NO_LIGHTING | VERT_NO_WIBBLE, p->scheduled_transparent);
}

void OutputSource_PolyFX_StageSpriteQuadWorldBlendAdd(
    const int32_t sprite_idx, const XYZ_32 world_pos[4],
    const RGBA_8888 color[4])
{
    M_PRIV *const p = &m_Priv;
    M_StageQuad(
        sprite_idx, world_pos, nullptr, color,
        VERT_NO_LIGHTING | VERT_NO_WIBBLE, p->scheduled_blend_add);
}

void OutputSource_PolyFX_StageQuadTransparentExt(
    const int32_t sprite_idx, const XYZ_32 world_pos[4], const float disp[4][2],
    const RGBA_8888 color[4], const uint16_t flags)
{
    M_PRIV *const p = &m_Priv;
    M_StageQuad(
        sprite_idx, world_pos, disp, color, flags, p->scheduled_transparent);
}

void OutputSource_PolyFX_StageQuadBlendAddExt(
    const int32_t sprite_idx, const XYZ_32 world_pos[4], const float disp[4][2],
    const RGBA_8888 color[4], const uint16_t flags)
{
    M_PRIV *const p = &m_Priv;
    M_StageQuad(
        sprite_idx, world_pos, disp, color, flags, p->scheduled_blend_add);
}

void OutputSource_PolyFX_StageSpark(const SPARK *const spark)
{
    if (spark == nullptr || !spark->on) {
        return;
    }

    const XYZ_32 pos = Sparks_GetWorldPos(spark);
    const XYZ_32 world_pos[4] = { pos, pos, pos, pos };

    const int64_t zv = M_GetViewDepth(pos);
    const int64_t near_z = Output_GetNearZ();
    const int64_t far_z = Output_GetFarZ();
    if (zv <= near_z || zv >= far_z) {
        return;
    }

    int32_t vpos_z = (int32_t)(zv >> W2V_SHIFT);
    if (vpos_z == 0) {
        vpos_z = 1;
    }

    int32_t sw = (int32_t)spark->size.width;
    int32_t sh = (int32_t)spark->size.height;

    const bool use_sprite = (spark->flags & SPARK_F_SPRITE) != 0U;
    if ((spark->flags & SPARK_F_SCALE) != 0U) {
        const int32_t scalar = spark->scalar;
        sw = (int32_t)(((((int64_t)sw * g_PhdPersp) << scalar) / vpos_z));
        sh = (int32_t)(((((int64_t)sh * g_PhdPersp) << scalar) / vpos_z));

        if (use_sprite) {
            const int32_t max_w = (int32_t)spark->size.width << scalar;
            const int32_t max_h = (int32_t)spark->size.height << scalar;
            int32_t min_wh = 4;
            if ((spark->flags & SPARK_F_ATTACHED_NODE) != 0U
                && spark->node_num == 0U) {
                min_wh = 2;
            }
            CLAMP(sw, min_wh, max_w);
            CLAMP(sh, min_wh, max_h);
        } else {
            const int32_t max_w = (int32_t)spark->size.width << 2;
            const int32_t max_h = (int32_t)spark->size.height << 2;
            CLAMP(sw, 1, max_w);
            CLAMP(sh, 1, max_h);
        }
    }

    const float w = ((sw / 2.0f) * (float)vpos_z) / (float)g_PhdPersp;
    const float h = ((sh / 2.0f) * (float)vpos_z) / (float)g_PhdPersp;
    float disp[4][2] = {
        { -w, -h },
        { -w, h },
        { w, h },
        { w, -h },
    };

    const RGBA_8888 color = { spark->color.r, spark->color.g, spark->color.b,
                              255 };
    const RGBA_8888 world_color[4] = { color, color, color, color };

    if ((spark->flags & SPARK_F_ROTATE) != 0U) {
        const int32_t angle = (int32_t)spark->rot_angle * DEG_180 / 0xFFF.p0;
        const float s = Math_Sin(angle) / (float)(1 << W2V_SHIFT);
        const float c = Math_Cos(angle) / (float)(1 << W2V_SHIFT);
        for (int32_t i = 0; i < 4; i++) {
            const float x = disp[i][0];
            const float y = disp[i][1];
            disp[i][0] = x * c - y * s;
            disp[i][1] = x * s + y * c;
        }
    }

    uint16_t flags =
        VERT_NO_LIGHTING | VERT_NO_WIBBLE | VERT_BILLBOARD | VERT_ABS_SPRITE;
    int32_t sprite_idx = spark->sprite_idx;
    if ((spark->flags & SPARK_F_SPRITE) == 0U) {
        flags |= VERT_FLAT_SHADED;
        sprite_idx = -1;
    }
    M_PRIV *const p = &m_Priv;
    if (spark->draw_type == DRAW_BLEND_SUB) {
        ASSERT_FAIL(); // TODO: subtractive blend pass (TR3 TransType==3)
    } else if (spark->draw_type == DRAW_BLEND_ADD) {
        M_StageQuad(
            sprite_idx, world_pos, disp, world_color, flags,
            p->scheduled_blend_add);
    } else {
        M_StageQuad(
            sprite_idx, world_pos, disp, world_color, flags,
            p->scheduled_transparent);
    }
}
