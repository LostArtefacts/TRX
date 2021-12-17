#include "game/overlay.h"

#include "config.h"
#include "game/clock.h"
#include "game/output.h"
#include "game/screen.h"
#include "game/text.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <stdio.h>

#include "log.h"

#define COLOR_STEPS 5
#define MAX_PICKUP_COLUMNS 4
#define MAX_PICKUPS 16

static TEXTSTRING *m_AmmoText = NULL;
static TEXTSTRING *m_FPSText = NULL;
static int16_t m_BarOffsetY[6] = { 0 };
static DISPLAYPU m_Pickups[MAX_PICKUPS] = { 0 };

static RGB888 m_ColorBarMap[][COLOR_STEPS] = {
    // gold
    { { 112, 92, 44 },
      { 164, 120, 72 },
      { 112, 92, 44 },
      { 88, 68, 0 },
      { 80, 48, 20 } },
    // blue
    { { 100, 116, 100 },
      { 92, 160, 156 },
      { 100, 116, 100 },
      { 76, 80, 76 },
      { 48, 48, 48 } },
    // grey
    { { 88, 100, 88 },
      { 116, 132, 116 },
      { 88, 100, 88 },
      { 76, 80, 76 },
      { 48, 48, 48 } },
    // red
    { { 160, 40, 28 },
      { 184, 44, 32 },
      { 160, 40, 28 },
      { 124, 32, 32 },
      { 84, 20, 32 } },
    // silver
    { { 150, 150, 150 },
      { 230, 230, 230 },
      { 200, 200, 200 },
      { 140, 140, 140 },
      { 100, 100, 100 } },
    // green
    { { 100, 190, 20 },
      { 130, 230, 30 },
      { 100, 190, 20 },
      { 90, 150, 15 },
      { 80, 110, 10 } },
    // gold2
    { { 220, 170, 0 },
      { 255, 200, 0 },
      { 220, 170, 0 },
      { 185, 140, 0 },
      { 150, 100, 0 } },
    // blue2
    { { 0, 170, 220 },
      { 0, 200, 255 },
      { 0, 170, 220 },
      { 0, 140, 185 },
      { 0, 100, 150 } },
    // pink
    { { 220, 140, 170 },
      { 255, 150, 200 },
      { 210, 130, 160 },
      { 165, 100, 120 },
      { 120, 60, 70 } },
};

typedef struct BAR_INFO {
    int32_t type;
    int32_t value;
    int32_t max_value;
    int32_t show;
    int32_t show_mode;
    int32_t blink;
    int32_t timer;
    int32_t color;
    int32_t bar_location;
} BAR_INFO;

static BAR_INFO m_HealthBar = { 0 };
static void Overlay_SetupHealthBar()
{
    m_HealthBar.type = BT_LARA_HEALTH;
    m_HealthBar.value = LARA_HITPOINTS;
    m_HealthBar.max_value = LARA_HITPOINTS;
    m_HealthBar.show_mode = g_Config.healthbar_showing_mode;
    m_HealthBar.show = 0;
    m_HealthBar.blink = 0;
    m_HealthBar.timer = 40;
    m_HealthBar.color = g_Config.healthbar_color;
    m_HealthBar.bar_location = g_Config.healthbar_location;
}

static BAR_INFO m_AirBar = { 0 };
static void Overlay_SetupAirBar()
{
    m_AirBar.type = BT_LARA_AIR;
    m_AirBar.value = LARA_AIR;
    m_AirBar.max_value = LARA_AIR;
    m_AirBar.show_mode = g_Config.airbar_showing_mode;
    m_AirBar.show = 0;
    m_AirBar.blink = 0;
    m_AirBar.timer = 0;
    m_AirBar.color = g_Config.airbar_color;
    m_AirBar.bar_location = g_Config.airbar_location;
}

static BAR_INFO m_EnemyBar = { 0 };
static void Overlay_SetupEnemyBar()
{
    m_EnemyBar.type = BT_ENEMY_HEALTH;
    m_EnemyBar.value = 0;
    m_EnemyBar.max_value = 0;
    m_EnemyBar.show_mode = g_Config.healthbar_showing_mode;
    m_EnemyBar.show = g_Config.enable_enemy_healthbar;
    m_EnemyBar.blink = 0;
    m_EnemyBar.timer = 0;
    m_EnemyBar.color = g_Config.enemy_healthbar_color;
    m_EnemyBar.bar_location = g_Config.enemy_healthbar_location;
}

static void Overlay_GetBarLocation(
    int8_t bar_location, int32_t width, int32_t height, int32_t *x, int32_t *y);
static void Overlay_DrawBar(BAR_INFO *draw_bar);
static void Overlay_OnAmmoTextRemoval(const TEXTSTRING *textstring);
static void Overlay_OnFPSTextRemoval(const TEXTSTRING *textstring);

