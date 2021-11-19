#include "game/screen.h"

#include "3dsystem/3d_gen.h"
#include "config.h"
#include "game/viewport.h"
#include "global/vars.h"
#include "specific/s_hwr.h"

#include <math.h>

// The screen resolution is controlled by two variables that are indices within
// an array of predefined screen resolutions.
// Actual screen resolution, sometimes different from the game resolution
// (during FMVs, main menu sequence, static pictures etc.)
static int32_t m_HiRes = 0;
// The resolution to render the game in. This is what gets saved in settings
// and the like.
static int32_t m_GameHiRes = 0;

void Screen_SetupSize()
{
    int32_t width = Screen_GetResWidth();
    int32_t height = Screen_GetResHeight();
    ViewPort_Init(width, height);

    phd_ResetMatrixStack();
    phd_AlterFOV(T1MConfig.fov_value * PHD_DEGREE);
}

bool Screen_SetGameResIdx(int32_t idx)
{
    if (idx >= 0 && idx < RESOLUTIONS_SIZE) {
        m_GameHiRes = idx;
        return true;
    }
    return false;
}

bool Screen_SetPrevGameRes()
{
    if (m_GameHiRes - 1 >= 0) {
        m_GameHiRes--;
        return true;
    }
    return false;
}

bool Screen_SetNextGameRes()
{
    if (m_GameHiRes + 1 < RESOLUTIONS_SIZE) {
        m_GameHiRes++;
        return true;
    }
    return false;
}

int32_t Screen_GetGameResIdx()
{
    return m_GameHiRes;
}

int32_t Screen_GetGameResWidth()
{
    return AvailableResolutions[m_GameHiRes].width;
}

int32_t Screen_GetGameResHeight()
{
    return AvailableResolutions[m_GameHiRes].height;
}

int32_t Screen_GetResIdx()
{
    return m_HiRes;
}

int32_t Screen_GetResWidth()
{
    return AvailableResolutions[m_HiRes].width;
}

int32_t Screen_GetResHeight()
{
    return AvailableResolutions[m_HiRes].height;
}

void Screen_SetResolution(int32_t hi_res)
{
    ModeLock = true;
    if (hi_res == m_HiRes) {
        return;
    }

    m_HiRes = hi_res;
    HWR_SwitchResolution();
}

void Screen_RestoreResolution()
{
    ModeLock = false;
    if (m_GameHiRes == m_HiRes) {
        return;
    }

    m_HiRes = m_GameHiRes;
    HWR_SwitchResolution();
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
        ? ((double)Screen_GetResWidth() * unit * T1MConfig.ui.text_scale)
            / baseWidth
        : unit * T1MConfig.ui.text_scale;
    int32_t scale_y = Screen_GetResHeight() > baseHeight
        ? ((double)Screen_GetResHeight() * unit * T1MConfig.ui.text_scale)
            / baseHeight
        : unit * T1MConfig.ui.text_scale;
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
