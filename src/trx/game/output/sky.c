#include <trx/game/output/sky.h>

#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/camera.h>
#include <trx/game/interpolation.h>
#include <trx/game/matrix.h>
#include <trx/game/objects/common.h>
#include <trx/game/output.h>
#include <trx/game/output/sources/sky.h>
#include <trx/game/output/state.h>
#include <trx/game/output/water.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/version.h>

static OUTPUT_SKY_LAYER m_Layers[OUTPUT_SKY_LAYER_COUNT] = {};
static int32_t m_LayerPos[OUTPUT_SKY_LAYER_COUNT] = {};
static int32_t m_LayerPosPrev[OUTPUT_SKY_LAYER_COUNT] = {};
static bool m_ColorAdd = false;
static bool m_FogGradient = false;
static int32_t m_TexturePage = -1;

static bool m_LightningEnabled = false;
static int32_t m_LightningCount = 0;
static int32_t m_LightningRand = 0;
static int32_t m_dLightningRand = 0;
static int32_t m_LightningSFXDelay = -1;
static int32_t m_LightningBase[3] = {};
static int32_t m_LightningRGB[3] = {};

static bool M_IsSkyVisible(void)
{
    // Like the OG engines, count the sky as visible when any drawn room is
    // outside-flagged, not just the camera's own room.
    return Output_IsSkyboxEnabled() && Room_IsSkyVisible();
}

static void M_UpdateLightningColor(void)
{
    if (m_LightningCount <= 0) {
        if (m_LightningRand < 4) {
            m_LightningRand = 0;
        } else {
            m_LightningRand -= m_LightningRand >> 2;
        }
    } else {
        m_LightningCount--;
        if (m_LightningCount != 0) {
            m_dLightningRand = Random_GetDraw() & 0x1FF;
            m_LightningRand += (m_dLightningRand - m_LightningRand) >> 1;
        } else {
            m_dLightningRand = 0;
            m_LightningRand = (Random_GetDraw() & 0x7F) + 400;
        }
    }

    for (int32_t i = 0; i < 3; i++) {
        m_LightningRGB[i] =
            m_LightningBase[i] + ((m_LightningBase[i] * m_LightningRand) >> 8);
        CLAMPG(m_LightningRGB[i], 255);
    }
}

static void M_UpdateLightning(void)
{
    if (m_LightningCount == 0 && m_LightningRand == 0) {
        if ((Random_GetDraw() & 127) == 0) {
            m_LightningCount = (Random_GetDraw() & 0x1F) + 16;
            m_dLightningRand = (Random_GetDraw() & 0xFF) + 256;
            m_LightningSFXDelay = (Random_GetDraw() & 3) + 12;
        }
    } else {
        M_UpdateLightningColor();
        if (m_LightningSFXDelay > -1) {
            m_LightningSFXDelay--;
        }
        if (m_LightningSFXDelay == 0) {
            Sound_Effect(SFX_THUNDER, nullptr, SPM_ALWAYS);
        }
    }
}

// TR4 skybox quads flagged in their texture info use a vertical alpha
// gradient to fade the horizon mesh into the flat sky layers, unless the
// level requests additive blending for the whole mesh.
static bool M_IsGradientFace(const FACE *const face)
{
    return g_TRVersion == 4 && (face->effects & 0x1u) != 0u && !m_ColorAdd;
}

static bool M_GetFacePass(const FACE *const face, SCENE_PASS *const pass)
{
    if (!M_IsGradientFace(face)) {
        return false;
    }
    *pass = SCENE_PASS_TRANSPARENT;
    return true;
}

static bool M_GetVertexColor(
    const FACE *const face, const int32_t vertex_idx, RGBA_8888 *const color)
{
    if (face->vertex_count != 4 || !M_IsGradientFace(face)) {
        return false;
    }
    // Top edge fades out to reveal the flat sky layers.
    *color = (RGBA_8888) { 0, 0, 0, vertex_idx < 2 ? 0 : 255 };
    return true;
}

// The OG engines light the skybox with a flat ambient of 128 and no lights,
// which the vertex color split then doubles to 255 - i.e. the mesh texture is
// drawn at full brightness. Model that with a 1.0 ambient and no per-vertex
// shade dimming.
static bool M_GetVertexShade(
    const FACE *const face, const int32_t vertex_idx, int32_t *const shade)
{
    // TR1/2 spell "no dimming" as SHADE_NEUTRAL: their shade multiplier is
    // 2 - shade / SHADE_NEUTRAL, so a 0 shade would double the texture.
    *shade = g_TRVersion < 3 ? SHADE_NEUTRAL : 0;
    return true;
}

