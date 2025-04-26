#pragma once

#include <libtrx/game/viewport.h>

typedef struct {
    int32_t width;
    int32_t height;
    int32_t near_z;
    int32_t far_z;
    int16_t view_angle;
    struct {
        int32_t w;
        int32_t h;
    } render_ar;

    // TODO: remove most of these variables if possible
    struct {
        int32_t win_max_x;
        int32_t win_max_y;
        int32_t win_width;
        int32_t win_height;
        int32_t win_center_x;
        int32_t win_center_y;

        int32_t win_left;
        int32_t win_top;
        int32_t win_right;
        int32_t win_bottom;

        int32_t persp;
        int32_t near_z;
        int32_t far_z;

        float flt_near_z;
        float flt_far_z;
        float flt_res_z;
        float flt_res_z_o_rhw;
        float flt_res_z_buf;
        float flt_rhw_o_persp;
        float flt_rhw_o_near_z;
        float flt_persp;
        float flt_persp_o_near_z;
        float flt_win_left;
        float flt_win_top;
        float flt_win_right;
        float flt_win_bottom;
        float flt_win_center_x;
        float flt_win_center_y;
        int32_t view_distance;
    } game_vars;
} VIEWPORT;

void Viewport_Reset(void);

void Viewport_Restore(const VIEWPORT *ref_vp);

int16_t Viewport_GetFOV(bool resolve_user_fov);
void Viewport_AlterFOV(int16_t view_angle);

const VIEWPORT *Viewport_Get(void);
