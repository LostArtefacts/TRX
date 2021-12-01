#include "game/screen.h"

#include "3dsystem/3d_gen.h"
#include "3dsystem/matrix.h"
#include "config.h"
#include "game/viewport.h"
#include "global/vars.h"
#include "specific/s_hwr.h"

#include <math.h>

static int32_t m_ResolutionIdx = 0;
static int32_t m_PendingResolutionIdx = 0;

void Screen_SetupSize()
{
    int32_t width = Screen_GetResWidth();
    int32_t height = Screen_GetResHeight();
    ViewPort_Init(width, height);

    phd_ResetMatrixStack();
    phd_AlterFOV(g_Config.fov_value * PHD_DEGREE);
}

bool Screen_SetResIdx(int32_t idx)
{
    if (idx >= 0 && idx < RESOLUTIONS_SIZE) {
        m_PendingResolutionIdx = idx;
        return true;
    }
    return false;
}

bool Screen_SetPrevRes()
{
    if (m_PendingResolutionIdx - 1 >= 0) {
        m_PendingResolutionIdx--;
        return true;
    }
    return false;
}

bool Screen_SetNextRes()
{
    if (m_PendingResolutionIdx + 1 < RESOLUTIONS_SIZE) {
        m_PendingResolutionIdx++;
        return true;
    }
    return false;
}

int32_t Screen_GetResIdx()
{
    return m_ResolutionIdx;
}

int32_t Screen_GetResWidth()
{
    return g_AvailableResolutions[m_ResolutionIdx].width;
}

int32_t Screen_GetResHeight()
{
    return g_AvailableResolutions[m_ResolutionIdx].height;
}

int32_t Screen_GetPendingResIdx()
{
    return m_PendingResolutionIdx;
}

int32_t Screen_GetPendingResWidth()
{
    return g_AvailableResolutions[m_PendingResolutionIdx].width;
}

int32_t Screen_GetPendingResHeight()
{
    return g_AvailableResolutions[m_PendingResolutionIdx].height;
}

int32_t Screen_GetResWidthDownscaled()
{
    return Screen_GetResWidth() * PHD_ONE / Screen_GetRenderScale(PHD_ONE);
}

int32_t Screen_GetResHeightDownscaled()
{
    return Screen_GetResHeight() * PHD_ONE / Screen_GetRenderScale(PHD_ONE);
}

int32_t Screen_GetRenderScale(int32_t unit)
{
    int32_t baseWidth = 640;
    int32_t baseHeight = 480;
    int32_t scale_x = Screen_GetResWidth() > baseWidth
        ? ((double)Screen_GetResWidth() * unit * g_Config.ui.text_scale)
            / baseWidth
        : unit * g_Config.ui.text_scale;
    int32_t scale_y = Screen_GetResHeight() > baseHeight
        ? ((double)Screen_GetResHeight() * unit * g_Config.ui.text_scale)
            / baseHeight
        : unit * g_Config.ui.text_scale;
    return MIN(scale_x, scale_y);
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

void Screen_ApplyResolution()
{
    m_ResolutionIdx = m_PendingResolutionIdx;
    HWR_SwitchResolution();
}