// Applies the flat, direction-less skybox lighting (TR3+ only).
static void M_AdjustLightInfo(OUTPUT_LIGHT_INFO *const info)
{
    if (g_TRVersion < 3) {
        return;
    }
    if (g_TRVersion == 4) {
        // Neutral full brightness (the OG flat ambient of 128), expressed in
        // the convention of whichever path the mesh data selects: meshes
        // with normals are object-lit and read the ambient as a 128-neutral
        // color (1.0 here, scaled by 128/255 on upload); prelit meshes are
        // own-lit and read it as a multiplier over the vertex prelight.
        const OBJECT *const skybox = Object_Get(O_SKYBOX);
        const OBJECT_MESH *const mesh = Object_GetMesh(skybox->mesh_idx);
        const float ambient = mesh->num_lights > 0 ? 1.0f : 128.0f / 255.0f;
        info->tr3_ambient = (RGB_F) { ambient, ambient, ambient };
    } else {
        info->tr3_ambient = (RGB_F) { 0.5f, 0.5f, 0.5f };
    }
    for (int32_t i = 0; i < 3; i++) {
        info->tr3_light_color[i] = (RGB_F) { 0.0f, 0.0f, 0.0f };
        info->tr3_light_dir_view[i] = (XYZ_32) { 0, 0, 0 };
    }
}

// The scroll advances on the 30 FPS logic tick; interpolate it between ticks
// so the clouds move smoothly at higher render frame rates.
static int32_t M_GetInterpolatedLayerPos(const int32_t layer_idx)
{
    const int32_t pos = m_LayerPos[layer_idx];
    int32_t delta = pos - m_LayerPosPrev[layer_idx];
    if (delta > OUTPUT_SKY_WRAP / 2) {
        delta -= OUTPUT_SKY_WRAP;
    } else if (delta < -OUTPUT_SKY_WRAP / 2) {
        delta += OUTPUT_SKY_WRAP;
    }
    return pos - delta + (int32_t)(delta * Interpolation_GetWorldRate());
}

static void M_StageLayers(void)
{
    for (int32_t i = 0; i < OUTPUT_SKY_LAYER_COUNT; i++) {
        const OUTPUT_SKY_LAYER *const layer = &m_Layers[i];
        if (!layer->enabled) {
            continue;
        }
        Matrix_PushUnit();
        Matrix_TranslateAbs32(g_ViewPos);
        if (m_Layers[0].enabled) {
            // OG rotates layer 1 by ~180 degrees and never pops that rotation
            // before drawing layer 2, so with layer 1 enabled both layers face
            // this way.
            Matrix_Rot16((XYZ_16) { .y = 32760 });
        }
        Matrix_TranslateRel(M_GetInterpolatedLayerPos(i), -1536, 0);
        OutputSource_Sky_StageLayer(
            g_WMatrixPtr, Output_Sky_GetLayerDrawColor(i), i == 1);
        Matrix_Pop();
    }
}

void Output_Sky_Reset(void)
{
    for (int32_t i = 0; i < OUTPUT_SKY_LAYER_COUNT; i++) {
        m_Layers[i] = (OUTPUT_SKY_LAYER) {};
        m_LayerPos[i] = 0;
        m_LayerPosPrev[i] = 0;
    }
    m_ColorAdd = false;
    m_FogGradient = false;
    m_TexturePage = -1;

    OutputSource_Sky_InvalidateFogGradient();

    m_LightningEnabled = false;
    m_LightningCount = 0;
    m_LightningRand = 0;
    m_dLightningRand = 0;
    m_LightningSFXDelay = -1;
    for (int32_t i = 0; i < 3; i++) {
        m_LightningBase[i] = 0;
        m_LightningRGB[i] = 0;
    }
}

void Output_Sky_SetLayer(
    const int32_t layer_idx, const RGB_888 color, const int16_t speed)
{
    ASSERT(layer_idx >= 0 && layer_idx < OUTPUT_SKY_LAYER_COUNT);
    m_Layers[layer_idx] = (OUTPUT_SKY_LAYER) {
        .enabled = true,
        .color = color,
        .speed = speed,
    };
    m_LightningBase[0] = color.r;
    m_LightningBase[1] = color.g;
    m_LightningBase[2] = color.b;
    m_LightningRGB[0] = color.r;
    m_LightningRGB[1] = color.g;
    m_LightningRGB[2] = color.b;
}

void Output_Sky_SetColorAdd(const bool enabled)
{
    m_ColorAdd = enabled;
}

void Output_Sky_SetFogGradient(const bool enabled)
{
    m_FogGradient = enabled;
}

void Output_Sky_SetLightningEnabled(const bool enabled)
{
    m_LightningEnabled = enabled;
}

void Output_Sky_SetTexturePage(const int32_t page_idx)
{
    m_TexturePage = page_idx;
}

