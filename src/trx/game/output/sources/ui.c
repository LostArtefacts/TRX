#include <trx/game/output/sources/ui.h>

#include <trx/config.h>
#include <trx/core/math/const.h>
#include <trx/core/math/func.h>
#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/game/anims/walk.h>
#include <trx/game/matrix.h>
#include <trx/game/objects/common.h>
#include <trx/game/output.h>
#include <trx/game/output/const.h>
#include <trx/game/output/func.h>
#include <trx/game/output/scene_compositor.h>
#include <trx/game/output/sources/objects.h>
#include <trx/game/output/state.h>
#include <trx/game/output/uniforms.h>
#include <trx/game/output/vars.h>
#include <trx/game/viewport.h>
#include <trx/gl/renderer.h>
#include <trx/gl/utils.h>
#include <trx/version.h>

#include <math.h>

// GL attribute mapping in the shader
typedef enum {
    // clang-format off
    M_ATTR_POS          = 0,
    M_ATTR_UVW          = 1,
    M_ATTR_TEXTURE_SIZE = 2,
    M_ATTR_FLAGS        = 3,
    M_ATTR_COLOR        = 4,
    // clang-format on
} M_VERTEX_ATTR;

typedef struct {
    XYZW_F pos;
    OUTPUT_UVW uvw;
    OUTPUT_TEXTURE_SIZE texture_size;
    OUTPUT_USHORT flags;
    RGBA_F color;
} M_VERTEX;

typedef struct {
    SCENE_SOURCE source;
    const SCENE_SOURCE *objects_source;
    VECTOR *scheduled_meshes;
    VECTOR *vertices;
    bool binocular_mask;
    bool laser_sight_mask;
    GLuint vao;
    GLuint vbo;
} M_PRIV;

static M_PRIV m_Priv = {};

// What a model is lit by where the script gives no light.
static const OUTPUT_UI_MESH_LIGHT m_PlainLight = {
    .ambient = { 1.0f, 1.0f, 1.0f },
};

static RGBA_F M_ToRGBA_F(const RGBA_8888 color)
{
    return (RGBA_F) {
        .r = color.r / 255.0f,
        .g = color.g / 255.0f,
        .b = color.b / 255.0f,
        .a = color.a / 255.0f,
    };
}

static float M_GetMeshScale(
    const VIEWPORT_RECT rect, const ANIM_FRAME *const frame)
{
    const XYZ_F obj_size = {
        .x = MAX(1, frame->bounds.max.x - frame->bounds.min.x),
        .y = MAX(1, frame->bounds.max.y - frame->bounds.min.y),
        .z = MAX(1, frame->bounds.max.z - frame->bounds.min.z),
    };

    // Reference scale that seems to works OK based on the following data:
    // rect: 480×360 (changes with window resizes)
    // key:         81  182 11
    // scion:       184 190 54
    // pistols:     215 57  146
    // shotgun:     365 123 147
    const float ref_scale = rect.w / 200.0f;

    // A scale factor to fit the mesh within the box,
    // ensuring it touches either side and is entirely contained.
    // clang-format off
    const float perfect_fit_scale = MIN3(
        rect.w / obj_size.x,
        rect.h / obj_size.y,
        rect.w / obj_size.z);
    // clang-format on

    // Some items are too big or too small – try to find a middle ground.
    return (ref_scale + perfect_fit_scale) / 2.0f;
}

// The brightness of a colour, from 0 to 255.
static int32_t M_GetGrey(const RGB_F color)
{
    return lroundf((color.r + color.g + color.b) / 3.0f * 255.0f);
}

static XYZ_32 M_Normalize(const XYZ_32 dir)
{
    const double length = sqrt(
        (double)dir.x * dir.x + (double)dir.y * dir.y + (double)dir.z * dir.z);
    if (length == 0.0) {
        return dir;
    }
    const double scale = (1 << W2V_SHIFT) / length;
    return (XYZ_32) {
        .x = (int32_t)lround(dir.x * scale),
        .y = (int32_t)lround(dir.y * scale),
        .z = (int32_t)lround(dir.z * scale),
    };
}

// Turns a direction by the rotation of the view matrix. Unlike a point, a
// direction takes none of the matrix's translation.
static XYZ_32 M_RotateToView(const XYZ_32 dir)
{
    const MATRIX *const m = &g_ViewMatrix;
    return (XYZ_32) {
        .x = (int32_t)(((int64_t)m->_00 * dir.x + (int64_t)m->_01 * dir.y
                        + (int64_t)m->_02 * dir.z)
                       >> W2V_SHIFT),
        .y = (int32_t)(((int64_t)m->_10 * dir.x + (int64_t)m->_11 * dir.y
                        + (int64_t)m->_12 * dir.z)
                       >> W2V_SHIFT),
        .z = (int32_t)(((int64_t)m->_20 * dir.x + (int64_t)m->_21 * dir.y
                        + (int64_t)m->_22 * dir.z)
                       >> W2V_SHIFT),
    };
}

