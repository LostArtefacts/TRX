#include "game/viewport.h"

#include "game/shell.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/math.h>
#include <libtrx/game/output.h>

#define M_INITIAL_FOV 65

static int16_t m_CurrentFOV = M_INITIAL_FOV;

void Viewport_Init(int32_t x, int32_t y, int32_t width, int32_t height)
{
    const VIEWPORT_RECT *const target = &g_Viewport_Rects[VIEWPORT_TARGET];
    VIEWPORT_RECT *const game = &g_Viewport_Rects[VIEWPORT_GAME];
    VIEWPORT_RECT *const ui = &g_Viewport_Rects[VIEWPORT_UI];

    if (x < 0 || y < 0 || width < 0 || height < 0) {
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
        case AM_16_10:
            ar.w = 16;
            ar.h = 10;
            break;
        case AM_ANY:
            ar.w = target->width;
            ar.h = target->height;
            break;
        }

        x = 0;
        y = 0;
        width = target->width;
        height = target->height;
        if (g_Config.rendering.aspect_mode != AM_ANY) {
            width = height * ar.w / ar.h;
        }
    }

    ui->x = x;
    ui->y = y;
    ui->width = width;
    ui->height = height;

    game->x = x;
    game->y = y;
    game->width = width / g_Config.rendering.upscaling_factor;
    game->height = height / g_Config.rendering.upscaling_factor;

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
    return g_Config.visuals.fov * DEG_1;
}

void Viewport_AlterFOV(int16_t fov)
{
    m_CurrentFOV = fov;
}

void Viewport_Reset(void)
{
    Viewport_ResetCommon();
    Viewport_Init(-1, -1, -1, -1);
    Viewport_Debug();
}
