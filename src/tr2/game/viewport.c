#include "game/viewport.h"

#include "game/output.h"
#include "game/render/common.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/math.h>
#include <libtrx/game/shell.h>

#define MAP_GAME_VARS()                                                        \
    MAP_GAME_VAR(win_max_x, g_PhdWinMaxX);                                     \
    MAP_GAME_VAR(win_max_y, g_PhdWinMaxY);                                     \
    MAP_GAME_VAR(win_width, g_PhdWinWidth);                                    \
    MAP_GAME_VAR(win_height, g_PhdWinHeight);                                  \
    MAP_GAME_VAR(win_center_x, g_PhdWinCenterX);                               \
    MAP_GAME_VAR(win_center_y, g_PhdWinCenterY);                               \
    MAP_GAME_VAR(win_left, g_PhdWinLeft);                                      \
    MAP_GAME_VAR(win_top, g_PhdWinTop);                                        \
    MAP_GAME_VAR(win_right, g_PhdWinRight);                                    \
    MAP_GAME_VAR(win_bottom, g_PhdWinBottom);                                  \
    MAP_GAME_VAR(near_z, g_PhdNearZ);                                          \
    MAP_GAME_VAR(far_z, g_PhdFarZ);                                            \
    MAP_GAME_VAR(flt_near_z, g_FltNearZ);                                      \
    MAP_GAME_VAR(flt_far_z, g_FltFarZ);                                        \
    MAP_GAME_VAR(persp, g_PhdPersp);                                           \
    MAP_GAME_VAR(flt_res_z, g_FltResZ);                                        \
    MAP_GAME_VAR(flt_res_z_o_rhw, g_FltResZORhw);                              \
    MAP_GAME_VAR(flt_res_z_buf, g_FltResZBuf);                                 \
    MAP_GAME_VAR(flt_rhw_o_near_z, g_FltRhwONearZ);                            \
    MAP_GAME_VAR(flt_rhw_o_persp, g_FltRhwOPersp);                             \
    MAP_GAME_VAR(flt_persp, g_FltPersp);                                       \
    MAP_GAME_VAR(flt_persp_o_near_z, g_FltPerspONearZ);                        \
    MAP_GAME_VAR(view_distance, g_PhdViewDistance);                            \
    MAP_GAME_VAR(flt_win_left, g_FltWinLeft);                                  \
    MAP_GAME_VAR(flt_win_top, g_FltWinTop);                                    \
    MAP_GAME_VAR(flt_win_right, g_FltWinRight);                                \
    MAP_GAME_VAR(flt_win_bottom, g_FltWinBottom);                              \
    MAP_GAME_VAR(flt_win_center_x, g_FltWinCenterX);                           \
    MAP_GAME_VAR(flt_win_center_y, g_FltWinCenterY);

static VIEWPORT m_Viewport = {};

static void M_AlterFov(VIEWPORT *vp);
static void M_InitGameVars(VIEWPORT *vp);
static void M_PullGameVars(VIEWPORT *vp);
static void M_ApplyGameVars(const VIEWPORT *vp);

static void M_AlterFov(VIEWPORT *const vp)
{
    const int32_t view_angle =
        vp->view_angle <= 0 ? g_Config.visuals.fov * DEG_1 : vp->view_angle;
    const int32_t fov_width = vp->game_vars.win_height * 320
        / (g_Config.visuals.use_psx_fov ? 200 : 240);
    vp->game_vars.persp =
        fov_width / 2 * Math_Cos(view_angle / 2) / Math_Sin(view_angle / 2);

    vp->game_vars.flt_persp = vp->game_vars.persp;
    vp->game_vars.flt_rhw_o_persp = g_RhwFactor / vp->game_vars.flt_persp;
    vp->game_vars.flt_persp_o_near_z =
        vp->game_vars.flt_persp / vp->game_vars.flt_near_z;
}

