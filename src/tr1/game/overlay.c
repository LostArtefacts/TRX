#include "game/overlay.h"

#include "game/clock.h"
#include "game/creature.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/output.h"
#include "game/screen.h"
#include "game/text.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/config.h>
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

static TEXTSTRING *m_AmmoText = nullptr;
static TEXTSTRING *m_FPSText = nullptr;
static int16_t m_BarOffsetY[6] = {};
static DISPLAY_PICKUP m_Pickups[MAX_PICKUPS] = {};
static CLOCK_TIMER m_PickupsTimer = { .type = CLOCK_TIMER_SIM };
static CLOCK_TIMER m_BlinkTimer = { .type = CLOCK_TIMER_SIM };
static CLOCK_TIMER m_FPSTimer = { .type = CLOCK_TIMER_REAL };

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

static BAR_INFO m_HealthBar = {};
static BAR_INFO m_AirBar = {};
static BAR_INFO m_EnemyBar = {};

static void M_BarSetupHealth(void);
static void M_BarSetupAir(void);
static void M_BarSetupEnemy(void);
static void M_BarBlink(BAR_INFO *bar_info);
static int32_t M_BarGetPercent(BAR_INFO *bar_info);
static void M_BarGetLocation(
    BAR_INFO *bar_info, int32_t *width, int32_t *height, int32_t *x,
    int32_t *y);
static float M_Ease(float cur_frame, float max_frames);
static void M_DrawPickup3D(DISPLAY_PICKUP *pu);
static void M_DrawPickups3D(void);
static void M_DrawPickupsSprites(void);
static void M_BarDrawAir(void);
static void M_BarDrawEnemy(void);
static void M_ResetBarLocations(void);
static void M_RemoveAmmoText(void);
static void M_DrawAmmoInfo(void);
static void M_DrawPickups(void);
static double M_GetBarToTextScale(void);

static void M_BarSetupHealth(void)
{
    m_HealthBar.type = BT_LARA_HEALTH;
    m_HealthBar.value = 0;
    m_HealthBar.max_value = LARA_MAX_HITPOINTS;
    m_HealthBar.show_mode = g_Config.ui.healthbar_show_mode;
    m_HealthBar.show = false;
    m_HealthBar.blink = false;
    m_HealthBar.timer = 40;
    m_HealthBar.color = g_Config.ui.healthbar_color;
    m_HealthBar.location = g_Config.ui.healthbar_location;
}

static void M_BarSetupAir(void)
{
    m_AirBar.type = BT_LARA_MAX_AIR;
    m_AirBar.value = LARA_MAX_AIR;
    m_AirBar.max_value = LARA_MAX_AIR;
    m_AirBar.show_mode = g_Config.ui.airbar_show_mode;
    m_AirBar.show = false;
    m_AirBar.blink = false;
    m_AirBar.timer = 0;
    m_AirBar.color = g_Config.ui.airbar_color;
    m_AirBar.location = g_Config.ui.airbar_location;
}

static void M_BarSetupEnemy(void)
{
    m_EnemyBar.type = BT_ENEMY_HEALTH;
    m_EnemyBar.value = 0;
    m_EnemyBar.max_value = 0;
    m_EnemyBar.show_mode = g_Config.ui.enemy_healthbar_show_mode;
    m_EnemyBar.show = false;
    m_EnemyBar.blink = false;
    m_EnemyBar.timer = 0;
    m_EnemyBar.color = g_Config.ui.enemy_healthbar_color;
    m_EnemyBar.location = g_Config.ui.enemy_healthbar_location;
}

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