// Lights what is drawn next by the light the script gave, each game in its own
// terms. In TR3 a mesh's own colour stands in for every light's.
static void M_ApplyLight(
    const OUTPUT_UI_MESH_LIGHT *const light, const RGB_F *const mesh_color)
{
    if (g_TRVersion == 4) {
        Output_CalculateStaticLightRGB_F(light->ambient);
    } else if (g_TRVersion == 3) {
        RGB_F colors[3] = {};
        XYZ_32 dirs[3] = {};
        for (int32_t i = 0; i < light->light_count; i++) {
            colors[i] =
                mesh_color != nullptr ? *mesh_color : light->lights[i].color;
            dirs[i] = light->lights[i].dir;
        }
        Output_SetTR3Light(light->ambient, colors, dirs);
    } else {
        // TR1 and TR2 shade a model by the ambient brightness, and lighten it
        // towards the first light by that light's brightness.
        Output_SetLightAdder(
            SHADE_NEUTRAL * 2 - M_GetGrey(light->ambient) * 32);
        const int32_t grey =
            light->light_count > 0 ? M_GetGrey(light->lights[0].color) : 0;
        if (grey > 0) {
            Output_SetLightDivider((1 << W2V_SHIFT) * 256 / grey);
            Output_SetLightVectorView(M_Normalize(light->lights[0].dir));
        } else {
            Output_SetLightVectorView((XYZ_32) {});
        }
    }
}

// Draws the model's meshes as Object_DrawStaticObject does, lighting each one
// the light names in its own colour.
static void M_DrawColoredMeshes(
    const OUTPUT_UI_MESH *const mesh, const ANIM_FRAME *const frame,
    const OUTPUT_UI_MESH_LIGHT *const light)
{
    const OBJECT *const obj = mesh->object;
    Matrix_Push();
    for (int32_t i = 0; i < obj->mesh_count && i < 32; i++) {
        if (i > 0) {
            const ANIM_BONE *const bone = Object_GetBone(obj, i - 1);
            if (bone->matrix_pop) {
                Matrix_Pop();
            }
            if (bone->matrix_push) {
                Matrix_Push();
            }
            Matrix_TranslateRel32(bone->pos);
            Matrix_Rot16(frame->mesh_rots[i]);
        }
        const uint32_t bit = 1u << i;
        if ((mesh->mesh_mask & bit) == 0) {
            continue;
        }
        M_ApplyLight(
            light, (light->lit_mask & bit) != 0 ? &light->colors[i] : nullptr);
        Object_DrawMesh(obj->mesh_idx + i, 0, false);
    }
    Matrix_Pop();
}

// Poses a model in its box on the canvas, seen flat.
static void M_PoseOnCanvas(
    const OUTPUT_UI_MESH *const mesh, const ANIM_FRAME *const frame)
{
    const VIEWPORT_RECT rect = mesh->rect;
    const float scale = M_GetMeshScale(rect, frame);
    const XYZ_32 origin = {
        .x = rect.x + rect.w / 2,
        .y = rect.y + rect.h / 2,
        .z = (OUTPUT_UI_NEAR_Z + OUTPUT_UI_FAR_Z) / 2,
    };

    // Lighting routines needs a W2V matrix to work; set up something for
    // it.
    Matrix_LookAt(
        origin.x, origin.y, origin.z - WALL_L, origin.x, origin.y, origin.z, 0);

    Matrix_PushUnit();
    Matrix_TranslateSet32(origin);
    Matrix_RotX(mesh->tilt);
    if (mesh->held) {
        Matrix_RotX(DEG_1 * 15);
        Matrix_RotY(-DEG_180);
    }
    Matrix_Rot16(mesh->rot);
    Matrix_Scale((1 << W2V_SHIFT) * scale);

    Matrix_TranslateRel16(frame->offset);
    Matrix_TranslateRel32((XYZ_32) {
        .x = -(frame->bounds.min.x + frame->bounds.max.x) / 2,
        .y = -(frame->bounds.min.y + frame->bounds.max.y) / 2,
        .z = -(frame->bounds.min.z + frame->bounds.max.z) / 2,
    });
    Matrix_Rot16(frame->mesh_rots[0]);
}