static void Overlay_GetBlink(BAR_INFO *blink_bar)
{
    if (blink_bar->show_mode == T1M_BSM_PS1) {
        blink_bar->blink = 0;
    }

    static int32_t blink_counter = 0;
    const int32_t percent_max = 100;

    if (blink_bar->value < 0) {
        blink_bar->value = 0;
    } else if (blink_bar->value > blink_bar->max_value) {
        blink_bar->value = blink_bar->max_value;
    }
    int32_t percent = blink_bar->value * 100 / blink_bar->max_value;

    const int32_t blink_interval = 20;
    const int32_t blink_threshold = blink_bar->type == BT_ENEMY_HEALTH ? 0 : 20;
    int32_t blink_time = blink_counter++ % blink_interval;

    blink_bar->blink =
        percent <= blink_threshold && blink_time > blink_interval / 2;
}

static void Overlay_GetBarLocation(
    int8_t bar_location, int32_t width, int32_t height, int32_t *x, int32_t *y)
{
    const int32_t screen_margin_h = 20;
    const int32_t screen_margin_v = 18;
    const int32_t bar_spacing = 8;

    if (bar_location == T1M_BL_TOP_LEFT || bar_location == T1M_BL_BOTTOM_LEFT) {
        *x = screen_margin_h;
    } else if (
        bar_location == T1M_BL_TOP_RIGHT
        || bar_location == T1M_BL_BOTTOM_RIGHT) {
        *x = Screen_GetResWidthDownscaled() - width * g_Config.ui.bar_scale
            - screen_margin_h;
    } else {
        *x = (Screen_GetResWidthDownscaled() - width) / 2;
    }

    if (bar_location == T1M_BL_TOP_LEFT || bar_location == T1M_BL_TOP_CENTER
        || bar_location == T1M_BL_TOP_RIGHT) {
        *y = screen_margin_v + m_BarOffsetY[bar_location];
    } else {
        *y = Screen_GetResHeightDownscaled() - height * g_Config.ui.bar_scale
            - screen_margin_v - m_BarOffsetY[bar_location];
    }

    m_BarOffsetY[bar_location] += height + bar_spacing;
}

static void Overlay_DrawBar(BAR_INFO *draw_bar)
{
    const int32_t percent_max = 100;

    if (draw_bar->value < 0) {
        draw_bar->value = 0;
    } else if (draw_bar->value > draw_bar->max_value) {
        draw_bar->value = draw_bar->max_value;
    }
    int32_t percent = draw_bar->value * 100 / draw_bar->max_value;

    const RGB888 rgb_bgnd = { 0, 0, 0 };
    const RGB888 rgb_border_light = { 128, 128, 128 };
    const RGB888 rgb_border_dark = { 64, 64, 64 };

    int32_t width = 100;
    int32_t height = 5;

    int32_t x = 0;
    int32_t y = 0;
    Overlay_GetBarLocation(draw_bar->bar_location, width, height, &x, &y);

    int32_t padding = Screen_GetResWidth() <= 800 ? 1 : 2;
    int32_t border = 1;
    int32_t sx = Screen_GetRenderScale(x) - padding;
    int32_t sy = Screen_GetRenderScale(y) - padding;
    int32_t sw =
        Screen_GetRenderScale(width) * g_Config.ui.bar_scale + padding * 2;
    int32_t sh =
        Screen_GetRenderScale(height) * g_Config.ui.bar_scale + padding * 2;

    // border
    Output_DrawScreenFlatQuad(
        sx - border, sy - border, sw + border, sh + border, rgb_border_dark);
    Output_DrawScreenFlatQuad(
        sx, sy, sw + border, sh + border, rgb_border_light);

    // background
    Output_DrawScreenFlatQuad(sx, sy, sw, sh, rgb_bgnd);

    // Check if bar should flash or not
    Overlay_GetBlink(draw_bar);

    if (percent && !draw_bar->blink) {
        width = width * percent / percent_max;

        sx = Screen_GetRenderScale(x);
        sy = Screen_GetRenderScale(y);
        sw = Screen_GetRenderScale(width) * g_Config.ui.bar_scale;
        sh = Screen_GetRenderScale(height) * g_Config.ui.bar_scale;

        if (g_Config.enable_smooth_bars) {
            for (int i = 0; i < COLOR_STEPS - 1; i++) {
                RGB888 c1 = m_ColorBarMap[draw_bar->color][i];
                RGB888 c2 = m_ColorBarMap[draw_bar->color][i + 1];
                int32_t lsy = sy + i * sh / (COLOR_STEPS - 1);
                int32_t lsh = sy + (i + 1) * sh / (COLOR_STEPS - 1) - lsy;
                Output_DrawScreenGradientQuad(sx, lsy, sw, lsh, c1, c1, c2, c2);
            }
        } else {
            for (int i = 0; i < COLOR_STEPS; i++) {
                RGB888 color = m_ColorBarMap[draw_bar->color][i];
                int32_t lsy = sy + i * sh / COLOR_STEPS;
                int32_t lsh = sy + (i + 1) * sh / COLOR_STEPS - lsy;
                Output_DrawScreenFlatQuad(sx, lsy, sw, lsh, color);
            }
        }
    }
}

