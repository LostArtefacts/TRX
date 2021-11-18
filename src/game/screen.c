#include "game/screen.h"

#include "3dsystem/3d_gen.h"
#include "global/vars.h"
#include "specific/s_hwr.h"

// The screen resolution is controlled by two variables that are indices within
// an array of predefined screen resolutions.
// Actual screen resolution, sometimes different from the game resolution
// (during FMVs, main menu sequence, static pictures etc.)
static int32_t m_HiRes = 0;
// The resolution to render the game in. This is what gets saved in settings
// and the like.
static int32_t m_GameHiRes = 0;

bool SetGameScreenSizeIdx(int32_t idx)
{
    if (idx >= 0 && idx < RESOLUTIONS_SIZE) {
        m_GameHiRes = idx;
        return true;
    }
    return false;
}

bool SetPrevGameScreenSize()
{
    if (m_GameHiRes - 1 >= 0) {
        m_GameHiRes--;
        return true;
    }
    return false;
}

bool SetNextGameScreenSize()
{
    if (m_GameHiRes + 1 < RESOLUTIONS_SIZE) {
        m_GameHiRes++;
        return true;
    }
    return false;
}

int32_t GetGameScreenSizeIdx()
{
    return m_GameHiRes;
}

int32_t GetGameScreenWidth()
{
    return AvailableResolutions[m_GameHiRes].width;
}

int32_t GetGameScreenHeight()
{
    return AvailableResolutions[m_GameHiRes].height;
}

int32_t GetScreenSizeIdx()
{
    return m_HiRes;
}

int32_t GetScreenWidth()
{
    return AvailableResolutions[m_HiRes].width;
}

int32_t GetScreenHeight()
{
    return AvailableResolutions[m_HiRes].height;
}

void SetupScreenSize()
{
    int32_t width = GetScreenWidth();
    int32_t height = GetScreenHeight();
    int32_t x = (width - width) / 2;
    int32_t y = (height - height) / 2;
    phd_InitWindow(x, y, width, height);
}

void TempVideoAdjust(int32_t hi_res)
{
    ModeLock = true;
    if (hi_res == m_HiRes) {
        return;
    }

    m_HiRes = hi_res;
    HWR_SwitchResolution();
}

void TempVideoRemove()
{
    ModeLock = false;
    if (m_GameHiRes == m_HiRes) {
        return;
    }

    m_HiRes = m_GameHiRes;
    HWR_SwitchResolution();
}