// Poses a model where it stands in a world of its own, as that world's camera
// sees it, the way the engine places and views a model in its own world.
static void M_PoseInWorld(const OUTPUT_UI_MESH_WORLD *const world)
{
    Viewport_AlterFOV(world->fov, FOV_MODE_PASSPORT);
    Output_ApplyFOV();
    int16_t angles[2];
    Math_GetVectorAngles(
        world->camera_target.x - world->camera_pos.x,
        world->camera_target.y - world->camera_pos.y,
        world->camera_target.z - world->camera_pos.z, angles);
    const XYZ_16 view_rot = { .x = angles[1], .y = angles[0], .z = 0 };
    Matrix_GenerateW2V(&world->camera_pos, &view_rot);
    if (world->center_x != 0.5f || world->center_y != 0.5f) {
        const float persp = (float)g_PhdPersp;
        Matrix_ShearW2V(
            (world->center_x - 0.5f) * Viewport_GetWidth(VIEWPORT_GAME) / persp,
            (world->center_y - 0.5f) * Viewport_GetHeight(VIEWPORT_GAME)
                / persp);
    }
    Output_Uniforms_UploadViewMatrix(Output_GetUniforms(), &g_ViewMatrix);

    Matrix_Push();
    Matrix_TranslateAbs32((XYZ_32) {});
    for (int32_t i = 0; i < world->step_count; i++) {
        const XYZ_32 vec = world->steps[i].vec;
        switch (world->steps[i].kind) {
        case OUTPUT_UI_MESH_STEP_TURN:
            Matrix_Rot16((XYZ_16) {
                .x = (int16_t)vec.x,
                .y = (int16_t)vec.y,
                .z = (int16_t)vec.z,
            });
            break;
        case OUTPUT_UI_MESH_STEP_SCALE:
            Matrix_Scale((1 << W2V_SHIFT) * vec.x / 1024);
            break;
        case OUTPUT_UI_MESH_STEP_MOVE:
            Matrix_TranslateRel32(vec);
            break;
        }
    }
}

// Returns the frame of a model's first animation, or nullptr past its last.
static const ANIM_FRAME *M_GetWorldFrame(
    const OBJECT *const object, const int32_t frame)
{
    const ANIM *const anim = Object_GetAnim(object, 0);
    if (frame < 0 || frame > anim->frame_end - anim->frame_base) {
        return nullptr;
    }
    return &object->frame_base[frame];
}

// Draws the joints in mesh_mask in a pose blended between two frames. At the
// pivot, the frame offset is left out.
static void M_DrawWorldPose(
    const OBJECT *const obj, const uint32_t mesh_mask,
    const int16_t *const extra_rotation, const ANIM_FRAME *const frame1,
    const ANIM_FRAME *const frame2, const int32_t frac, const int32_t rate,
    const bool at_pivot)
{
    BOUNDS_16 bounds = frame1->bounds;
    if (at_pivot) {
        bounds.min = XYZ_16_Subtract(bounds.min, frame1->offset);
        bounds.max = XYZ_16_Subtract(bounds.max, frame1->offset);
    }
    const CLIP clip = Object_GetClip(obj, &bounds);
    if (clip == CLIP_NOT_VISIBLE) {
        return;
    }

    Matrix_Push();
    ANIM_WALK walk;
    Anim_Walk_Begin(
        &walk,
        &(ANIM_WALK_DESC) {
            .obj = obj,
            .pose = Anim_Pose_FromFrames(frame1, frame2, frac, rate),
            .extra_rotations = extra_rotation,
            .applies_base_rot = true,
            .skips_frame_offset = at_pivot,
        });
    while (Anim_Walk_Next(&walk)) {
        if ((mesh_mask & (1u << walk.joint)) != 0) {
            Object_DrawMesh(
                obj->mesh_idx + walk.joint, clip, walk.interpolated);
        }
    }
    Anim_Walk_End(&walk);
    Matrix_Pop();
}

