#include "game/output/sources/ui.h"

#include "config.h"
#include "game/objects/common.h"
#include "game/output.h"
#include "game/output/scene_compositor.h"
#include "game/output/sources/objects.h"
#include "game/viewport.h"
#include "gfx/gl/utils.h"
#include "memory.h"
#include "utils.h"
#include "vector.h"

#define M_PICKUPS_FOV 65

typedef struct {
    XYZW_F pos;
    OUTPUT_UVW uvw;
    OUTPUT_TEXTURE_SIZE texture_size;
    OUTPUT_USHORT flags;
    OUTPUT_SHORT shade;
    RGBA_8888 color;
} M_VERTEX;

typedef struct {
    SCENE_SOURCE source;
    const SCENE_SOURCE *objects_source;
    VECTOR *scheduled_pickups;
    VECTOR *vertices;
    GLuint vao;
    GLuint vbo;
} M_PRIV;

static M_PRIV m_Priv = {};

VIEWPORT_RECT OutputSource_UI_GetPickupRect(
    const OUTPUT_UI_PICKUP *const pickup)
{
    const VIEWPORT_RECT viewport = Viewport_GetRect(VIEWPORT_UI);

    const float pickup_h = viewport.h * g_Config.ui.pickup_scale / 6;
    const float pickup_w = pickup_h * 5 / 4;
    const float window_padding_y = viewport.h / 16;
    const float window_padding_x = window_padding_y * 4 / 3;
    const float grid_padding_x = pickup_w / 8;
    const float grid_padding_y = pickup_h / 8;

    const float src_x = viewport.w + window_padding_x + pickup_w;
    const float src_y = viewport.h - window_padding_y - pickup_h / 2;

    const float dst_x = viewport.w - window_padding_x - pickup_w / 2
        - (pickup_w + grid_padding_x) * pickup->grid_x;
    const float dst_y = viewport.h - window_padding_y - pickup_h / 2
        - (pickup_h + grid_padding_y) * pickup->grid_y;

    const float x = src_x + (dst_x - src_x) * pickup->ease;
    const float y = src_y + (dst_y - src_y) * pickup->ease;
    return (VIEWPORT_RECT) {
        .x = x - pickup_w / 2,
        .y = y - pickup_h / 2,
        .w = pickup_w,
        .h = pickup_h,
    };
}

static float M_Get3DPickupScale(
    const VIEWPORT_RECT pickup_rect, const ANIM_FRAME *const frame)
{
    const XYZ_F obj_size = {
        .x = MAX(1, frame->bounds.max.x - frame->bounds.min.x),
        .y = MAX(1, frame->bounds.max.y - frame->bounds.min.y),
        .z = MAX(1, frame->bounds.max.z - frame->bounds.min.z),
    };

    // Reference scale that seems to works OK based on the following data:
    // pickup_rect: 480×360 (changes with window resizes)
    // key:         81  182 11
    // scion:       184 190 54
    // pistols:     215 57  146
    // shotgun:     365 123 147
    const float ref_scale = pickup_rect.w / 200.0f;

    // A scale factor to fit the mesh within pickup_rect,
    // ensuring it touches either side and is entirely contained.
    // clang-format off
    const float perfect_fit_scale = MIN3(
        pickup_rect.w / obj_size.x,
        pickup_rect.h / obj_size.y,
        pickup_rect.w / obj_size.z);
    // clang-format on

    // Some items are too big or too small – try to find a middle ground.
    return (ref_scale + perfect_fit_scale) / 2.0f;
}

