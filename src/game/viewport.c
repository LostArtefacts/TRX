#include "game/viewport.h"

#include "config.h"
#include "game/screen.h"
#include "global/vars.h"
#include "math/math.h"

#include <math.h>

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

int32_t ViewPort_GetMinX(void)
{
    return m_MinX;
}

int32_t ViewPort_GetMinY(void)
{
    return m_MinY;
}

int32_t ViewPort_GetCenterX(void)
{
    return m_CenterX;
}

int32_t ViewPort_GetCenterY(void)
{
    return m_CenterY;
}

int32_t ViewPort_GetMaxX(void)
{
    return m_MaxX;
}

int32_t ViewPort_GetMaxY(void)
{
    return m_MaxY;
}

int32_t ViewPort_GetWidth(void)
{
    return m_Width;
}

int32_t ViewPort_GetHeight(void)
{
    return m_Height;
}

void ViewPort_AlterFOV(PHD_ANGLE fov)
{
    // In places that use GAME_FOV, it can be safely changed to user's choice.
    // But for cinematics, the FOV value chosen by devs needs to stay
    // unchanged, otherwise the game renders the low camera in the Lost Valley
    // cutscene wrong.
    if (g_Config.fov_vertical) {
        double aspect_ratio =
            Screen_GetResWidth() / (double)Screen_GetResHeight();
        double fov_rad_h = fov * M_PI / 32760;
        double fov_rad_v = 2 * atan(aspect_ratio * tan(fov_rad_h / 2));
        fov = round((fov_rad_v / M_PI) * 32760);
    }

    int16_t c = Math_Cos(fov / 2);
    int16_t s = Math_Sin(fov / 2);
    g_PhdPersp = ((Screen_GetResWidth() / 2) * c) / s;
}