// Draws a model in a world of its own in its frame, blended towards the next,
// with the turns its bones take on top. The bones that take a turn keep the
// axes they turn about, as the engine's own animated bones do.
static void M_DrawWorldFrame(const OUTPUT_UI_MESH *const mesh)
{
    const OUTPUT_UI_MESH_WORLD *const world = mesh->world;
    const OBJECT *const obj = mesh->object;
    const ANIM_FRAME *const frame1 = M_GetWorldFrame(obj, world->frame);
    const ANIM_FRAME *frame2 = M_GetWorldFrame(obj, world->next_frame);
    if (frame1 == nullptr) {
        return;
    }
    if (frame2 == nullptr) {
        frame2 = frame1;
    }

    for (int32_t i = 0; i < world->bone_count; i++) {
        const int32_t bone = world->bones[i].bone;
        if (bone >= 0 && bone < obj->mesh_count - 1) {
            ANIM_BONE *const anim_bone = Object_GetBone(obj, bone);
            anim_bone->rot.x |= world->bones[i].axes.x;
            anim_bone->rot.y |= world->bones[i].axes.y;
            anim_bone->rot.z |= world->bones[i].axes.z;
        }
    }

    // The turns go in the order the bones are walked, each bone's as yaw,
    // pitch, then roll, which is the order the walk reads them back in.
    int16_t extra_rotation[(OUTPUT_UI_MESH_BONES + 32) * 3] = {};
    int32_t count = 0;
    for (int32_t bone = 0; bone < obj->mesh_count - 1 && bone < 32; bone++) {
        const ANIM_BONE *const anim_bone = Object_GetBone(obj, bone);
        XYZ_16 rot = {};
        for (int32_t i = 0; i < world->bone_count; i++) {
            if (world->bones[i].bone == bone) {
                rot = world->bones[i].rot;
            }
        }
        if (anim_bone->rot.y) {
            extra_rotation[count++] = rot.y;
        }
        if (anim_bone->rot.x) {
            extra_rotation[count++] = rot.x;
        }
        if (anim_bone->rot.z) {
            extra_rotation[count++] = rot.z;
        }
    }

    const int32_t rate = 1024;
    int32_t frac = (int32_t)(world->blend * rate + 0.5f);
    CLAMP(frac, 0, rate);

    // A mesh drawn see-through is drawn again on its own, so that the tint
    // that carries how solid it is reaches that mesh alone.
    const uint32_t faded_mask = world->faded_mask & mesh->mesh_mask;
    for (int32_t i = -1; i < 32; i++) {
        uint32_t mask = mesh->mesh_mask & ~faded_mask;
        if (i >= 0) {
            mask = 1u << i;
            if ((faded_mask & mask) == 0) {
                continue;
            }
            Output_PushTintOverride((RGBA_F) {
                .r = 1.0f,
                .g = 1.0f,
                .b = 1.0f,
                .a = world->mesh_opacity[i],
            });
        }
        M_DrawWorldPose(
            obj, mask, extra_rotation, frame1, frame2, frac, rate,
            world->at_pivot);
        if (i >= 0) {
            Output_PopTintOverride();
        }
    }
}