static void M_BarGetLocation(
    BAR_INFO *bar_info, int32_t *width, int32_t *height, int32_t *x, int32_t *y)
{
    const int32_t screen_margin_h = 25;
    const int32_t screen_margin_v = 18;
    const int32_t bar_spacing = 16;

    if (bar_info->location == BL_CUSTOM) {
        *width = bar_info->custom_width;
        *height = bar_info->custom_height;
        *x = bar_info->custom_x;
        *y = bar_info->custom_y;
        return;
    }

    if (bar_info->location == BL_TOP_LEFT
        || bar_info->location == BL_BOTTOM_LEFT) {
        *x = screen_margin_h;
    } else if (
        bar_info->location == BL_TOP_RIGHT
        || bar_info->location == BL_BOTTOM_RIGHT) {
        *x = Screen_GetResWidthDownscaled(RSR_BAR) - *width - screen_margin_h;
    } else {
        *x = (Screen_GetResWidthDownscaled(RSR_BAR) - *width) / 2;
    }

    if (bar_info->location == BL_TOP_LEFT || bar_info->location == BL_TOP_CENTER
        || bar_info->location == BL_TOP_RIGHT) {
        *y = screen_margin_v + m_BarOffsetY[bar_info->location];
    } else {
        *y = Screen_GetResHeightDownscaled(RSR_BAR) - *height - screen_margin_v
            - m_BarOffsetY[bar_info->location];
    }

    if (g_GameInfo.showing_demo && bar_info->location == BL_BOTTOM_CENTER) {
        *y -= M_GetBarToTextScale() * (TEXT_HEIGHT + bar_spacing);
    } else if (
        g_GameInfo.inv_ring_shown && GF_GetCurrentLevel() != nullptr
        && GF_GetCurrentLevel()->type == GFL_TITLE
        && (bar_info->location == BL_TOP_CENTER
            || bar_info->location == BL_BOTTOM_CENTER)) {
        *y = screen_margin_v + m_BarOffsetY[bar_info->location]
            + M_GetBarToTextScale() * (TEXT_HEIGHT + bar_spacing);
    }

    m_BarOffsetY[bar_info->location] += *height + bar_spacing;
}

static double M_GetBarToTextScale(void)
{
    return g_Config.ui.text_scale / g_Config.ui.bar_scale;
}