int32_t Output_Sky_GetTexturePage(void)
{
    return m_TexturePage;
}

const OUTPUT_SKY_LAYER *Output_Sky_GetLayer(const int32_t layer_idx)
{
    ASSERT(layer_idx >= 0 && layer_idx < OUTPUT_SKY_LAYER_COUNT);
    return &m_Layers[layer_idx];
}

int32_t Output_Sky_GetLayerPos(const int32_t layer_idx)
{
    ASSERT(layer_idx >= 0 && layer_idx < OUTPUT_SKY_LAYER_COUNT);
    return m_LayerPos[layer_idx];
}

// Returns the layer color (with any lightning flash applied) in the OG
// 128-neutral scale; the shader's VERT_OVERBRIGHT path doubles it and turns
// the excess into an additive overbright term (OG's CalcColorSplit).
RGB_888 Output_Sky_GetLayerDrawColor(const int32_t layer_idx)
{
    ASSERT(layer_idx >= 0 && layer_idx < OUTPUT_SKY_LAYER_COUNT);
    if (layer_idx == 0 && m_LightningEnabled) {
        return (RGB_888) {
            .r = m_LightningRGB[0],
            .g = m_LightningRGB[1],
            .b = m_LightningRGB[2],
        };
    }
    return m_Layers[layer_idx].color;
}

bool Output_Sky_IsColorAdd(void)
{
    return m_ColorAdd;
}

void Output_Sky_Update(void)
{
    if (g_TRVersion != 4) {
        return;
    }

    for (int32_t i = 0; i < OUTPUT_SKY_LAYER_COUNT; i++) {
        if (!m_Layers[i].enabled) {
            continue;
        }
        m_LayerPosPrev[i] = m_LayerPos[i];
        m_LayerPos[i] += m_Layers[i].speed;
        if (m_LayerPos[i] > OUTPUT_SKY_WRAP) {
            m_LayerPos[i] -= OUTPUT_SKY_WRAP;
        } else if (m_LayerPos[i] < 0) {
            m_LayerPos[i] += OUTPUT_SKY_WRAP;
        }
    }

    if (m_LightningEnabled && M_IsSkyVisible()) {
        M_UpdateLightning();
    }
}

static OUTPUT_OBJECT_MESH_POLICY m_SkyboxMeshPolicy = {
    .vertex_flags = 0,
    .get_face_pass = M_GetFacePass,
    .get_vertex_color = M_GetVertexColor,
    .get_vertex_shade = M_GetVertexShade,
    .adjust_light_info = M_AdjustLightInfo,
};

void Output_Sky_ObserveLevelLoad(void)
{
    const OBJECT *const skybox = Object_Get(O_SKYBOX);
    if (!skybox->loaded) {
        return;
    }
    // Pin TR1/2 to the own-light path so the horizon reads only the sky shade
    // adder, whichever way the level's mesh data is lit. TR3/TR4 must keep the
    // path their mesh data selects: forcing own light there makes the horizon
    // eligible for the dynamic point-light term (the binoculars lighting it).
    // The OG leaves the horizon unfogged: every sky vertex carries a full fog
    // factor (output.cpp ProcessObjectMeshVertices), and the levels that want
    // a fogged horizon paint the gradient on themselves.
    m_SkyboxMeshPolicy.vertex_flags =
        VERT_NO_FOG | (g_TRVersion < 3 ? VERT_USE_OWN_LIGHT : 0);
    for (int32_t i = 0; i < skybox->mesh_count; i++) {
        OutputSource_Objects_AddMeshPolicy(
            skybox->mesh_idx + i, &m_SkyboxMeshPolicy);
    }
}

void Output_Sky_ObserveLevelUnload(void)
{
    OutputSource_Objects_RemoveMeshPolicy(&m_SkyboxMeshPolicy);
}

bool Output_Sky_Draw(void)
{
    const OBJECT *const skybox = Object_Get(O_SKYBOX);
    if (!skybox->loaded) {
        return false;
    }

    Output_Water_SetupAboveWater(g_Camera.underwater);
    if (g_TRVersion == 4) {
        M_StageLayers();
    }
    Matrix_PushUnit();
    Matrix_TranslateAbs32(g_ViewPos);
    Matrix_Rot16(skybox->frame_base->mesh_rots[0]);
    Output_CalculateStaticLight(Output_GetSkyShade());
    const OBJECT_MESH *const mesh = Object_GetMesh(skybox->mesh_idx);
    if (g_TRVersion == 4 && m_FogGradient) {
        OutputSource_Sky_StageFogGradient(
            g_WMatrixPtr, mesh, Output_GetFogColor());
    }
    OutputSource_Objects_StageObjectMesh(mesh);
    SceneCompositor_Flush();
    Matrix_Pop();
    return true;
}
