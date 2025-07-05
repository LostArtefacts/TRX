#include "game/viewport.h"

#include "game/output.h"
#include "game/shell.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/math.h>

#define M_INITIAL_FOV 65

static int16_t m_CurrentFOV = M_INITIAL_FOV;

void Viewport_Init(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (x < 0 || y < 0 || width < 0 || height < 0) {
        const SHELL_SIZE size = Shell_GetCurrentSize();

        struct {
            int32_t w, h;
        } ar;
        switch (g_Config.rendering.aspect_mode) {
        case AM_4_3:
            ar.w = 4;
            ar.h = 3;
            break;
        case AM_16_9:
            ar.w = 16;
            ar.h = 9;
            break;
        case AM_ANY:
            ar.w = size.w;
            ar.h = size.h;
            break;
        }

        x = 0;
        y = 0;
        width = size.w / g_Config.rendering.upscaling_factor;
        height = size.h / g_Config.rendering.upscaling_factor;
        if (g_Config.rendering.aspect_mode != AM_ANY) {
            width = height * ar.w / ar.h;
        }
    }

    VIEWPORT_RECT *const rect = &g_Viewport_Rects[VIEWPORT_GAME];
    rect->x = x;
    rect->y = y;
    rect->width = width;
    rect->height = height;

    g_Viewport_Rects[VIEWPORT_UI] = g_Viewport_Rects[VIEWPORT_GAME];

    g_PhdLeft = Viewport_GetMinX(VIEWPORT_GAME);
    g_PhdTop = Viewport_GetMinY(VIEWPORT_GAME);
    g_PhdRight = Viewport_GetMaxX(VIEWPORT_GAME);
    g_PhdBottom = Viewport_GetMaxY(VIEWPORT_GAME);
}

int16_t Viewport_GetSystemFOV(void)
{
    return m_CurrentFOV;
}

int16_t Viewport_GetUserFOV(void)
{
    return g_Config.visuals.fov_value * DEG_1;
}

void Viewport_AlterFOV(int16_t fov)
{
    m_CurrentFOV = fov;
    Output_ObserveFOVChange();
}

void Viewport_Reset(void)
{
    Viewport_ResetCommon();
    Viewport_Init(-1, -1, -1, -1);
    GFX_Context_SetWindowSize(
        g_Viewport_Rects[VIEWPORT_WINDOW].width,
        g_Viewport_Rects[VIEWPORT_WINDOW].height);
    GFX_Context_SetDisplaySize(
        g_Viewport_Rects[VIEWPORT_GAME].width,
        g_Viewport_Rects[VIEWPORT_GAME].height);
    Viewport_Debug();
}