void Overlay_BarDraw(BAR_INFO *bar_info, RENDER_SCALE_REF scale_ref)
{
    const RGBA_8888 rgb_bgnd = { 0, 0, 0, 255 };
    const RGBA_8888 rgb_border = { 53, 53, 53, 255 };

    int32_t width = 200;
    int32_t height = 10;

    int32_t x = 0;
    int32_t y = 0;
    M_BarGetLocation(bar_info, &width, &height, &x, &y);

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
    Output_ApplyFOV();

    Matrix_PushUnit();
    Matrix_TranslateSet(
        src_x + (dst_x - src_x) * ease, src_y + (dst_y - src_y) * ease, scale);
    Matrix_RotX(DEG_1 * 15);
    Matrix_RotY(pu->rot_y);

    Output_SetLightDivider(0x6000);
    Output_SetLightAdder(LOW_LIGHT);
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
        Output_DrawUISprite(x, y, scale, sprite_num, 4096);
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

static void M_BarDrawAir(void)
{
    if (!g_Config.ui.enable_game_ui) {
        return;
    }
    m_AirBar.value = g_Lara.air;
    CLAMP(m_AirBar.value, 0, m_AirBar.max_value);

    m_AirBar.show = g_Lara.water_status == LWS_UNDERWATER
        || g_Lara.water_status == LWS_SURFACE;

    if (!m_AirBar.show) {
        return;
    }

    switch (m_AirBar.show_mode) {
    case BSM_DEFAULT:
        m_AirBar.show |=
            g_Lara.air <= (m_AirBar.max_value * BLINK_THRESHOLD) / 100;
        break;
    case BSM_FLASHING_ONLY:
        m_AirBar.show =
            g_Lara.air <= (m_AirBar.max_value * BLINK_THRESHOLD) / 100;
        break;
    case BSM_NEVER:
        m_AirBar.show = false;
        break;
    default:
        break;
    }
    if (!m_AirBar.show) {
        return;
    }

    Overlay_BarDraw(&m_AirBar, RSR_BAR);
}

static void M_BarDrawEnemy(void)
{
    if (!g_Config.ui.enable_game_ui) {
        return;
    }
    if (!g_Lara.target) {
        return;
    }

    switch (m_EnemyBar.show_mode) {
    case BSM_DEFAULT:
    case BSM_PS1:
    case BSM_NEVER:
        m_EnemyBar.show = false;
        break;

    case BSM_FLASHING_ONLY:
    case BSM_FLASHING_OR_DEFAULT:
    case BSM_ALWAYS:
        m_EnemyBar.show = true;
        break;

    case BSM_BOSS_ONLY:
        m_EnemyBar.show = Creature_IsBoss(Item_GetIndex(g_Lara.target));
        break;
    }

    if (!m_EnemyBar.show) {
        return;
    }

    const OBJECT *const obj = Object_Get(g_Lara.target->object_id);
    m_EnemyBar.value = g_Lara.target->hit_points;
    m_EnemyBar.max_value =
        obj->hit_points * ((g_GameInfo.bonus_flag & GBF_NGPLUS) ? 2 : 1);
    CLAMP(m_EnemyBar.value, 0, m_EnemyBar.max_value);

    Overlay_BarDraw(&m_EnemyBar, RSR_BAR);
}

static void M_ResetBarLocations(void)
{
    for (int i = 0; i < 6; i++) {
        m_BarOffsetY[i] = 0;
    }
}

static void M_RemoveAmmoText(void)
{
    if (m_AmmoText) {
        Text_Remove(m_AmmoText);
        m_AmmoText = nullptr;
    }
}

static void M_DrawAmmoInfo(void)
{
    if (!g_Config.ui.enable_game_ui) {
        M_RemoveAmmoText();
        return;
    }

    const double scale = 1.5;
    const int32_t text_height = 17 * scale;
    const int32_t text_offset_x = 3;
    const int32_t screen_margin_h = 24;
    const int32_t screen_margin_v = 18;

    double scale_ammo_to_bar = g_Config.ui.bar_scale / g_Config.ui.text_scale;

    if (g_Lara.gun_status != LGS_READY
        || (g_GameInfo.bonus_flag & GBF_NGPLUS)) {
        M_RemoveAmmoText();
        return;
    }

    char ammo_string[80] = "";
    switch (g_Lara.gun_type) {
    case LGT_PISTOLS:
        return;
    case LGT_SHOTGUN:
        sprintf(ammo_string, "%5d A", g_Lara.shotgun.ammo / SHOTGUN_AMMO_CLIP);
        break;
    case LGT_UZIS:
        sprintf(ammo_string, "%5d C", g_Lara.uzis.ammo);
        break;
    case LGT_MAGNUMS:
        sprintf(ammo_string, "%5d B", g_Lara.magnums.ammo);
        break;
    default:
        return;
    }
    Overlay_MakeAmmoString(ammo_string);

    if (m_AmmoText) {
        Text_ChangeText(m_AmmoText, ammo_string);
    } else {
        m_AmmoText = Text_Create(
            -screen_margin_h - text_offset_x, text_height + screen_margin_v,
            ammo_string);
        Text_SetScale(
            m_AmmoText, TEXT_BASE_SCALE * scale, TEXT_BASE_SCALE * scale);
        Text_AlignRight(m_AmmoText, 1);
    }

    m_AmmoText->pos.x = m_BarOffsetY[BL_TOP_RIGHT]
        ? (-screen_margin_h * scale_ammo_to_bar) - text_offset_x
        : -screen_margin_h - text_offset_x;

    m_AmmoText->pos.y = m_BarOffsetY[BL_TOP_RIGHT]
        ? text_height + (screen_margin_v * scale_ammo_to_bar)
            + (m_BarOffsetY[BL_TOP_RIGHT] * scale_ammo_to_bar)
        : text_height + screen_margin_v;

    if (m_AmmoText) {
        Text_DrawText(m_AmmoText);
    }
}

void Overlay_Init(void)
{
    for (int i = 0; i < MAX_PICKUPS; i++) {
        m_Pickups[i].phase = DPP_DEAD;
    }

    M_BarSetupHealth();
    M_BarSetupAir();
    M_BarSetupEnemy();
}

void Overlay_BarSetHealthTimer(int16_t timer)
{
    m_HealthBar.timer = timer;
}

void Overlay_BarHealthTimerTick(void)
{
    m_HealthBar.timer--;
    CLAMPL(m_HealthBar.timer, 0);
}

void Overlay_BarDrawHealth(void)
{
    if (!g_Config.ui.enable_game_ui) {
        return;
    }
    static int32_t old_hit_points = 0;

    m_HealthBar.value = g_LaraItem->hit_points;
    CLAMP(m_HealthBar.value, 0, m_HealthBar.max_value);

    if (old_hit_points != m_HealthBar.value) {
        old_hit_points = m_HealthBar.value;
        m_HealthBar.timer = 40;
    }

    m_HealthBar.show = m_HealthBar.timer > 0 || m_HealthBar.value <= 0
        || g_Lara.gun_status == LGS_READY;
    switch (m_HealthBar.show_mode) {
    case BSM_FLASHING_OR_DEFAULT:
        m_HealthBar.show |= m_HealthBar.value
            <= (m_HealthBar.max_value * BLINK_THRESHOLD) / 100;
        break;
    case BSM_FLASHING_ONLY:
        m_HealthBar.show = m_HealthBar.value
            <= (m_HealthBar.max_value * BLINK_THRESHOLD) / 100;
        break;
    case BSM_ALWAYS:
        m_HealthBar.show = true;
        break;
    case BSM_NEVER:
        m_HealthBar.show = false;
        break;
    default:
        break;
    }
    if (!m_HealthBar.show) {
        return;
    }

    Overlay_BarDraw(&m_HealthBar, RSR_BAR);
}

void Overlay_HideGameInfo(void)
{
    M_ResetBarLocations();
    M_RemoveAmmoText();
    ClockTimer_Sync(&m_PickupsTimer);
}

void Overlay_DrawGameInfo(void)
{
    Output_ClearDepthBuffer();
    M_ResetBarLocations();
    Overlay_BarDrawHealth();
    M_BarDrawAir();
    M_BarDrawEnemy();
    if (Game_IsPlaying()) {
        M_DrawPickups();
    }
    M_DrawAmmoInfo();
}

void Overlay_DrawFPSInfo(void)
{
    if (g_Config.rendering.enable_fps_counter && g_Config.ui.enable_game_ui) {
        const int32_t text_offset_x = 3;
        const int32_t text_height = 17;
        const int32_t text_inv_offset_y = 3;
        double scale_fps_to_bar =
            g_Config.ui.bar_scale / g_Config.ui.text_scale;
        int16_t x = 21;
        int16_t y = 10;

        if (ClockTimer_CheckElapsedAndTake(&m_FPSTimer, 1.0)) {
            if (m_FPSText) {
                char fps_buf[20];
                sprintf(fps_buf, "%d FPS", g_FPSCounter);
                Text_ChangeText(m_FPSText, fps_buf);
            } else {
                char fps_buf[20];
                sprintf(fps_buf, "? FPS");
                m_FPSText = Text_Create(10, 30, fps_buf);
            }
            g_FPSCounter = 0;
        }

        bool inv_health_showable = g_GameInfo.inv_ring_shown
            && g_GameInfo.inv_showing_medpack
            && m_HealthBar.location == BL_TOP_LEFT;
        bool game_bar_showable = (Game_IsPlaying())
            && !g_GameInfo.inv_ring_shown
            && (m_HealthBar.location == BL_TOP_LEFT
                || m_AirBar.location == BL_TOP_LEFT
                || m_EnemyBar.location == BL_TOP_LEFT);

        if (inv_health_showable || game_bar_showable) {
            x = (x * scale_fps_to_bar) + text_offset_x;
            y = text_height
                + scale_fps_to_bar * (y + m_BarOffsetY[BL_TOP_LEFT]);
        } else if (g_GameInfo.inv_ring_shown && g_GameInfo.inv_ring_above) {
            y += (text_height * 2) + text_inv_offset_y;
        } else {
            y += text_height;
        }

        Text_SetPos(m_FPSText, x, y);
    } else if (m_FPSText) {
        Text_Remove(m_FPSText);
        m_FPSText = nullptr;
        g_FPSCounter = 0;
    }

    if (m_FPSText) {
        Text_DrawText(m_FPSText);
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

void Overlay_MakeAmmoString(char *const string)
{
    char result[128] = "";

    char *ptr = string;
    while (*ptr != '\0') {
        if (*ptr == ' ') {
            strcat(result, " ");
        } else if (*ptr == 'A') {
            strcat(result, "\\{ammo shotgun}");
        } else if (*ptr == 'B') {
            strcat(result, "\\{ammo magnums}");
        } else if (*ptr == 'C') {
            strcat(result, "\\{ammo uzis}");
        } else if (*ptr >= '0' && *ptr <= '9') {
            strcat(result, "\\{small digit ");
            char tmp[2] = { *ptr, '\0' };
            strcat(result, tmp);
            strcat(result, "}");
        }
        ptr++;
    }

    strcpy(string, result);
}

void Overlay_DrawModeInfo(void)
{
}
