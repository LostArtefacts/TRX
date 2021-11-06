#include "game/overlay.h"

#include "3dsystem/scalespr.h"
#include "config.h"
#include "game/text.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "specific/clock.h"
#include "specific/display.h"
#include "specific/frontend.h"
#include "specific/output.h"

#include <stdio.h>

#define COLOR_STEPS 5
#define MAX_PICKUP_COLUMNS 4
#define MAX_PICKUPS 16

static int16_t BarOffsetY[6] = { 0 };
static DISPLAYPU Pickups[MAX_PICKUPS] = { 0 };

static RGB888 ColorBarMap[][COLOR_STEPS] = {
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

static void Overlay_GetBarLocation(
    int8_t bar_location, int32_t width, int32_t height, int32_t *x, int32_t *y);
static void Overlay_DrawBar(int32_t value, int32_t value_max, int32_t bar_type);

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
        *x = GetRenderWidthDownscaled() - width * T1MConfig.ui.bar_scale
            - screen_margin_h;
    } else {
        *x = (GetRenderWidthDownscaled() - width) / 2;
    }

    if (bar_location == T1M_BL_TOP_LEFT || bar_location == T1M_BL_TOP_CENTER
        || bar_location == T1M_BL_TOP_RIGHT) {
        *y = screen_margin_v + BarOffsetY[bar_location];
    } else {
        *y = GetRenderHeightDownscaled() - height * T1MConfig.ui.bar_scale
            - screen_margin_v - BarOffsetY[bar_location];
    }

    BarOffsetY[bar_location] += height + bar_spacing;
}

static void Overlay_DrawBar(int32_t value, int32_t value_max, int32_t bar_type)
{
    static int32_t blink_counter = 0;
    const int32_t percent_max = 100;

    if (value < 0) {
        value = 0;
    } else if (value > value_max) {
        value = value_max;
    }
    int32_t percent = value * 100 / value_max;

    const RGB888 rgb_bgnd = { 0, 0, 0 };
    const RGB888 rgb_border_light = { 128, 128, 128 };
    const RGB888 rgb_border_dark = { 64, 64, 64 };

    int32_t width = 100;
    int32_t height = 5;
    int16_t bar_color = bar_type;

    int32_t x = 0;
    int32_t y = 0;
    if (bar_type == BT_LARA_HEALTH) {
        Overlay_GetBarLocation(
            T1MConfig.healthbar_location, width, height, &x, &y);
        bar_color = T1MConfig.healthbar_color;
    } else if (bar_type == BT_LARA_AIR) {
        Overlay_GetBarLocation(
            T1MConfig.airbar_location, width, height, &x, &y);
        bar_color = T1MConfig.airbar_color;
    } else if (bar_type == BT_ENEMY_HEALTH) {
        Overlay_GetBarLocation(
            T1MConfig.enemy_healthbar_location, width, height, &x, &y);
        bar_color = T1MConfig.enemy_healthbar_color;
    }

    int32_t padding = GetScreenWidth() <= 800 ? 1 : 2;
    int32_t border = 1;
    int32_t sx = GetRenderScale(x) - padding;
    int32_t sy = GetRenderScale(y) - padding;
    int32_t sw = GetRenderScale(width) * T1MConfig.ui.bar_scale + padding * 2;
    int32_t sh = GetRenderScale(height) * T1MConfig.ui.bar_scale + padding * 2;

    // border
    S_DrawScreenFlatQuad(
        sx - border, sy - border, sw + border, sh + border, rgb_border_dark);
    S_DrawScreenFlatQuad(sx, sy, sw + border, sh + border, rgb_border_light);

    // background
    S_DrawScreenFlatQuad(sx, sy, sw, sh, rgb_bgnd);

    const int32_t blink_interval = 20;
    const int32_t blink_threshold = bar_type == BT_ENEMY_HEALTH ? 0 : 20;
    int32_t blink_time = blink_counter++ % blink_interval;
    int32_t blink =
        percent <= blink_threshold && blink_time > blink_interval / 2;

    if (percent && !blink) {
        width = width * percent / percent_max;

        sx = GetRenderScale(x);
        sy = GetRenderScale(y);
        sw = GetRenderScale(width) * T1MConfig.ui.bar_scale;
        sh = GetRenderScale(height) * T1MConfig.ui.bar_scale;

        if (T1MConfig.enable_smooth_bars) {
            for (int i = 0; i < COLOR_STEPS - 1; i++) {
                RGB888 c1 = ColorBarMap[bar_color][i];
                RGB888 c2 = ColorBarMap[bar_color][i + 1];
                int32_t lsy = sy + i * sh / (COLOR_STEPS - 1);
                int32_t lsh = sy + (i + 1) * sh / (COLOR_STEPS - 1) - lsy;
                S_DrawScreenGradientQuad(sx, lsy, sw, lsh, c1, c1, c2, c2);
            }
        } else {
            for (int i = 0; i < COLOR_STEPS; i++) {
                RGB888 color = ColorBarMap[bar_color][i];
                int32_t lsy = sy + i * sh / COLOR_STEPS;
                int32_t lsh = sy + (i + 1) * sh / COLOR_STEPS - lsy;
                S_DrawScreenFlatQuad(sx, lsy, sw, lsh, color);
            }
        }
    }
}

