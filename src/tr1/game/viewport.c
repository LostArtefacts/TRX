#include "game/viewport.h"

#include "config.h"
#include "game/screen.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/math.h"

static int32_t m_MinX = 0;
static int32_t m_MinY = 0;
static int32_t m_CenterX = 0;
static int32_t m_CenterY = 0;
static int32_t m_MaxX = 0;
static int32_t m_MaxY = 0;
static int32_t m_Width = 0;
static int32_t m_Height = 0;
static int16_t m_CurrentFOV = PASSPORT_FOV;

void Viewport_Init(int32_t x, int32_t y, int32_t width, int32_t height)
{
    m_MinX = x;
    m_MinY = y;
    m_MaxX = x + width - 1;
    m_MaxY = y + height - 1;
    m_CenterX = (m_MinX + m_MaxX) / 2;
    m_CenterY = (m_MinY + m_MaxY) / 2;
    m_Width = width;
    m_Height = height;

    g_PhdLeft = Viewport_GetMinX();
    g_PhdTop = Viewport_GetMinY();
    g_PhdRight = Viewport_GetMaxX();
    g_PhdBottom = Viewport_GetMaxY();
}

int32_t Viewport_GetMinX(void)
{
    return m_MinX;
}

int32_t Viewport_GetMinY(void)
{
    return m_MinY;
}

int32_t Viewport_GetCenterX(void)
{
    return m_CenterX;
}

int32_t Viewport_GetCenterY(void)
{
    return m_CenterY;
}

int32_t Viewport_GetMaxX(void)
{
    return m_MaxX;
}

int32_t Viewport_GetMaxY(void)
{
    return m_MaxY;
}

int32_t Viewport_GetWidth(void)
{
    return m_Width;
}

int32_t Viewport_GetHeight(void)
{
    return m_Height;
}

int16_t Viewport_GetFOV(void)
{
    return m_CurrentFOV == -1 ? Viewport_GetUserFOV() : m_CurrentFOV;
}

int16_t Viewport_GetUserFOV(void)
{
    return g_Config.fov_value * PHD_DEGREE;
}

void Viewport_SetFOV(int16_t fov)
{
    m_CurrentFOV = fov;
}
