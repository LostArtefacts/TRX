#include "game/output.h"
#include "game/output/scene_compositor.h"
#include "game/output/sources/objects.h"
#include "game/output/sources/rooms.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/output/textures.h>

static int32_t m_Time = 0;
static int32_t m_AnimatedTexturesOffset = 0;

static int32_t m_FogEnd = 0;
static RGB_F m_WaterColor = {};

static int32_t m_LsAdder = 0;
static int32_t m_LsDivider = 0;
static XYZ_32 m_LsVectorView = {};

static bool m_IsWibbleEffect = false;
static bool m_IsWaterEffect = false;
static bool m_IsShadeEffect = false;
static bool m_IsSkyboxEnabled = false;

int32_t Output_GetTime(void)
{
    return m_Time;
}

void Output_SetSkyboxEnabled(const bool enabled)
{
    m_IsSkyboxEnabled = enabled;
}

bool Output_IsSkyboxEnabled(void)
{
    return m_IsSkyboxEnabled && g_Config.visuals.enable_skybox;
}

int32_t Output_GetFogEnd(void)
{
    return m_FogEnd;
}

void Output_SetFogEnd(const int32_t dist)
{
    m_FogEnd = dist;

    const double near_z = Output_GetNearZ();
    const double far_z = Output_GetFarZ();
    const double res_z = 0.99 * near_z * far_z / (far_z - near_z);
    g_FltResZ = res_z;
    g_FltResZBuf = 0.005 + res_z / near_z;
}

int32_t Output_GetNearZ(void)
{
    return 20 << W2V_SHIFT;
}

int32_t Output_GetFarZ(void)
{
    return Output_GetFogEnd() << W2V_SHIFT;
}

void Output_SetupBelowWater(const bool underwater)
{
    m_IsWaterEffect = true;
    m_IsWibbleEffect = !underwater;
    m_IsShadeEffect = true;
}

void Output_SetupAboveWater(const bool underwater)
{
    m_IsWaterEffect = false;
    m_IsWibbleEffect = underwater;
    m_IsShadeEffect = underwater;
}

bool Output_GetWaterEffect(void)
{
    return m_IsWaterEffect;
}

bool Output_GetWibbleEffect(void)
{
    return m_IsWibbleEffect;
}

void Output_SetWaterColor(const RGB_888 color)
{
    m_WaterColor.r = color.r / 255.0f;
    m_WaterColor.g = color.g / 255.0f;
    m_WaterColor.b = color.b / 255.0f;
}

RGB_F Output_GetTint(void)
{
    if (m_IsShadeEffect) {
        return m_WaterColor;
    }
    return (RGB_F) { 1.0f, 1.0f, 1.0f };
}

int32_t Output_GetLightAdder(void)
{
    return m_LsAdder;
}

void Output_SetLightAdder(const int32_t adder)
{
    m_LsAdder = adder;
}

int32_t Output_GetLightDivider(void)
{
    return m_LsDivider;
}

void Output_SetLightDivider(const int32_t divider)
{
    m_LsDivider = divider;
}

XYZ_32 Output_GetLightVectorView(void)
{
    return m_LsVectorView;
}

void Output_RotateLight(const int16_t pitch, const int16_t yaw)
{
    const int32_t cp = Math_Cos(pitch);
    const int32_t sp = Math_Sin(pitch);
    const int32_t cy = Math_Cos(yaw);
    const int32_t sy = Math_Sin(yaw);
    const int32_t x = TRIGMULT2(cp, sy);
    const int32_t y = -sp;
    const int32_t z = TRIGMULT2(cp, cy);
    const MATRIX *const m = &g_W2VMatrix;
    m_LsVectorView.x = (m->_00 * x + m->_01 * y + m->_02 * z) >> W2V_SHIFT;
    m_LsVectorView.y = (m->_10 * x + m->_11 * y + m->_12 * z) >> W2V_SHIFT;
    m_LsVectorView.z = (m->_20 * x + m->_21 * y + m->_22 * z) >> W2V_SHIFT;
}

void Output_AnimateTextures(const int32_t num_frames)
{
    m_Time += num_frames;
    m_AnimatedTexturesOffset += num_frames;
    bool update = false;
    while (m_AnimatedTexturesOffset > 5) {
        Output_CycleAnimatedTextures();
        update = true;
        m_AnimatedTexturesOffset -= 5;
    }
    if (update) {
        Output_Textures_CycleAnimations();
        SceneCompositor_AnimateTextures();
    }
}

void Output_EnableScissor(
    const float x, const float y, const float w, const float h)
{
    // Causes the rendering pipeline to discard every pixel outside of the
    // specified window. The window is in game framebuffer viewport's
    // coordinates; to make it work properly, we need to translate it to the
    // SDL window coordinates first.

    // To deal with precision issues coming from using integer matrix ops
    const int32_t border = 4;

    const VIEWPORT_RECT game = Viewport_GetRect(VIEWPORT_GAME);
    const VIEWPORT_RECT window = Viewport_GetRect(VIEWPORT_GAME);
    const float scale_x = window.w / (float)game.w;
    const float scale_y = window.h / (float)game.h;
    VIEWPORT_RECT scissor = {
        .x = window.x + (x * scale_x) - border,
        .y = window.y + (game.h - y) * scale_y - border,
        .w = w * scale_x + border * 2,
        .h = h * scale_y + border * 2,
    };

    glEnable(GL_SCISSOR_TEST);
    glScissor(scissor.x, scissor.y, scissor.w, scissor.h);
}

void Output_DisableScissor(void)
{
    glDisable(GL_SCISSOR_TEST);
}