// Draws a model in a world of its own through the passes the scene draws
// its own models through, so that it looks as the scene would have shown it.
static void M_RenderWorldPasses(const M_PRIV *const p)
{
    OUTPUT_MESH_SHADER *const shader = Output_GetMeshShader();
    Output_MeshShader_UploadAlphaDiscard(shader, true);
    p->objects_source->render_pass(p->objects_source, SCENE_PASS_OPAQUE);
    Output_MeshShader_UploadAlphaDiscard(shader, false);
    glDepthMask(GL_FALSE);
    p->objects_source->render_pass(p->objects_source, SCENE_PASS_TRANSPARENT);
    SceneCompositor_SetSubtractiveBlend();
    p->objects_source->render_pass(p->objects_source, SCENE_PASS_BLEND_SUB);
    glBlendEquation(GL_FUNC_ADD);
    // Adds light and leaves the coverage alone, so that the scene under the
    // interface still shows through the face.
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
    p->objects_source->render_pass(p->objects_source, SCENE_PASS_BLEND_ADD);
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

static void M_DrawMeshes(const M_PRIV *const p)
{
    SceneCompositor_SetSamplerFilter(g_Config.rendering.texture_filter);
    Output_MeshShader_Bind(Output_GetMeshShader());
    // A model on the canvas is lit by its own fog, not by the room's.
    const int32_t old_fog_start = Output_GetFogStart();
    const int32_t old_fog_end = Output_GetFogEnd();
    Output_SetFogStart(20 * WALL_L);
    Output_SetFogEnd(100 * WALL_L);

    for (int32_t i = 0; i < p->scheduled_meshes->count; i++) {
        const OUTPUT_UI_MESH *const mesh = Vector_Get(p->scheduled_meshes, i);
        const ANIM_FRAME *const frame =
            Object_GetAnim(mesh->object, 0)->frame_ptr;
        // Tested before the batch opens, because the batch below is closed by
        // the flush at the end of the loop body.
        if (frame == nullptr) {
            continue;
        }

        const bool own_target =
            mesh->world != nullptr && TRX_GL_Renderer_BeginUiScenePass();
        if (p->objects_source->render_begin != nullptr) {
            p->objects_source->render_begin(p->objects_source);
        }
        // Each model hides its own far side, and only its own.
        glClear(GL_DEPTH_BUFFER_BIT);

        const int16_t old_fov = Viewport_GetSystemFOV();
        const FOV_MODE old_fov_mode = Viewport_GetFOVMode();
        OUTPUT_UI_MESH_LIGHT light =
            mesh->light != nullptr ? *mesh->light : m_PlainLight;
        if (mesh->world != nullptr) {
            M_PoseInWorld(mesh->world);
            Output_PushTintOverride((RGBA_F) {
                .r = 1.0f,
                .g = 1.0f,
                .b = 1.0f,
                .a = mesh->world->opacity,
            });
            for (int32_t j = 0; j < light.light_count; j++) {
                light.lights[j].dir = M_RotateToView(light.lights[j].dir);
            }
        } else {
            M_PoseOnCanvas(mesh, frame);
        }
        if (light.lit_mask != 0 && g_TRVersion == 3) {
            const ANIM_FRAME *posed = frame;
            if (mesh->world != nullptr) {
                posed = M_GetWorldFrame(mesh->object, mesh->world->frame);
                if (posed == nullptr) {
                    posed = frame;
                }
                Matrix_TranslateRel16(posed->offset);
                Matrix_Rot16(posed->mesh_rots[0]);
            }
            M_DrawColoredMeshes(mesh, posed, &light);
        } else if (mesh->world != nullptr) {
            M_ApplyLight(&light, nullptr);
            M_DrawWorldFrame(mesh);
        } else {
            M_ApplyLight(&light, nullptr);
            Object_DrawStaticObject(mesh->object, frame, mesh->mesh_mask);
        }
        if (mesh->world != nullptr) {
            Output_PopTintOverride();
        }
        Matrix_Pop();

        // Immediately flush scheduled object, so that it gets rendered
        // in the target viewport
        if (mesh->world != nullptr) {
            M_RenderWorldPasses(p);
        } else {
            p->objects_source->render_pass(
                p->objects_source, SCENE_PASS_OPAQUE);
            p->objects_source->render_pass(
                p->objects_source, SCENE_PASS_TRANSPARENT);
            glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE);
            p->objects_source->render_pass(
                p->objects_source, SCENE_PASS_BLEND_ADD);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        }
        if (p->objects_source->render_end != nullptr) {
            p->objects_source->render_end(p->objects_source);
        }
        if (own_target) {
            TRX_GL_Renderer_EndUiScenePass();
            SceneCompositor_BindTextures();
            Output_MeshShader_Bind(Output_GetMeshShader());
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
        }
        if (mesh->world != nullptr) {
            Viewport_AlterFOV(old_fov, old_fov_mode);
            Output_ApplyFOV();
            Output_Uniforms_UploadOrthoMatrix(Output_GetUniforms());
        }
    }

    Output_SetFogStart(old_fog_start);
    Output_SetFogEnd(old_fog_end);
    SceneCompositor_SetSamplerFilter(g_Config.rendering.ui_filter);
}

// Returns how wide the mask is drawn. The laser sight mask keeps the 4:3
// shape of the original screen, so that its circle stays round on a wider
// screen, and black bars fill the rest of it. The binocular mask spans the
// whole viewport.
static int32_t M_GetMaskWidth(
    const VIEWPORT_RECT viewport, const bool laser_sight)
{
    return laser_sight ? MIN(viewport.w, viewport.h * 4 / 3) : viewport.w;
}

