#include "game/overlay.h"

#include "game/clock.h"
#include "game/creature.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/output.h"
#include "game/output/meshes/common.h"
#include "game/output/shader.h"
#include "game/screen.h"
#include "game/text.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/gun/const.h>
#include <libtrx/game/matrix.h>
#include <libtrx/utils.h>

#include <stdio.h>
#include <string.h>

#define COLOR_STEPS 5
#define MAX_PICKUP_COLUMNS 4
#define MAX_PICKUP_DURATION_DISPLAY 2.0 // seconds
#define MAX_PICKUP_DURATION_EASE_IN 0.5 // seconds
#define MAX_PICKUP_DURATION_EASE_OUT 1.0 // seconds
#define MAX_PICKUPS 16
#define BLINK_THRESHOLD 20

typedef enum {
    DPP_EASE_IN,
    DPP_DISPLAY,
    DPP_EASE_OUT,
    DPP_DEAD,
} DISPLAY_PICKUP_PHASE;

typedef struct {
    GAME_OBJECT_ID object_id;
    double elapsed;
    int32_t grid_x;
    int32_t grid_y;
    int32_t rot_y;
    DISPLAY_PICKUP_PHASE phase;
} DISPLAY_PICKUP;

static DISPLAY_PICKUP m_Pickups[MAX_PICKUPS] = {};
static CLOCK_TIMER m_PickupsTimer = { .type = CLOCK_TIMER_SIM };
static CLOCK_TIMER m_BlinkTimer = { .type = CLOCK_TIMER_SIM };

static RGBA_8888 m_ColorBarMap[][COLOR_STEPS] = {
    // gold
    { { 124, 94, 37, 255 },
      { 161, 131, 60, 255 },
      { 124, 94, 37, 255 },
      { 100, 70, 19, 255 },
      { 76, 46, 2, 255 } },
    // blue
    { { 61, 113, 123, 255 },
      { 101, 146, 154, 255 },
      { 61, 113, 123, 255 },
      { 31, 93, 107, 255 },
      { 0, 74, 91, 255 } },
    // grey
    { { 88, 100, 88, 255 },
      { 116, 132, 116, 255 },
      { 88, 100, 88, 255 },
      { 76, 80, 76, 255 },
      { 48, 48, 48, 255 } },
    // red
    { { 160, 40, 28, 255 },
      { 184, 44, 32, 255 },
      { 160, 40, 28, 255 },
      { 124, 32, 32, 255 },
      { 84, 20, 32, 255 } },
    // silver
    { { 150, 150, 150, 255 },
      { 230, 230, 230, 255 },
      { 200, 200, 200, 255 },
      { 140, 140, 140, 255 },
      { 100, 100, 100, 255 } },
    // green
    { { 100, 190, 20, 255 },
      { 130, 230, 30, 255 },
      { 100, 190, 20, 255 },
      { 90, 150, 15, 255 },
      { 80, 110, 10, 255 } },
    // gold2
    { { 220, 170, 0, 255 },
      { 255, 200, 0, 255 },
      { 220, 170, 0, 255 },
      { 185, 140, 0, 255 },
      { 150, 100, 0, 255 } },
    // blue2
    { { 0, 170, 220, 255 },
      { 0, 200, 255, 255 },
      { 0, 170, 220, 255 },
      { 0, 140, 185, 255 },
      { 0, 100, 150, 255 } },
    // pink
    { { 220, 140, 170, 255 },
      { 255, 150, 200, 255 },
      { 210, 130, 160, 255 },
      { 165, 100, 120, 255 },
      { 120, 60, 70, 255 } },
    // purple
    { { 52, 22, 80, 255 },
      { 70, 30, 107, 255 },
      { 52, 22, 80, 255 },
      { 39, 17, 60, 255 },
      { 26, 11, 40, 255 } },
};

static void M_BarBlink(BAR_INFO *bar_info);
static int32_t M_BarGetPercent(BAR_INFO *bar_info);
static float M_Ease(float cur_frame, float max_frames);
static void M_DrawPickup3D(DISPLAY_PICKUP *pu);
static void M_DrawPickups3D(void);
static void M_DrawPickupsSprites(void);
static void M_DrawPickups(void);

