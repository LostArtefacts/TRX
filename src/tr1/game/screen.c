#include "game/screen.h"

#include "config.h"
#include "game/output.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/types.h"
#include "math/matrix.h"
#include "specific/s_shell.h"

#include <libtrx/utils.h>

#include <math.h>

static int32_t m_ResolutionIdx = 0;
static int32_t m_ResolutionsCount = 0;
static RESOLUTION m_Resolutions[] = {
    // clang-format off
    { 0, 0 } /* desktop */,
    { 640, 480 },
    { 800, 600 },
    { 1024, 768 },
    { 1280, 720 },
    { 1920, 1080 },
    { 2560, 1440 },
    { 3840, 2160 },
    { 4096, 2160 },
    { 7680, 4320 },
    { -1, -1 },
    // clang-format on
};

static void M_ApplyResolution(void);
static int32_t M_GetRenderScaleBase(
    int32_t unit, int32_t base_width, int32_t base_height, double factor);

static void M_ApplyResolution(void)
{
    int32_t width = Screen_GetResWidth();
    int32_t height = Screen_GetResHeight();
    Viewport_Init(0, 0, width, height);

    Output_ApplyRenderSettings();

    Matrix_ResetStack();
    Viewport_SetFOV(Viewport_GetUserFOV());
}

static int32_t M_GetRenderScaleBase(
    int32_t unit, int32_t base_width, int32_t base_height, double factor)
{
    int32_t scale_x = Screen_GetResWidth() > base_width
        ? ((double)Screen_GetResWidth() * unit * factor) / base_width
        : unit * factor;
    int32_t scale_y = Screen_GetResHeight() > base_height
        ? ((double)Screen_GetResHeight() * unit * factor) / base_height
        : unit * factor;
    return MIN(scale_x, scale_y);
}

void Screen_Init(void)
{
    RESOLUTION *res;

    // count resolutions
    res = &m_Resolutions[0];
    m_ResolutionsCount = 0;
    while (res->width != -1) {
        res++;
        m_ResolutionsCount++;
    }

    // set the first resolution size to desktop size
    res = &m_Resolutions[0];
    res->width = S_Shell_GetCurrentDisplayWidth();
    res->height = S_Shell_GetCurrentDisplayHeight();

    // select matching resolution from config
    if (g_Config.resolution_width > 0 && g_Config.resolution_height > 0) {
        res = &m_Resolutions[0];
        m_ResolutionIdx = -1;
        while (res->width != -1) {
            if (g_Config.resolution_width == res->width
                && g_Config.resolution_height == res->height) {
                m_ResolutionIdx = res - m_Resolutions;
            }
            res++;
        }

        // if the user-supplied size is odd, override the default desktop
        // resolution with the user choice
        if (m_ResolutionIdx == -1) {
            res = &m_Resolutions[0];
            res->width = g_Config.resolution_width;
            res->height = g_Config.resolution_height;
            m_ResolutionIdx = 0;
        }
    } else {
        m_ResolutionIdx = 0;
    }

    M_ApplyResolution();
}

int32_t Screen_GetResWidth(void)
{
    return m_Resolutions[m_ResolutionIdx].width;
}

int32_t Screen_GetResHeight(void)
{
    return m_Resolutions[m_ResolutionIdx].height;
}

int32_t Screen_GetResWidthDownscaled(RENDER_SCALE_REF ref)
{
    return Screen_GetResWidth() * PHD_ONE / Screen_GetRenderScale(PHD_ONE, ref);
}

int32_t Screen_GetResHeightDownscaled(RENDER_SCALE_REF ref)
{
    return Screen_GetResHeight() * PHD_ONE
        / Screen_GetRenderScale(PHD_ONE, ref);
}

int32_t Screen_GetRenderScale(int32_t unit, RENDER_SCALE_REF ref)
{
    if (ref == RSR_TEXT) {
        return M_GetRenderScaleBase(unit, 640, 480, g_Config.ui.text_scale);
    } else if (ref == RSR_BAR) {
        return M_GetRenderScaleBase(unit, 640, 480, g_Config.ui.bar_scale);
    } else {
        return M_GetRenderScaleBase(unit, 640, 480, 0);
    }
}

int32_t Screen_GetRenderScaleGLRage(int32_t unit)
{
    // GLRage-style UI scaler
    double result = Screen_GetResWidth();
    result *= unit;
    result /= 800.0;

    // only scale up, not down
    if (result < unit) {
        result = unit;
    }

    return round(result);
}

bool Screen_CanSetPrevRes(void)
{
    return m_ResolutionIdx - 1 >= 0;
}

bool Screen_CanSetNextRes(void)
{
    return m_ResolutionIdx + 1 < m_ResolutionsCount;
}

bool Screen_SetPrevRes(void)
{
    if (m_ResolutionIdx - 1 >= 0) {
        m_ResolutionIdx--;
        M_ApplyResolution();
        return true;
    }
    return false;
}

bool Screen_SetNextRes(void)
{
    if (m_ResolutionIdx + 1 < m_ResolutionsCount) {
        m_ResolutionIdx++;
        M_ApplyResolution();
        return true;
    }
    return false;
}