static void M_DrawBinocularMask(const M_PRIV *const p)
{
    const OBJECT *const obj =
        Object_Get(p->laser_sight_mask ? O_TARGET_GFX : O_BINOCULAR_GFX);
    if (!obj->loaded) {
        return;
    }
    // The original maps each mesh to a 512x240 screen through a pre-scale
    // of its own (see tomb4's InitBinoculars and InitTarget): x*32/96,
    // y*30/224 for the binoculars, and x*80/96, y*60/224 for the laser
    // sight.
    const int64_t scale_x = p->laser_sight_mask ? 80 : 32;
    const int64_t scale_y = p->laser_sight_mask ? 60 : 30;

    Output_MeshShader_Bind(Output_GetMeshShader());
    if (p->objects_source->render_begin != nullptr) {
        p->objects_source->render_begin(p->objects_source);
    }

    const VIEWPORT_RECT viewport = Viewport_GetRect(VIEWPORT_UI);
    const XYZ_32 origin = {
        .x = viewport.x + viewport.w / 2,
        .y = viewport.y + viewport.h / 2,
        .z = (OUTPUT_UI_NEAR_Z + OUTPUT_UI_FAR_Z) / 2,
    };

    // Lighting routines need a W2V matrix to work; set up something for it.
    Matrix_LookAt(
        origin.x, origin.y, origin.z - WALL_L, origin.x, origin.y, origin.z, 0);
    Output_CalculateStaticLight(SHADE_NEUTRAL);

    Matrix_PushUnit();
    Matrix_TranslateSet32(origin);
    // Combined with the pre-scale, the mapping stretches the raw mesh over
    // the whole viewport. Flatten the depth.
    const int32_t mask_w = M_GetMaskWidth(viewport, p->laser_sight_mask);
    Matrix_ScaleX(
        (int32_t)(((int64_t)1 << W2V_SHIFT) * mask_w * scale_x / (512 * 96)));
    Matrix_ScaleY((int32_t)(((int64_t)1 << W2V_SHIFT) * viewport.h * scale_y
                            / (240 * 224)));
    Matrix_ScaleZ((1 << W2V_SHIFT) / 64);

    Object_DrawMesh(obj->mesh_idx, CLIP_FULLY_VISIBLE, false);
    Matrix_Pop();

    p->objects_source->render_pass(p->objects_source, SCENE_PASS_OPAQUE);
    p->objects_source->render_pass(p->objects_source, SCENE_PASS_TRANSPARENT);
    glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE);
    p->objects_source->render_pass(p->objects_source, SCENE_PASS_BLEND_ADD);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    if (p->objects_source->render_end != nullptr) {
        p->objects_source->render_end(p->objects_source);
    }

    Output_UIShader_Bind(Output_GetUIShader());
}

static void M_DrawVertices(const M_PRIV *const p)
{
    glBindVertexArray(p->vao);
    glBindBuffer(GL_ARRAY_BUFFER, p->vbo);
    TRX_GL_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER, p->vertices->count * sizeof(M_VERTEX),
        Vector_GetData(p->vertices), GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, p->vertices->count);
}

static void M_RenderBegin(const SCENE_SOURCE *const source)
{
    M_PRIV *const p = &m_Priv;
    Vector_Clear(p->scheduled_meshes);
    Vector_Clear(p->vertices);
    p->binocular_mask = false;
    p->laser_sight_mask = false;
}

