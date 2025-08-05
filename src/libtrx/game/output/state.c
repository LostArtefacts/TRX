#include "game/output/state.h"

#include "config.h"
#include "game/const.h"
#include "game/viewport.h"
#include "utils.h"

extern float g_FltResZ;
extern float g_FltResZBuf;

static int32_t m_FogStart = 0;

int32_t Output_GetFogStart(void)
{
    return MIN(m_FogStart, Output_GetFogEnd());
}

void Output_SetFogStart(const int32_t dist)
{
    m_FogStart = dist;
}

int32_t Output_GetNearZ_UI(void)
{
    return 20;
}

int32_t Output_GetFarZ_UI(void)
{
    return 10000;
}

void Output_GetPerspProjectionMatrix(GLfloat output[][4])
{
    const float left = 0.0f;
    const float top = 0.0f;
    const float right = Viewport_GetWidth(VIEWPORT_GAME);
    const float bottom = Viewport_GetHeight(VIEWPORT_GAME);
    const float near = Output_GetNearZ() / (float)(1 << W2V_SHIFT);
    const float far = Output_GetFarZ() / (float)(1 << W2V_SHIFT);
    const float aspect = (float)(right - left) / (float)(bottom - top);
    const float fov = Viewport_GetEffectiveFOV() * M_PI / (float)DEG_180;

    float f_x, f_y;
    if (g_Config.visuals.fov_vertical) {
        f_y = 1.0f / tanf(fov * 0.5f);
        f_x = f_y / aspect;
    } else {
        f_x = 1.0f / tanf(fov * 0.5f);
        f_y = f_x * aspect;
    }

    output[0][0] = f_x;
    output[0][1] = 0.0f;
    output[0][2] = 0.0f;
    output[0][3] = 0.0f;

    output[1][0] = 0.0f;
    output[1][1] = -f_y;
    output[1][2] = 0.0f;
    output[1][3] = 0.0f;

    output[2][0] = 0.0f;
    output[2][1] = 0.0f;
    output[2][2] = g_FltResZBuf;
    output[2][3] = -g_FltResZ / (float)(1 << W2V_SHIFT);

    output[3][0] = 0.0f;
    output[3][1] = 0.0f;
    output[3][2] = 1.0f;
    output[3][3] = 0.0f;
}

void Output_GetOrthoProjectionMatrix(GLfloat output[][4])
{
    const float left = 0.0f;
    const float top = 0.0f;
    const float right = Viewport_GetWidth(VIEWPORT_UI);
    const float bottom = Viewport_GetHeight(VIEWPORT_UI);
    const float near = Output_GetNearZ_UI();
    const float far = Output_GetFarZ_UI();

    output[0][0] = 2.0f / (right - left);
    output[0][1] = 0.0f;
    output[0][2] = 0.0f;
    output[0][3] = -(right + left) / (right - left);

    output[1][0] = 0.0f;
    output[1][1] = 2.0f / (top - bottom);
    output[1][2] = 0.0f;
    output[1][3] = -(top + bottom) / (top - bottom);

    output[2][0] = 0.0f;
    output[2][1] = 0.0f;
    output[2][2] = 2.0f / (far - near);
    output[2][3] = -(far + near) / (far - near);

    output[3][0] = 0.0f;
    output[3][1] = 0.0f;
    output[3][2] = 0.0f;
    output[3][3] = 1.0f;
}
