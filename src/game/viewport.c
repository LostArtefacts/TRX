#include "game/viewport.h"

#include "global/vars.h"

static int32_t m_MinX = 0;
static int32_t m_MinY = 0;
static int32_t m_CenterX = 0;
static int32_t m_CenterY = 0;
static int32_t m_MaxX = 0;
static int32_t m_MaxY = 0;
static int32_t m_Width = 0;
static int32_t m_Height = 0;

void ViewPort_Init(int32_t width, int32_t height)
{
    m_MinX = 0;
    m_MinY = 0;
    m_CenterX = width / 2;
    m_CenterY = height / 2;
    m_MaxX = width - 1;
    m_MaxY = height - 1;
    m_Width = width;
    m_Height = height;

    g_PhdLeft = ViewPort_GetMinX();
    g_PhdTop = ViewPort_GetMinY();
    g_PhdRight = ViewPort_GetMaxX();
    g_PhdBottom = ViewPort_GetMaxY();
}

int32_t ViewPort_GetMinX()
{
    return m_MinX;
}

int32_t ViewPort_GetMinY()
{
    return m_MinY;
}

int32_t ViewPort_GetCenterX()
{
    return m_CenterX;
}

int32_t ViewPort_GetCenterY()
{
    return m_CenterY;
}

int32_t ViewPort_GetMaxX()
{
    return m_MaxX;
}

int32_t ViewPort_GetMaxY()
{
    return m_MaxY;
}
int32_t ViewPort_GetWidth()
{
    return m_Width;
}

int32_t ViewPort_GetHeight()
{
    return m_Height;
}