static void M_Draw3DPickups(const M_PRIV *const p)
{
    for (int32_t i = 0; i < p->scheduled_pickups->count; i++) {
        if (p->objects_source->render_begin != nullptr) {
            p->objects_source->render_begin(p->objects_source);
        }

        const OUTPUT_UI_PICKUP *const pickup =
            Vector_Get(p->scheduled_pickups, i);
        const ANIM_FRAME *const frame =
            Object_GetAnim(pickup->object, 0)->frame_ptr;

        const VIEWPORT_RECT pickup_rect = OutputSource_UI_GetPickupRect(pickup);
        const XYZ_32 origin = {
            .x = pickup_rect.x + pickup_rect.w / 2,
            .y = pickup_rect.y + pickup_rect.h / 2,
            .z = (Output_GetNearZ_UI() + Output_GetFarZ_UI()) / 2,
        };

        const float scale = M_Get3DPickupScale(pickup_rect, frame);

        // Output_GetLightVectorView() used by the 3D object lighting needs a
        // W2V matrix to work. Set up something for it – probably wrong since
        // we're in an orthographic projection and the Z buffer has a different
        // operating range (Output_GetFarZ_UI vs Output_GetFarZ), but looks OK.
        const XYZ_32 camera = {
            .x = origin.x,
            .y = origin.y,
            .z = origin.z - WALL_L,
        };
        Matrix_LookAt(
            camera.x, camera.y, camera.z, origin.x, origin.y, origin.z, 0);

        Matrix_PushUnit();
        Matrix_TranslateSet(origin.x, origin.y, origin.z);
        Matrix_RotX(DEG_1 * 15);
        Matrix_RotY(-DEG_180);
        Matrix_RotY(pickup->rot_y);
        Matrix_Scale((1 << W2V_SHIFT) * scale);

        // Set up lighting for the pickup mesh.
        Output_SetLightDivider((1 << W2V_SHIFT) * 2);
        Output_SetLightAdder(SHADE_LOW);
        Output_RotateLight(DEG_1 * -30, DEG_1 * 45);

        Matrix_TranslateRel16(frame->offset);
        Matrix_TranslateRel32((XYZ_32) {
            .x = -(frame->bounds.min.x + frame->bounds.max.x) / 2,
            .y = -(frame->bounds.min.y + frame->bounds.max.y) / 2,
            .z = -(frame->bounds.min.z + frame->bounds.max.z) / 2,
        });
        Matrix_Rot16(frame->mesh_rots[0]);
        Object_DrawStaticObject(pickup->object, frame);
        Matrix_Pop();

        // Immediately flush scheduled object, so that it gets rendered
        // in the target viewport
        p->objects_source->render_pass(p->objects_source, SCENE_PASS_MESHES);
        p->objects_source->render_pass(
            p->objects_source, SCENE_PASS_TRANSPARENT);
        if (p->objects_source->render_end != nullptr) {
            p->objects_source->render_end(p->objects_source);
        }
    }
}

static void M_DrawVertices(const M_PRIV *const p)
{
    glBindVertexArray(p->vao);
    glBindBuffer(GL_ARRAY_BUFFER, p->vbo);
    glVertexAttrib3f(OUTPUT_MESH_ATTR_NORMAL, 0.0f, 0.0f, 0.0f);
    glVertexAttrib2f(OUTPUT_MESH_ATTR_TRAPEZOID_RATIO, 1.0f, 1.0f);
    GFX_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER, p->vertices->count * sizeof(M_VERTEX),
        Vector_GetData(p->vertices), GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, p->vertices->count);
}

static void M_RenderBegin(const SCENE_SOURCE *const source)
{
    M_PRIV *const p = &m_Priv;
    Vector_Clear(p->scheduled_pickups);
    Vector_Clear(p->vertices);
}

static void M_RenderPass(
    const SCENE_SOURCE *const source, const SCENE_PASS pass)
{
    M_PRIV *const p = &m_Priv;
    if (pass != SCENE_PASS_UI) {
        return;
    }

    if (p->scheduled_pickups->count == 0 && p->vertices->count == 0) {
        return;
    }

    if (p->scheduled_pickups->count > 0) {
        glEnable(GL_CULL_FACE);
        M_Draw3DPickups(p);
        glDisable(GL_CULL_FACE);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if (p->vertices->count > 0) {
        Output_Shader_UploadViewModelMatrix(
            Output_GetMeshShader(), &g_IDMatrix);
        M_DrawVertices(p);
    }
}

static bool M_IsDirty(const SCENE_SOURCE *const source, const SCENE_PASS pass)
{
    const M_PRIV *const p = &m_Priv;
    return pass == SCENE_PASS_UI
        && (p->scheduled_pickups->count > 0 || p->vertices->count > 0);
}

void OutputSource_UI_Init(void)
{
    M_PRIV *const p = &m_Priv;
    p->scheduled_pickups = Vector_Create(sizeof(OUTPUT_UI_PICKUP));
    p->vertices = Vector_CreateAtCapacity(sizeof(M_VERTEX), 500);
    p->source.render_begin = M_RenderBegin;
    p->source.render_pass = M_RenderPass;
    p->source.is_dirty = M_IsDirty;
    p->objects_source = OutputSource_Objects_GetSource();
    SceneCompositor_AddSource(&p->source);

    glGenVertexArrays(1, &p->vao);
    glBindVertexArray(p->vao);

    glGenBuffers(1, &p->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, p->vbo);

    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_POS);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_UVW);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_COLOR);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_SHADE);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_TEXTURE_SIZE);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_NORMAL);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_TRAPEZOID_RATIO);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_FLAGS);
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_POS, 4, GL_FLOAT, GL_FALSE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, pos));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_UVW, 3, GL_FLOAT, GL_FALSE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, uvw));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, color));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_SHADE, 1, OUTPUT_USHORT_GL, GL_FALSE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, shade));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_TEXTURE_SIZE, 4, GL_FLOAT, GL_FALSE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, texture_size));
    glVertexAttribIPointer(
        OUTPUT_MESH_ATTR_FLAGS, 1, OUTPUT_USHORT_GL, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, flags));
}