static void M_RenderPass(
    const SCENE_SOURCE *const source, const SCENE_PASS pass)
{
    M_PRIV *const p = &m_Priv;
    if (pass != SCENE_PASS_UI) {
        return;
    }

    if (p->scheduled_meshes->count == 0 && p->vertices->count == 0
        && !p->binocular_mask) {
        return;
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if (p->binocular_mask) {
        M_DrawBinocularMask(p);
    }

    // The flat interface stacks in the order it was drawn in, not by depth.
    if (p->vertices->count > 0) {
        glDisable(GL_DEPTH_TEST);
        M_DrawVertices(p);
        glEnable(GL_DEPTH_TEST);
    }

    if (p->scheduled_meshes->count > 0) {
        glEnable(GL_CULL_FACE);
        M_DrawMeshes(p);
        glDisable(GL_CULL_FACE);
        Output_UIShader_Bind(Output_GetUIShader());
    }
}

static bool M_IsDirty(const SCENE_SOURCE *const source, const SCENE_PASS pass)
{
    const M_PRIV *const p = &m_Priv;
    return pass == SCENE_PASS_UI
        && (p->scheduled_meshes->count > 0 || p->vertices->count > 0
            || p->binocular_mask);
}

// Emit a ring band from r_inner to r_outer with per-edge vertex colors.
// inner_color is applied to the vertices at r_inner, outer_color to r_outer.
// Fading inner/outer colors to alpha=0 produces antialiased edges.
static void M_StageRingBand(
    M_PRIV *const p, const float cx, const float cy, const float ri,
    const float ro, const float z, const RGBA_F inner_color,
    const RGBA_F outer_color)
{
    const int32_t n = 32;
    for (int32_t i = 0; i < n; i++) {
        const float a0 = (float)(2.0 * M_PI) * i / n;
        const float a1 = (float)(2.0 * M_PI) * (i + 1) / n;
        const float cos0 = cosf(a0);
        const float sin0 = sinf(a0);
        const float cos1 = cosf(a1);
        const float sin1 = sinf(a1);

        // v[0]/v[1] = outer edge, v[2]/v[3] = inner edge
        M_VERTEX v[4] = {};
        v[0].pos = (XYZW_F) { cx + ro * cos0, cy + ro * sin0, z, 0.0f };
        v[1].pos = (XYZW_F) { cx + ro * cos1, cy + ro * sin1, z, 0.0f };
        v[2].pos = (XYZW_F) { cx + ri * cos0, cy + ri * sin0, z, 0.0f };
        v[3].pos = (XYZW_F) { cx + ri * cos1, cy + ri * sin1, z, 0.0f };
        v[0].flags = VERT_FLAT_SHADED;
        v[1].flags = VERT_FLAT_SHADED;
        v[2].flags = VERT_FLAT_SHADED;
        v[3].flags = VERT_FLAT_SHADED;
        v[0].color = outer_color;
        v[1].color = outer_color;
        v[2].color = inner_color;
        v[3].color = inner_color;

        Vector_Add(p->vertices, &v[0]);
        Vector_Add(p->vertices, &v[1]);
        Vector_Add(p->vertices, &v[2]);
        Vector_Add(p->vertices, &v[2]);
        Vector_Add(p->vertices, &v[1]);
        Vector_Add(p->vertices, &v[3]);
    }
}

// Stages the band of a line between two distances from its middle, measured
// along the normal, shaded from one color at the first to the other at the
// second.
static void M_StageLineBand(
    M_PRIV *const p, const OUTPUT_UI_LINE *const line, const float nx,
    const float ny, const float d0, const float d1, const RGBA_F color_0,
    const RGBA_F color_1)
{
    const float z = (float)line->z;
    M_VERTEX v[4] = {};
    v[0].pos = (XYZW_F) { line->x0 + nx * d0, line->y0 + ny * d0, z, 0.0f };
    v[1].pos = (XYZW_F) { line->x1 + nx * d0, line->y1 + ny * d0, z, 0.0f };
    v[2].pos = (XYZW_F) { line->x0 + nx * d1, line->y0 + ny * d1, z, 0.0f };
    v[3].pos = (XYZW_F) { line->x1 + nx * d1, line->y1 + ny * d1, z, 0.0f };
    for (int32_t i = 0; i < 4; i++) {
        v[i].flags = VERT_FLAT_SHADED;
        v[i].color = i < 2 ? color_0 : color_1;
    }

    Vector_Add(p->vertices, &v[0]);
    Vector_Add(p->vertices, &v[1]);
    Vector_Add(p->vertices, &v[2]);
    Vector_Add(p->vertices, &v[2]);
    Vector_Add(p->vertices, &v[1]);
    Vector_Add(p->vertices, &v[3]);
}

void OutputSource_UI_Init(void)
{
    M_PRIV *const p = &m_Priv;
    p->scheduled_meshes = Vector_Create(sizeof(OUTPUT_UI_MESH));
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

    glEnableVertexAttribArray(M_ATTR_POS);
    glEnableVertexAttribArray(M_ATTR_UVW);
    glEnableVertexAttribArray(M_ATTR_COLOR);
    glEnableVertexAttribArray(M_ATTR_TEXTURE_SIZE);
    glEnableVertexAttribArray(M_ATTR_FLAGS);
    glVertexAttribPointer(
        M_ATTR_POS, 4, GL_FLOAT, GL_FALSE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, pos));
    glVertexAttribPointer(
        M_ATTR_UVW, 3, GL_FLOAT, GL_FALSE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, uvw));
    glVertexAttribPointer(
        M_ATTR_COLOR, 4, GL_FLOAT, GL_TRUE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, color));
    glVertexAttribPointer(
        M_ATTR_TEXTURE_SIZE, 4, GL_FLOAT, GL_FALSE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, texture_size));
    glVertexAttribIPointer(
        M_ATTR_FLAGS, 1, OUTPUT_USHORT_GL, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, flags));
}

