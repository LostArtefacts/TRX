#include <trx/game/output/binocular_mask.h>

#include <trx/game/objects/common.h>
#include <trx/game/output/shaders/mesh.h>
#include <trx/game/output/sources/objects.h>

// The laser sight mask feathers its rim triangles from a table rather than
// at one vertex, three values to a triangle, in the order the mesh lists its
// feathered triangles (see tomb4's TargetGraphColTab). Its quads feather as
// the binocular mask's do.
#define M_TARGET_TRIS 16
static const uint8_t m_TargetTriAlpha[M_TARGET_TRIS * 3] = {
    0,   0,   255, 0,   0,   255, 255, 255, 0,   255, 255, 0,   0, 0,   255, 0,
    0,   255, 255, 255, 0,   255, 255, 0,   0,   0,   255, 0,   0, 255, 0,   0,
    255, 0,   0,   255, 255, 255, 0,   255, 255, 0,   255, 255, 0, 255, 255, 0,
};
static const FACE *m_TargetTris[M_TARGET_TRIS];
static int32_t m_TargetTriCount = 0;

// The original game draws the binocular mask black with custom draw modes
// (see tomb4's InitBinoculars/DrawBinoculars): the solid faces are forced
// fully opaque (their level data flags them as additive, and additively
// blended black would be invisible), while the faces around the circle rims
// are alpha-blended with a per-vertex black-to-transparent gradient that
// feathers the mask into the scene.
static bool M_IsFeatheredFace(const FACE *const face)
{
    return (face->effects & 0x1u) != 0u;
}

static bool M_GetFacePass(const FACE *const face, SCENE_PASS *const pass)
{
    *pass =
        M_IsFeatheredFace(face) ? SCENE_PASS_TRANSPARENT : SCENE_PASS_OPAQUE;
    return true;
}

static bool M_GetVertexColor(
    const FACE *const face, const int32_t vertex_idx, RGBA_8888 *const color)
{
    bool opaque = true;
    if (M_IsFeatheredFace(face)) {
        // Same vertex gradient as the original: quads fade from their first
        // two vertices, triangles are opaque at their middle vertex only.
        opaque = face->vertex_count == 4 ? vertex_idx <= 1 : vertex_idx == 1;
    }
    *color = (RGBA_8888) { .r = 0, .g = 0, .b = 0, .a = opaque ? 255 : 0 };
    return true;
}

static bool M_GetVertexShade(
    const FACE *const face, const int32_t vertex_idx, int32_t *const shade)
{
    *shade = 0;
    return true;
}

// The mask lies over the screen rather than in the scene, so neither the
// fog nor the fog bulbs reach it.
static const OUTPUT_OBJECT_MESH_POLICY m_BinocularMaskMeshPolicy = {
    .vertex_flags = VERT_USE_OWN_LIGHT | VERT_NO_FOG,
    .get_face_pass = M_GetFacePass,
    .get_vertex_color = M_GetVertexColor,
    .get_vertex_shade = M_GetVertexShade,
};

static bool M_GetTargetVertexColor(
    const FACE *const face, const int32_t vertex_idx, RGBA_8888 *const color)
{
    if (face->vertex_count == 4 || !M_IsFeatheredFace(face)) {
        return M_GetVertexColor(face, vertex_idx, color);
    }
    uint8_t alpha = 255;
    for (int32_t i = 0; i < m_TargetTriCount; i++) {
        if (m_TargetTris[i] == face) {
            alpha = m_TargetTriAlpha[i * 3 + vertex_idx];
            break;
        }
    }
    *color = (RGBA_8888) { .r = 0, .g = 0, .b = 0, .a = alpha };
    return true;
}

static const OUTPUT_OBJECT_MESH_POLICY m_TargetMaskMeshPolicy = {
    .vertex_flags = VERT_USE_OWN_LIGHT | VERT_NO_FOG,
    .get_face_pass = M_GetFacePass,
    .get_vertex_color = M_GetTargetVertexColor,
    .get_vertex_shade = M_GetVertexShade,
};

static void M_ObserveTargetMask(void)
{
    m_TargetTriCount = 0;
    const OBJECT *const obj = Object_Get(O_TARGET_GFX);
    if (!obj->loaded) {
        return;
    }
    const OBJECT_MESH *const mesh = Object_GetMesh(obj->mesh_idx);
    for (int32_t i = 0;
         i < mesh->tex_face3s.count && m_TargetTriCount < M_TARGET_TRIS; i++) {
        const FACE *const face = &mesh->tex_face3s.data[i];
        if (M_IsFeatheredFace(face)) {
            m_TargetTris[m_TargetTriCount++] = face;
        }
    }
    for (int32_t i = 0; i < obj->mesh_count; i++) {
        OutputSource_Objects_AddMeshPolicy(
            obj->mesh_idx + i, &m_TargetMaskMeshPolicy);
    }
}

void Output_BinocularMask_ObserveLevelLoad(void)
{
    M_ObserveTargetMask();
    const OBJECT *const obj = Object_Get(O_BINOCULAR_GFX);
    if (!obj->loaded) {
        return;
    }
    for (int32_t i = 0; i < obj->mesh_count; i++) {
        OutputSource_Objects_AddMeshPolicy(
            obj->mesh_idx + i, &m_BinocularMaskMeshPolicy);
    }
}

void Output_BinocularMask_ObserveLevelUnload(void)
{
    OutputSource_Objects_RemoveMeshPolicy(&m_BinocularMaskMeshPolicy);
    OutputSource_Objects_RemoveMeshPolicy(&m_TargetMaskMeshPolicy);
    m_TargetTriCount = 0;
}