void Overlay_Init()
{
    for (int i = 0; i < MAX_PICKUPS; i++) {
        Pickups[i].duration = 0;
    }
}

void Overlay_DrawHealthBar()
{
    static int32_t old_hit_points = 0;

    for (int i = 0; i < 6; i++) {
        BarOffsetY[i] = 0;
    }

    int hit_points = LaraItem->hit_points;
    if (hit_points < 0) {
        hit_points = 0;
    } else if (hit_points > LARA_HITPOINTS) {
        hit_points = LARA_HITPOINTS;
    }

    if (old_hit_points != hit_points) {
        old_hit_points = hit_points;
        HealthBarTimer = 40;
    }

    if (HealthBarTimer < 0) {
        HealthBarTimer = 0;
    }

    int32_t show =
        HealthBarTimer > 0 || hit_points <= 0 || Lara.gun_status == LGS_READY;
    switch (T1MConfig.healthbar_showing_mode) {
    case T1M_BSM_ALWAYS:
        show = 1;
        break;
    case T1M_BSM_NEVER:
        show = 0;
        break;
    case T1M_BSM_FLASHING_OR_DEFAULT:
        show |= hit_points <= (LARA_HITPOINTS * 20) / 100;
        break;
    case T1M_BSM_FLASHING_ONLY:
        show = hit_points <= (LARA_HITPOINTS * 20) / 100;
        break;
    }
    if (!show) {
        return;
    }

    Overlay_DrawBar(hit_points, LARA_HITPOINTS, BT_LARA_HEALTH);
}

void Overlay_DrawAirBar()
{
    int32_t show =
        Lara.water_status == LWS_UNDERWATER || Lara.water_status == LWS_SURFACE;
    switch (T1MConfig.airbar_showing_mode) {
    case T1M_BSM_ALWAYS:
        show = 1;
        break;
    case T1M_BSM_NEVER:
        show = 0;
        break;
    case T1M_BSM_FLASHING_OR_DEFAULT:
        show |= Lara.air <= (LARA_AIR * 20) / 100;
        break;
    case T1M_BSM_FLASHING_ONLY:
        show = Lara.air <= (LARA_AIR * 20) / 100;
        break;
    }
    if (!show) {
        return;
    }

    int air = Lara.air;
    if (air < 0) {
        air = 0;
    } else if (Lara.air > LARA_AIR) {
        air = LARA_AIR;
    }

    Overlay_DrawBar(air, LARA_AIR, BT_LARA_AIR);
}