static int32_t M_BarGetPercent(BAR_INFO *bar_info)
{
    return bar_info->value * 100 / bar_info->max_value;
}

static void M_BarBlink(BAR_INFO *bar_info)
{
    if (bar_info->show_mode == BSM_PS1 || bar_info->type == BT_ENEMY_HEALTH
        || bar_info->type == BT_PROGRESS) {
        bar_info->blink = false;
        return;
    }

    const int32_t percent = M_BarGetPercent(bar_info);
    if (percent > BLINK_THRESHOLD) {
        bar_info->blink = false;
        return;
    }

    if (ClockTimer_CheckElapsedAndTake(
            &m_BlinkTimer, 10.0 / (double)LOGIC_FPS)) {
        bar_info->blink = !bar_info->blink;
    }
}

void Overlay_BarDraw(BAR_INFO *bar_info, RENDER_SCALE_REF scale_ref)
{
    const RGBA_8888 rgb_bgnd = { 0, 0, 0, 255 };
    const RGBA_8888 rgb_border = { 53, 53, 53, 255 };
    ASSERT(bar_info->location == BL_CUSTOM);
    int32_t width = bar_info->custom_width;
    int32_t height = bar_info->custom_height;
    int32_t x = bar_info->custom_x;
    int32_t y = bar_info->custom_y;

    int32_t padding = Screen_GetRenderScale(2, scale_ref);
    int32_t border = Screen_GetRenderScale(2, scale_ref);

    int32_t sx = Screen_GetRenderScale(x, scale_ref) - padding;
    int32_t sy = Screen_GetRenderScale(y, scale_ref) - padding;
    int32_t sw = Screen_GetRenderScale(width, scale_ref) + padding * 2;
    int32_t sh = Screen_GetRenderScale(height, scale_ref) + padding * 2;

    // border
    Output_DrawScreenFlatQuad(
        sx - border, sy - border, sw + 2 * border, sh + 2 * border, rgb_border);

    // background
    Output_DrawScreenFlatQuad(sx, sy, sw, sh, rgb_bgnd);

    int32_t percent = M_BarGetPercent(bar_info);

    // Check if bar should flash or not
    M_BarBlink(bar_info);

    if (percent && !bar_info->blink) {
        width = width * percent / 100;

        sx = Screen_GetRenderScale(x, scale_ref);
        sy = Screen_GetRenderScale(y, scale_ref);
        sw = Screen_GetRenderScale(width, scale_ref);
        sh = Screen_GetRenderScale(height, scale_ref);

        if (g_Config.ui.enable_smooth_bars) {
            for (int i = 0; i < COLOR_STEPS - 1; i++) {
                RGBA_8888 c1 = m_ColorBarMap[bar_info->color][i];
                RGBA_8888 c2 = m_ColorBarMap[bar_info->color][i + 1];
                int32_t lsy = sy + i * sh / (COLOR_STEPS - 1);
                int32_t lsh = sy + (i + 1) * sh / (COLOR_STEPS - 1) - lsy;
                Output_DrawScreenGradientQuad(sx, lsy, sw, lsh, c1, c1, c2, c2);
            }
        } else {
            for (int i = 0; i < COLOR_STEPS; i++) {
                RGBA_8888 color = m_ColorBarMap[bar_info->color][i];
                int32_t lsy = sy + i * sh / COLOR_STEPS;
                int32_t lsh = sy + (i + 1) * sh / COLOR_STEPS - lsy;
                Output_DrawScreenFlatQuad(sx, lsy, sw, lsh, color);
            }
        }
    }
}

static float M_Ease(const float cur_frame, const float max_frames)
{
    const float ratio = cur_frame / max_frames;
    if (ratio < 0.5f) {
        return 2.0f * ratio * ratio;
    }
    const float new_ratio = ratio - 1.0f;
    return 1.0f - 2.0f * new_ratio * new_ratio;
}