void OutputSource_UI_Shutdown(void)
{
    M_PRIV *const p = &m_Priv;
    Vector_Free(p->scheduled_pickups);
    Vector_Free(p->vertices);
}

void OutputSource_UI_StagePickup(const OUTPUT_UI_PICKUP pickup)
{
    M_PRIV *const p = &m_Priv;
    Vector_Add(p->scheduled_pickups, &pickup);
}

void OutputSource_UI_StageSprite(const OUTPUT_UI_SPRITE sprite)
{
    M_PRIV *const p = &m_Priv;

    const SPRITE_TEXTURE *const sprite_tex =
        Output_GetSpriteTexture(sprite.sprite_idx);

    const float u0 = (sprite_tex->offset & 0xFF) / 256.0f;
    const float v0 = (sprite_tex->offset >> 8) / 256.0f;
    const float u1 = u0 + sprite_tex->width / 65536.0f;
    const float v1 = v0 + sprite_tex->height / 65536.0f;

    M_VERTEX vertices[4];
    for (int32_t i = 0; i < 4; i++) {
        vertices[i].pos.z = sprite.z;
        vertices[i].pos.w = 0.0f;
        vertices[i].color.r = sprite.color.r;
        vertices[i].color.g = sprite.color.g;
        vertices[i].color.b = sprite.color.b;
        vertices[i].color.a = sprite.color.a;
        vertices[i].shade = sprite.shade;
        vertices[i].uvw.w = sprite_tex->tex_page;
        vertices[i].texture_size.x0 = u0;
        vertices[i].texture_size.y0 = v0;
        vertices[i].texture_size.x1 = u1;
        vertices[i].texture_size.y1 = v1;
        vertices[i].flags = VERT_NO_CAUSTICS;
    }

#define L_SET(vtx_idx, x_, y_, u_, v_)                                         \
    vertices[vtx_idx].pos.x = x_;                                              \
    vertices[vtx_idx].pos.y = y_;                                              \
    vertices[vtx_idx].uvw.u = u_;                                              \
    vertices[vtx_idx].uvw.v = v_;
    L_SET(0, sprite.x0, sprite.y0, u0, v0);
    L_SET(1, sprite.x1, sprite.y0, u1, v0);
    L_SET(2, sprite.x1, sprite.y1, u1, v1);
    L_SET(3, sprite.x0, sprite.y1, u0, v1);
#undef L_SET

    for (int32_t i = 0; i < OUTPUT_QUAD_VERTICES; i++) {
        const int32_t j = OUTPUT_QUAD_TO_FAN(i);
        Vector_Add(p->vertices, &vertices[j]);
    }
}

void OutputSource_UI_StageQuad(const OUTPUT_UI_QUAD quad)
{
    M_PRIV *const p = &m_Priv;

    M_VERTEX vertices[4];
    for (int32_t i = 0; i < 4; i++) {
        vertices[i].pos.z = quad.z;
        vertices[i].pos.w = 0.0f;
        vertices[i].shade = SHADE_NEUTRAL;
        vertices[i].flags =
            VERT_NO_LIGHTING | VERT_FLAT_SHADED | VERT_NO_CAUSTICS;
    }

#define L_SET(vtx_idx, x_, y_, color_)                                         \
    vertices[vtx_idx].pos.x = x_;                                              \
    vertices[vtx_idx].pos.y = y_;                                              \
    vertices[vtx_idx].color.r = color_.r;                                      \
    vertices[vtx_idx].color.g = color_.g;                                      \
    vertices[vtx_idx].color.b = color_.b;                                      \
    vertices[vtx_idx].color.a = color_.a;
    L_SET(0, quad.x0, quad.y0, quad.tl);
    L_SET(1, quad.x1, quad.y0, quad.tr);
    L_SET(2, quad.x1, quad.y1, quad.br);
    L_SET(3, quad.x0, quad.y1, quad.bl);
#undef L_SET

    for (int32_t i = 0; i < OUTPUT_QUAD_VERTICES; i++) {
        const int32_t j = OUTPUT_QUAD_TO_FAN(i);
        Vector_Add(p->vertices, &vertices[j]);
    }
}
