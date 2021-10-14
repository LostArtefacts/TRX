#include "game/health.h"

#include "3dsystem/scalespr.h"
#include "config.h"
#include "game/text.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "specific/clock.h"
#include "specific/output.h"
#include "util.h"

#include <stdio.h>

#define MAX_PICKUP_COLUMNS 4
#define MAX_PICKUPS 16

static DISPLAYPU Pickups[MAX_PICKUPS];

void DrawGameInfo()
{
    if (OverlayFlag > 0) {
        DrawHealthBar();
        DrawAirBar();
        DrawEnemyBar();
        DrawPickups();
    }

    DrawAmmoInfo();
    DrawFPSInfo();

    T_DrawText();
}

void DrawFPSInfo()
{
    static char fps_buf[20];
    static int32_t elapsed = 0;

    if (RenderSettings & RSF_FPS) {
        if (ClockGetMS() - elapsed >= 1000) {
            if (FPSText) {
                sprintf(fps_buf, "%d FPS", FPSCounter);
                T_ChangeText(FPSText, fps_buf);
            } else {
                sprintf(fps_buf, "? FPS");
                FPSText = T_Print(10, 30, fps_buf);
            }
            FPSCounter = 0;
            elapsed = ClockGetMS();
        }
    } else if (FPSText) {
        T_RemovePrint(FPSText);
        FPSText = NULL;
        FPSCounter = 0;
    }
}

void DrawHealthBar()
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

    S_DrawHealthBar(hit_points * 100 / LARA_HITPOINTS);
}

void DrawAirBar()
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

    S_DrawAirBar(air * 100 / LARA_AIR);
}

void DrawEnemyBar()
{
    if (!T1MConfig.enable_enemy_healthbar || !Lara.target) {
        return;
    }

    RenderBar(
        Lara.target->hit_points,
        Objects[Lara.target->object_number].hit_points
            * ((SaveGame.bonus_flag & GBF_NGPLUS) ? 2 : 1),
        BT_ENEMY_HEALTH);
}

void DrawAmmoInfo()
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
            T_RemovePrint(AmmoText);
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

    MakeAmmoString(ammostring);

    if (AmmoText) {
        T_ChangeText(AmmoText, ammostring);
    } else {
        AmmoText = T_Print(
            -screen_margin_h - text_offset_x, text_height + screen_margin_v,
            ammostring);
        T_SetScale(AmmoText, PHD_ONE * scale, PHD_ONE * scale);
        T_RightAlign(AmmoText, 1);
    }

    AmmoText->ypos = BarOffsetY[T1M_BL_TOP_RIGHT]
        ? text_height + screen_margin_v + BarOffsetY[T1M_BL_TOP_RIGHT]
        : text_height + screen_margin_v;
}

void MakeAmmoString(char *string)
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

void InitialisePickUpDisplay()
{
    for (int i = 0; i < MAX_PICKUPS; i++) {
        Pickups[i].duration = 0;
    }
}

void DrawPickups()
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

void AddDisplayPickup(int16_t object_num)
{
    for (int i = 0; i < MAX_PICKUPS; i++) {
        if (Pickups[i].duration <= 0) {
            Pickups[i].duration = 75;
            Pickups[i].sprnum = Objects[object_num].mesh_index;
            return;
        }
    }
}

void T1MInjectGameHealth()
{
    INJECT(0x0041DD00, DrawGameInfo);
    INJECT(0x0041DEA0, DrawHealthBar);
    INJECT(0x0041DF20, MakeAmmoString);
    INJECT(0x0041DF50, DrawAmmoInfo);
    INJECT(0x0041E0A0, InitialisePickUpDisplay);
    INJECT(0x0041E0C0, AddDisplayPickup);
}