static void M_DrawPickup3D(DISPLAY_PICKUP *pu)
{
    if (!g_Config.ui.enable_game_ui) {
        return;
    }

    int32_t screen_width = Screen_GetResWidth();
    int32_t screen_height = Screen_GetResHeight();
    int32_t pickup_width = screen_width / 8;
    int32_t pickup_height = screen_height / 8;
    int32_t padding_x = ((screen_width + screen_height) / 2) / 8;
    int32_t padding_y = padding_x * 3 / 4;
    int32_t scale = 768;

    float aspect_ratio = (float)screen_height / screen_width;

    int32_t vp_x1 = screen_width / 2 - padding_x - pu->grid_x * pickup_width;
    int32_t vp_x2 = vp_x1 + screen_width;
    int32_t vp_y1 = screen_height / 2 - padding_y - pu->grid_y * pickup_height;
    int32_t vp_y2 = vp_y1 + screen_height;

    int32_t dst_x = 0;
    int32_t dst_y = 0;
    int32_t src_x = padding_x + (pu->grid_x + 1.0f) * pickup_width;
    int32_t src_y = 0;

    float ease = 1.0f;
    switch (pu->phase) {
    case DPP_EASE_IN:
        ease = M_Ease(pu->elapsed, MAX_PICKUP_DURATION_EASE_IN);
        break;
    case DPP_EASE_OUT:
        ease = M_Ease(
            MAX_PICKUP_DURATION_EASE_OUT - pu->elapsed,
            MAX_PICKUP_DURATION_EASE_OUT);
        break;
    case DPP_DISPLAY:
        ease = 1.0f;
        break;
    case DPP_DEAD:
        return;
    }

    // Reset the FOV and the W2V matrix in case they get changed by a cinematic
    // camera (when picking up the Scion). Move the viewport rather than
    // translating the object in order to avoid perspective distortion in the
    // screen corners.
    int16_t old_fov = Viewport_GetFOV();
    Viewport_SetFOV(PICKUPS_FOV * DEG_1);
    Viewport_Init(vp_x1, vp_y1, vp_x2 - vp_x1, vp_y2 - vp_y1);
    glViewport(vp_x1, -vp_y1, screen_width, screen_height);
    Output_ApplyFOV();

    Matrix_PushUnit();
    Matrix_TranslateSet(
        src_x + (dst_x - src_x) * ease, src_y + (dst_y - src_y) * ease, scale);
    Matrix_RotX(DEG_1 * 15);
    Matrix_RotY(pu->rot_y);

    Output_SetLightDivider(0x6000);
    Output_SetLightAdder(SHADE_LOW);
    Output_RotateLight(0, 0);

    const OBJECT *const obj = Object_Get(Inv_GetItemOption(pu->object_id));
    const ANIM_FRAME *const frame = Object_GetAnim(obj, 0)->frame_ptr;

    Matrix_Push();
    Matrix_TranslateRel16(frame->offset);
    Matrix_TranslateRel32((XYZ_32) {
        .x = -(frame->bounds.min.x + frame->bounds.max.x) / 2,
        .y = -(frame->bounds.min.y + frame->bounds.max.y) / 2,
        .z = -(frame->bounds.min.z + frame->bounds.max.z) / 2,
    });
    Matrix_Rot16(frame->mesh_rots[0]);

    Object_DrawMesh(obj->mesh_idx, 0, false);

    for (int i = 1; i < obj->mesh_count; i++) {
        const ANIM_BONE *const bone = Object_GetBone(obj, i - 1);
        if (bone->matrix_pop) {
            Matrix_Pop();
        }

        if (bone->matrix_push) {
            Matrix_Push();
        }

        Matrix_TranslateRel32(bone->pos);
        Matrix_Rot16(frame->mesh_rots[i]);

        Object_DrawMesh(obj->mesh_idx + i, 0, false);
    }
    Matrix_Pop();

    Matrix_Pop();
    Viewport_Init(0, 0, Screen_GetResWidth(), Screen_GetResHeight());
    Viewport_SetFOV(old_fov);
    glViewport(0, 0, Screen_GetResWidth(), Screen_GetResHeight());
}