void Overlay_DrawEnemyBar()
{
    if (!T1MConfig.enable_enemy_healthbar || !Lara.target) {
        return;
    }

    Overlay_DrawBar(
        Lara.target->hit_points,
        Objects[Lara.target->object_number].hit_points
            * ((SaveGame.bonus_flag & GBF_NGPLUS) ? 2 : 1),
        BT_ENEMY_HEALTH);
}

void Overlay_DrawAmmoInfo()
{
    const double scale = 0.8;
    const int32_t text_height = 17 * scale;
    const int32_t text_offset_x = 3;
    const int32_t screen_margin_h = 20;
    const int32_t screen_margin_v = 18;

    char ammostring[80] = "";

    if (Lara.gun_status != LGS_READY || OverlayFlag <= 0
        || (SaveGame.bonus_flag & GBF_NGPLUS)) {
        if (AmmoText) {
            Text_Remove(AmmoText);
            AmmoText = NULL;
        }
        return;
    }

    switch (Lara.gun_type) {
    case LGT_PISTOLS:
        return;
    case LGT_MAGNUMS:
        sprintf(ammostring, "%5d B", Lara.magnums.ammo);
        break;
    case LGT_UZIS:
        sprintf(ammostring, "%5d C", Lara.uzis.ammo);
        break;
    case LGT_SHOTGUN:
        sprintf(ammostring, "%5d A", Lara.shotgun.ammo / SHOTGUN_AMMO_CLIP);
        break;
    default:
        return;
    }

    Overlay_MakeAmmoString(ammostring);

    if (AmmoText) {
        Text_ChangeText(AmmoText, ammostring);
    } else {
        AmmoText = Text_Create(
            -screen_margin_h - text_offset_x, text_height + screen_margin_v,
            ammostring);
        Text_SetScale(AmmoText, PHD_ONE * scale, PHD_ONE * scale);
        Text_AlignRight(AmmoText, 1);
    }

    AmmoText->pos.y = BarOffsetY[T1M_BL_TOP_RIGHT]
        ? text_height + screen_margin_v + BarOffsetY[T1M_BL_TOP_RIGHT]
        : text_height + screen_margin_v;
}

void Overlay_DrawPickups()
{
    static int32_t old_game_timer = 0;
    int16_t time = SaveGame.timer - old_game_timer;
    old_game_timer = SaveGame.timer;

    if (time > 0 && time < 60) {
        int32_t sprite_height = MIN(PhdWinWidth, PhdWinHeight * 320 / 200) / 10;
        int32_t sprite_width = sprite_height * 4 / 3;
        int32_t y = PhdWinHeight - sprite_height;
        int32_t x = PhdWinWidth - sprite_height;
        for (int i = 0; i < MAX_PICKUPS; i++) {
            DISPLAYPU *pu = &Pickups[i];
            pu->duration -= time;
            if (pu->duration <= 0) {
                pu->duration = 0;
            } else {
                S_DrawUISprite(
                    x, y, GetRenderScaleGLRage(12288), pu->sprnum, 4096);

                if (i % MAX_PICKUP_COLUMNS == MAX_PICKUP_COLUMNS - 1) {
                    x = PhdWinWidth - sprite_height;
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

    if (T1MConfig.render_flags.fps_counter) {
        if (ClockGetMS() - elapsed >= 1000) {
            if (FPSText) {
                char fps_buf[20];
                sprintf(fps_buf, "%d FPS", FPSCounter);
                Text_ChangeText(FPSText, fps_buf);
            } else {
                char fps_buf[20];
                sprintf(fps_buf, "? FPS");
                FPSText = Text_Create(10, 30, fps_buf);
            }
            FPSCounter = 0;
            elapsed = ClockGetMS();
        }
    } else if (FPSText) {
        Text_Remove(FPSText);
        FPSText = NULL;
        FPSCounter = 0;
    }
}

void Overlay_DrawGameInfo()
{
    if (OverlayFlag > 0) {
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
        if (Pickups[i].duration <= 0) {
            Pickups[i].duration = 75;
            Pickups[i].sprnum = Objects[object_num].mesh_index;
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