void OutputSource_UI_Shutdown(void)
{
    M_PRIV *const p = &m_Priv;
    if (p->scheduled_meshes != nullptr) {
        Vector_Free(p->scheduled_meshes);
        p->scheduled_meshes = nullptr;
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

void OutputSource_UI_StageMesh(const OUTPUT_UI_MESH mesh)
{
    M_PRIV *const p = &m_Priv;
    Vector_Add(p->scheduled_meshes, &mesh);
}

void OutputSource_UI_StageBinocularMask(const bool laser_sight)
{
    M_PRIV *const p = &m_Priv;
    p->binocular_mask = true;
    p->laser_sight_mask = laser_sight;

    const VIEWPORT_RECT viewport = Viewport_GetRect(VIEWPORT_UI);
    const int32_t bar_w =
        (viewport.w - M_GetMaskWidth(viewport, laser_sight)) / 2;
    if (bar_w <= 0) {
        return;
    }
    const RGBA_8888 black = { .r = 0, .g = 0, .b = 0, .a = 255 };
    const int32_t y0 = viewport.y;
    const int32_t y1 = viewport.y + viewport.h;
    // The bars reach a pixel into the mask, so that no seam shows between.
    const int32_t left_x1 = viewport.x + bar_w + 1;
    const int32_t right_x0 = viewport.x + viewport.w - bar_w - 1;
    OutputSource_UI_StageQuad((OUTPUT_UI_QUAD) {
        .x0 = viewport.x,
        .y0 = y0,
        .x1 = left_x1,
        .y1 = y1,
        .z = OUTPUT_UI_NEAR_Z,
        .tl = black,
        .tr = black,
        .bl = black,
        .br = black,
    });
    OutputSource_UI_StageQuad((OUTPUT_UI_QUAD) {
        .x0 = right_x0,
        .y0 = y0,
        .x1 = viewport.x + viewport.w,
        .y1 = y1,
        .z = OUTPUT_UI_NEAR_Z,
        .tl = black,
        .tr = black,
        .bl = black,
        .br = black,
    });
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
        vertices[i].color = sprite.color[i];
        vertices[i].uvw.w = sprite_tex->tex_page;
        vertices[i].texture_size.x0 = u0;
        vertices[i].texture_size.y0 = v0;
        vertices[i].texture_size.x1 = u1;
        vertices[i].texture_size.y1 = v1;
        vertices[i].flags = 0;
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

bool OutputSource_UI_HasStaged(void)
{
    const M_PRIV *const p = &m_Priv;
    return p->scheduled_meshes->count > 0 || p->vertices->count > 0
        || p->binocular_mask;
}

void OutputSource_UI_StageQuad(const OUTPUT_UI_QUAD quad)
{
    M_PRIV *const p = &m_Priv;

    M_VERTEX vertices[4];
    for (int32_t i = 0; i < 4; i++) {
        vertices[i].pos.z = quad.z;
        vertices[i].pos.w = 0.0f;
        vertices[i].flags = VERT_FLAT_SHADED;
    }

#define L_SET(vtx_idx, x_, y_, color_)                                         \
    vertices[vtx_idx].pos.x = x_;                                              \
    vertices[vtx_idx].pos.y = y_;                                              \
    vertices[vtx_idx].color = M_ToRGBA_F(color_);
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

void OutputSource_UI_StageCircle(const OUTPUT_UI_CIRCLE circle)
{
    M_PRIV *const p = &m_Priv;
    const float ri = (float)circle.r_inner;
    const float ro = (float)circle.r_outer;
    if (ro <= ri) {
        return;
    }

    const RGBA_F col = M_ToRGBA_F(circle.color);
    // Transparent edge color for antialiasing (same hue, zero alpha)
    const RGBA_F col_0 = { .r = col.r, .g = col.g, .b = col.b, .a = 0.0f };
    const float cx = (float)circle.cx;
    const float cy = (float)circle.cy;
    const float z = (float)circle.z;

    // Feather band width — creates the smooth edge via vertex alpha
    // interpolation
    const float feather = 1.5f;
    const float half_w = (ro - ri) * 0.5f;
    const float fw = feather < half_w ? feather : half_w;

    // Inner feather: ri → ri+fw, transparent→solid (rings only, not discs)
    if (ri > 0.0f) {
        M_StageRingBand(p, cx, cy, ri, ri + fw, z, col_0, col);
    }

    // Solid body: (ri+fw or ri) → ro-fw
    const float r_body_inner = ri > 0.0f ? ri + fw : ri;
    const float r_body_outer = ro - fw;
    if (r_body_inner < r_body_outer) {
        M_StageRingBand(p, cx, cy, r_body_inner, r_body_outer, z, col, col);
    }

    // Outer feather: ro-fw → ro, solid→transparent
    M_StageRingBand(p, cx, cy, ro - fw, ro, z, col, col_0);
}

void OutputSource_UI_StageLine(const OUTPUT_UI_LINE line)
{
    M_PRIV *const p = &m_Priv;
    const float dx = line.x1 - line.x0;
    const float dy = line.y1 - line.y0;
    const float length = sqrtf(dx * dx + dy * dy);
    if (length <= 0.0f || line.width <= 0.0f) {
        return;
    }

    const float nx = -dy / length;
    const float ny = dx / length;
    const RGBA_F col = M_ToRGBA_F(line.color);
    const RGBA_F col_0 = { .r = col.r, .g = col.g, .b = col.b, .a = 0.0f };

    // A one pixel feather on either side keeps a thin slanted line smooth.
    const float half_w = line.width * 0.5f;
    const float feather = 1.0f;
    M_StageLineBand(p, &line, nx, ny, -half_w, half_w, col, col);
    M_StageLineBand(p, &line, nx, ny, half_w, half_w + feather, col, col_0);
    M_StageLineBand(p, &line, nx, ny, -half_w, -half_w - feather, col, col_0);
}