static void M_DrawPickups3D(void)
{
    const double elapsed = ClockTimer_TakeElapsed(&m_PickupsTimer);

    for (int i = 0; i < MAX_PICKUPS; i++) {
        DISPLAY_PICKUP *const pu = &m_Pickups[i];

        switch (pu->phase) {
        case DPP_DEAD:
            continue;

        case DPP_EASE_IN:
            pu->elapsed += elapsed;
            if (pu->elapsed >= MAX_PICKUP_DURATION_EASE_IN) {
                pu->phase = DPP_DISPLAY;
                pu->elapsed = 0.0;
            }
            break;

        case DPP_DISPLAY:
            pu->elapsed += elapsed;
            if (pu->elapsed >= MAX_PICKUP_DURATION_DISPLAY) {
                pu->phase = DPP_EASE_OUT;
                pu->elapsed = 0.0;
            }
            break;

        case DPP_EASE_OUT:
            pu->elapsed += elapsed;
            if (pu->elapsed >= MAX_PICKUP_DURATION_EASE_OUT) {
                pu->phase = DPP_DEAD;
                pu->elapsed = 0.0;
            }
            break;
        }

        pu->rot_y += 4 * DEG_1 * (elapsed * LOGIC_FPS);

        M_DrawPickup3D(pu);
    }
}

static void M_DrawPickupsSprites(void)
{
    const double elapsed = ClockTimer_TakeElapsed(&m_PickupsTimer);

    const int32_t sprite_height =
        MIN(Viewport_GetWidth(), Viewport_GetHeight() * 320 / 200) / 10;
    const int32_t sprite_width = sprite_height * 4 / 3;

    for (int i = 0; i < MAX_PICKUPS; i++) {
        DISPLAY_PICKUP *const pu = &m_Pickups[i];
        if (pu->phase == DPP_DEAD) {
            continue;
        }

        pu->elapsed += elapsed;
        if (pu->elapsed >= MAX_PICKUP_DURATION_DISPLAY) {
            pu->phase = DPP_DEAD;
            continue;
        }

        if (!g_Config.ui.enable_game_ui) {
            return;
        }

        const int32_t x =
            Viewport_GetWidth() - sprite_height - sprite_width * pu->grid_x;
        const int32_t y =
            Viewport_GetHeight() - sprite_height - sprite_height * pu->grid_y;
        const int32_t scale = Screen_GetRenderScaleGLRage(12288);
        const int16_t sprite_num = Object_Get(pu->object_id)->mesh_idx;
        Output_DrawUISprite(x, y, scale, sprite_num, SHADE_NEUTRAL);
    }
}

static void M_DrawPickups(void)
{
    if (g_Config.visuals.enable_3d_pickups) {
        M_DrawPickups3D();
    } else {
        M_DrawPickupsSprites();
    }
}

void Overlay_Reset(void)
{
    for (int i = 0; i < MAX_PICKUPS; i++) {
        m_Pickups[i].phase = DPP_DEAD;
    }
}

void Overlay_HideGameInfo(void)
{
    ClockTimer_Sync(&m_PickupsTimer);
}

void Overlay_DrawGameInfo(void)
{
    Output_ClearDepthBuffer();
    if (Game_IsPlaying()) {
        M_DrawPickups();
    }
}

void Overlay_AddPickup(const GAME_OBJECT_ID obj_id)
{
    int32_t grid_x = -1;
    int32_t grid_y = -1;
    for (int i = 0; i < MAX_PICKUPS; i++) {
        int x = i % MAX_PICKUP_COLUMNS;
        int y = i / MAX_PICKUP_COLUMNS;
        bool is_occupied = false;
        for (int j = 0; j < MAX_PICKUPS; j++) {
            bool is_dead_or_dying = m_Pickups[j].phase == DPP_DEAD
                || m_Pickups[j].phase == DPP_EASE_OUT;
            if (m_Pickups[j].grid_x == x && m_Pickups[j].grid_y == y
                && !is_dead_or_dying) {
                is_occupied = true;
                break;
            }
        }

        if (!is_occupied) {
            grid_x = x;
            grid_y = y;
            break;
        }
    }

    for (int i = 0; i < MAX_PICKUPS; i++) {
        if (m_Pickups[i].phase == DPP_DEAD) {
            m_Pickups[i].object_id = obj_id;
            m_Pickups[i].elapsed = 0.0;
            m_Pickups[i].grid_x = grid_x;
            m_Pickups[i].grid_y = grid_y;
            m_Pickups[i].rot_y = 0;
            m_Pickups[i].phase =
                g_Config.visuals.enable_3d_pickups ? DPP_EASE_IN : DPP_DISPLAY;
            return;
        }
    }
}

void Overlay_DrawModeInfo(void)
{
}