static void Overlay_OnAmmoTextRemoval(const TEXTSTRING *textstring)
{
    m_AmmoText = NULL;
}

static void Overlay_OnFPSTextRemoval(const TEXTSTRING *textstring)
{
    m_FPSText = NULL;
}

void Overlay_Init()
{
    for (int i = 0; i < MAX_PICKUPS; i++) {
        m_Pickups[i].duration = 0;
    }

    Overlay_SetupHealthBar();
    Overlay_SetupAirBar();
    Overlay_SetupEnemyBar();
}

void Overlay_SetHealtBarTimer(int16_t health_bar_timer)
{
    m_HealthBar.timer = health_bar_timer;
}

void Overlay_TimerTick()
{
    m_HealthBar.timer--;
    if (m_HealthBar.timer < 0) {
        Overlay_SetHealtBarTimer(0);
    }
}

void Overlay_DrawHealthBar()
{
    static int32_t old_hit_points = 0;

    for (int32_t i = 0; i < 6; i++) {
        m_BarOffsetY[i] = 0;
    }

    m_HealthBar.value = g_LaraItem->hit_points;
    if (m_HealthBar.value < 0) {
        m_HealthBar.value = 0;
    } else if (m_HealthBar.value > m_HealthBar.max_value) {
        m_HealthBar.value = m_HealthBar.max_value;
    }

    if (old_hit_points != m_HealthBar.value) {
        old_hit_points = m_HealthBar.value;
        Overlay_SetHealtBarTimer(40);
    }

    m_HealthBar.show = m_HealthBar.timer > 0 || m_HealthBar.value <= 0
        || g_Lara.gun_status == LGS_READY;
    switch (m_HealthBar.show_mode) {
    case T1M_BSM_FLASHING_OR_DEFAULT:
        m_HealthBar.show |=
            m_HealthBar.value <= (m_HealthBar.max_value * 20) / 100;
        break;
    case T1M_BSM_FLASHING_ONLY:
        m_HealthBar.show =
            m_HealthBar.value <= (m_HealthBar.max_value * 20) / 100;
        break;
    case T1M_BSM_ALWAYS:
        m_HealthBar.show = 1;
        break;
    case T1M_BSM_NEVER:
        m_HealthBar.show = 0;
        break;
    default:
        break;
    }
    if (!m_HealthBar.show) {
        return;
    }

    Overlay_DrawBar(&m_HealthBar);
}

void Overlay_DrawAirBar()
{
    m_AirBar.value = g_Lara.air;
    if (m_AirBar.value < 0) {
        m_AirBar.value = 0;
    } else if (g_Lara.air > m_AirBar.max_value) {
        m_AirBar.value = m_AirBar.max_value;
    }

    m_AirBar.show = g_Lara.water_status == LWS_UNDERWATER
        || g_Lara.water_status == LWS_SURFACE;
    switch (m_AirBar.show_mode) {
    case T1M_BSM_DEFAULT:
        m_AirBar.show |= g_Lara.air <= (m_AirBar.max_value * 20) / 100;
        break;
    case T1M_BSM_FLASHING_ONLY:
        m_AirBar.show = g_Lara.air <= (m_AirBar.max_value * 20) / 100;
        break;
    case T1M_BSM_NEVER:
        m_AirBar.show = 0;
        break;
    default:
        break;
    }
    if (!m_AirBar.show) {
        return;
    }

    Overlay_DrawBar(&m_AirBar);
}

void Overlay_DrawEnemyBar()
{
    if (!m_EnemyBar.show || !g_Lara.target) {
        return;
    }

    m_EnemyBar.value = g_Lara.target->hit_points;
    m_EnemyBar.max_value = g_Objects[g_Lara.target->object_number].hit_points
        * ((g_SaveGame.bonus_flag & GBF_NGPLUS) ? 2 : 1);

    Overlay_DrawBar(&m_EnemyBar);
}