static void M_InitGameVars(VIEWPORT *const vp)
{
    vp->game_vars.win_max_x = vp->width - 1;
    vp->game_vars.win_max_y = vp->height - 1;
    vp->game_vars.win_width = vp->width;
    vp->game_vars.win_height = vp->height;
    vp->game_vars.win_center_x = vp->width / 2;
    vp->game_vars.win_center_y = vp->height / 2;

    vp->game_vars.win_left = 0;
    vp->game_vars.win_top = 0;
    vp->game_vars.win_right = vp->game_vars.win_max_x;
    vp->game_vars.win_bottom = vp->game_vars.win_max_y;

    vp->game_vars.flt_win_left = vp->game_vars.win_left;
    vp->game_vars.flt_win_top = vp->game_vars.win_top;
    vp->game_vars.flt_win_right = vp->game_vars.win_right + 1;
    vp->game_vars.flt_win_bottom = vp->game_vars.win_bottom + 1;
    vp->game_vars.flt_win_center_x = vp->game_vars.win_center_x;
    vp->game_vars.flt_win_center_y = vp->game_vars.win_center_y;

    vp->game_vars.near_z = vp->near_z << W2V_SHIFT;
    vp->game_vars.far_z = vp->far_z << W2V_SHIFT;

    vp->game_vars.flt_near_z = vp->game_vars.near_z;
    vp->game_vars.flt_far_z = vp->game_vars.far_z;

    const double res_z = 0.99 * vp->game_vars.flt_near_z
        * vp->game_vars.flt_far_z
        / (vp->game_vars.flt_far_z - vp->game_vars.flt_near_z);
    vp->game_vars.flt_res_z = res_z;
    vp->game_vars.flt_res_z_o_rhw = res_z / g_RhwFactor;
    vp->game_vars.flt_res_z_buf = 0.005 + res_z / vp->game_vars.flt_near_z;
    vp->game_vars.flt_rhw_o_near_z = g_RhwFactor / vp->game_vars.flt_near_z;
    vp->game_vars.flt_persp = vp->game_vars.persp;
    vp->game_vars.flt_persp_o_near_z =
        vp->game_vars.flt_persp / vp->game_vars.flt_near_z;
    vp->game_vars.view_distance = vp->far_z;

    M_AlterFov(vp);
}

static void M_PullGameVars(VIEWPORT *const vp)
{
#undef MAP_GAME_VAR
#define MAP_GAME_VAR(a, b) vp->game_vars.a = b;
    MAP_GAME_VARS();
}

static void M_ApplyGameVars(const VIEWPORT *const vp)
{
#undef MAP_GAME_VAR
#define MAP_GAME_VAR(a, b) b = vp->game_vars.a;
    MAP_GAME_VARS();
}

void Viewport_Reset(void)
{
    Viewport_ResetCommon();

    const VIEWPORT_RECT *const target = &g_Viewport_Rects[VIEWPORT_TARGET];
    VIEWPORT_RECT *const game = &g_Viewport_Rects[VIEWPORT_GAME];
    VIEWPORT_RECT *const ui = &g_Viewport_Rects[VIEWPORT_UI];

    VIEWPORT *const vp = &m_Viewport;
    switch (g_Config.rendering.aspect_mode) {
    case AM_4_3:
        vp->render_ar.w = 4;
        vp->render_ar.h = 3;
        break;
    case AM_16_9:
        vp->render_ar.w = 16;
        vp->render_ar.h = 9;
        break;
    case AM_ANY:
        vp->render_ar.w = target->width;
        vp->render_ar.h = target->height;
        break;
    }

    ui->x = 0;
    ui->y = 0;
    ui->width = target->width;
    ui->height = target->height;
    if (g_Config.rendering.aspect_mode != AM_ANY) {
        ui->width = ui->height * vp->render_ar.w / vp->render_ar.h;
    }

    game->x = ui->x;
    game->y = ui->y;
    game->width = ui->width / g_Config.rendering.upscaling_factor;
    game->height = ui->height / g_Config.rendering.upscaling_factor;

    if (g_Config.rendering.render_mode == RM_SOFTWARE) {
        g_Viewport_Rects[VIEWPORT_UI] = g_Viewport_Rects[VIEWPORT_GAME];
    }

    vp->width = game->width;
    vp->height = game->height;
    vp->near_z = Output_GetNearZ() >> W2V_SHIFT;
    vp->far_z = Output_GetFarZ() >> W2V_SHIFT;

    // We do not update vp->view_angle on purpose, as it's managed by the game
    // rather than the window manager. (Think cutscenes, special cameras, etc.)

    switch (g_Config.rendering.render_mode) {
    case RM_SOFTWARE:
        g_PerspectiveDistance = g_Config.rendering.enable_perspective_filter
            ? SW_DETAIL_HIGH
            : SW_DETAIL_MEDIUM;
        break;

    case RM_HARDWARE:
        break;

    default:
        Shell_ExitSystem("unknown render mode");
    }

    M_InitGameVars(&m_Viewport);
    M_ApplyGameVars(&m_Viewport);

    Render_SetupDisplay(target->width, target->height, vp->width, vp->height);
    Viewport_Debug();
}

const VIEWPORT *Viewport_Get(void)
{
    M_PullGameVars(&m_Viewport);
    return &m_Viewport;
}

void Viewport_Restore(const VIEWPORT *ref_vp)
{
    memcpy(&m_Viewport, ref_vp, sizeof(VIEWPORT));
    M_ApplyGameVars(&m_Viewport);
}

int16_t Viewport_GetSystemFOV(void)
{
    return m_Viewport.view_angle;
}

int16_t Viewport_GetUserFOV(void)
{
    return g_Config.visuals.fov * DEG_1;
}

void Viewport_AlterFOV(const int16_t view_angle)
{
    m_Viewport.view_angle = view_angle;
    M_PullGameVars(&m_Viewport);
    M_AlterFov(&m_Viewport);
    M_ApplyGameVars(&m_Viewport);
}