void Overlay_DrawAmmoInfo()
{
    const double scale = 0.8;
    const int32_t text_height = 17 * scale;
    const int32_t text_offset_x = 3;
    const int32_t screen_margin_h = 20;
    const int32_t screen_margin_v = 18;

    char ammostring[80] = "";

    if (g_Lara.gun_status != LGS_READY || g_OverlayFlag <= 0
        || (g_SaveGame.bonus_flag & GBF_NGPLUS)) {
        if (m_AmmoText) {
            Text_Remove(m_AmmoText);
            m_AmmoText = NULL;
        }
        return;
    }

    switch (g_Lara.gun_type) {
    case LGT_PISTOLS:
        return;
    case LGT_MAGNUMS:
        sprintf(ammostring, "%5d B", g_Lara.magnums.ammo);
        break;
    case LGT_UZIS:
        sprintf(ammostring, "%5d C", g_Lara.uzis.ammo);
        break;
    case LGT_SHOTGUN:
        sprintf(ammostring, "%5d A", g_Lara.shotgun.ammo / SHOTGUN_AMMO_CLIP);
        break;
    default:
        return;
    }

    Overlay_MakeAmmoString(ammostring);

    if (m_AmmoText) {
        Text_ChangeText(m_AmmoText, ammostring);
    } else {
        m_AmmoText = Text_Create(
            -screen_margin_h - text_offset_x, text_height + screen_margin_v,
            ammostring);
        m_AmmoText->on_remove = Overlay_OnAmmoTextRemoval;
        Text_SetScale(m_AmmoText, PHD_ONE * scale, PHD_ONE * scale);
        Text_AlignRight(m_AmmoText, 1);
    }

    m_AmmoText->pos.y = m_BarOffsetY[T1M_BL_TOP_RIGHT]
        ? text_height + screen_margin_v + m_BarOffsetY[T1M_BL_TOP_RIGHT]
        : text_height + screen_margin_v;
}

void Overlay_DrawPickups()
{
    static int32_t old_game_timer = 0;
    int16_t time = g_SaveGame.timer - old_game_timer;
    old_game_timer = g_SaveGame.timer;

    if (time > 0 && time < 60) {
        int32_t sprite_height =
            MIN(ViewPort_GetWidth(), ViewPort_GetHeight() * 320 / 200) / 10;
        int32_t sprite_width = sprite_height * 4 / 3;
        int32_t y = ViewPort_GetHeight() - sprite_height;
        int32_t x = ViewPort_GetWidth() - sprite_height;
        for (int i = 0; i < MAX_PICKUPS; i++) {
            DISPLAYPU *pu = &m_Pickups[i];
            pu->duration -= time;
            if (pu->duration <= 0) {
                pu->duration = 0;
            } else {
                Output_DrawUISprite(
                    x, y, Screen_GetRenderScaleGLRage(12288), pu->sprnum, 4096);

                if (i % MAX_PICKUP_COLUMNS == MAX_PICKUP_COLUMNS - 1) {
                    x = ViewPort_GetWidth() - sprite_height;
                    y -= sprite_height;
                } else {
                    x -= sprite_width;
                }
            }
        }
    }
}

void Overlay_DrawFPSInfo()
{
    static int32_t elapsed = 0;

    if (g_Config.render_flags.fps_counter) {
        if (Clock_GetMS() - elapsed >= 1000) {
            if (m_FPSText) {
                char fps_buf[20];
                sprintf(fps_buf, "%d FPS", g_FPSCounter);
                Text_ChangeText(m_FPSText, fps_buf);
            } else {
                char fps_buf[20];
                sprintf(fps_buf, "? FPS");
                m_FPSText = Text_Create(10, 30, fps_buf);
                m_FPSText->on_remove = Overlay_OnFPSTextRemoval;
            }
            g_FPSCounter = 0;
            elapsed = Clock_GetMS();
        }
    } else if (m_FPSText) {
        Text_Remove(m_FPSText);
        m_FPSText = NULL;
        g_FPSCounter = 0;
    }
}

void Overlay_DrawGameInfo()
{
    if (g_OverlayFlag > 0) {
        Overlay_DrawHealthBar();
        Overlay_DrawAirBar();
        Overlay_DrawEnemyBar();
        Overlay_DrawPickups();
    }

    Overlay_DrawAmmoInfo();
    Overlay_DrawFPSInfo();

    Text_Draw();
}

void Overlay_AddPickup(int16_t object_num)
{
    for (int i = 0; i < MAX_PICKUPS; i++) {
        if (m_Pickups[i].duration <= 0) {
            m_Pickups[i].duration = 75;
            m_Pickups[i].sprnum = g_Objects[object_num].mesh_index;
            return;
        }
    }
}

void Overlay_MakeAmmoString(char *string)
{
    char *c;

    for (c = string; *c != 0; c++) {
        if (*c == 32) {
            continue;
        } else if (*c - 'A' >= 0) {
            *c += 12 - 'A';
        } else {
            *c += 1 - '0';
        }
    }
}
