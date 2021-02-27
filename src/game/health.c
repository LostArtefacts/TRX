#include "3dsystem/scalespr.h"
#include "game/health.h"
#include "game/text.h"
#include "game/vars.h"
#include "specific/output.h"
#include "config.h"
#include "util.h"

void DrawGameInfo()
{
    if (OverlayFlag > 0) {
        DrawHealthBar();
        DrawAirBar();
#ifdef T1M_FEAT_GAMEPLAY
        DrawEnemyBar();
#endif
        DrawPickups();
    }

    // NOTE: this was drawn before the healthbars in the original code.
    // In T1M it's called after drawing the healthbars, as it updates the
    // healthbars position and thus vertical offset for the ammo text.
    DrawAmmoInfo();

    T_DrawText();
}

void DrawHealthBar()
{
#ifdef T1M_FEAT_UI
    for (int i = 0; i < 6; i++) {
        BarOffsetY[i] = 0;
    }
#endif

    int hit_points = LaraItem->hit_points;
    if (hit_points < 0) {
        hit_points = 0;
    } else if (hit_points > LARA_HITPOINTS) {
        hit_points = LARA_HITPOINTS;
    }

    if (OldHitPoints != hit_points) {
        OldHitPoints = hit_points;
        HealthBarTimer = 40;
    }

    if (HealthBarTimer < 0) {
        HealthBarTimer = 0;
    }

#ifdef T1M_FEAT_GAMEPLAY
    int32_t show =
        HealthBarTimer > 0 || hit_points <= 0 || Lara.gun_status == LGS_READY;
    switch (T1MConfig.healthbar_showing_mode) {
    case T1M_BSM_ALWAYS:
        show = 1;
        break;
    case T1M_BSM_NEVER:
        show = 0;
        return;
    case T1M_BSM_FLASHING:
        if (hit_points <= (LARA_HITPOINTS * 20) / 100) {
            show = 1;
        }
        break;
    }
    if (!show) {
        return;
    }
#else
    if (HealthBarTimer <= 0 && hit_points > 0 && Lara.gun_status != LGS_READY) {
        return;
    }
#endif

    S_DrawHealthBar(hit_points * 100 / LARA_HITPOINTS);
}

void DrawAirBar()
{
#ifdef T1M_FEAT_GAMEPLAY
    int32_t show =
        Lara.water_status == LWS_UNDERWATER || Lara.water_status == LWS_SURFACE;
    switch (T1MConfig.airbar_showing_mode) {
    case T1M_BSM_ALWAYS:
        show = 1;
        break;
    case T1M_BSM_NEVER:
        show = 0;
        return;
    case T1M_BSM_FLASHING:
        if (Lara.air > (LARA_AIR * 20) / 100) {
            show = 0;
        }
        break;
    }
    if (!show) {
        return;
    }
#else
    if (Lara.water_status != LWS_UNDERWATER
        && Lara.water_status != LWS_SURFACE) {
        return;
    }
#endif

    int air = Lara.air;
    if (air < 0) {
        air = 0;
    } else if (Lara.air > LARA_AIR) {
        air = LARA_AIR;
    }

    S_DrawAirBar(air * 100 / LARA_AIR);
}

#ifdef T1M_FEAT_GAMEPLAY
void DrawEnemyBar()
{
    if (!T1MConfig.enable_enemy_healthbar || !Lara.target) {
        return;
    }

    RenderBar(
        Lara.target->hit_points,
        Objects[Lara.target->object_number].hit_points
            * (SaveGame[0].bonus_flag ? 2 : 1),
        BT_ENEMY_HEALTH);
}
#endif

void DrawAmmoInfo()
{
    char ammostring[80] = "";

    if (Lara.gun_status != LGS_READY || OverlayFlag <= 0
        || SaveGame[0].bonus_flag) {
        if (AmmoText) {
            T_RemovePrint(AmmoText);
            AmmoText = 0;
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
        AmmoText = T_Print(-17, 22, 0, ammostring);
        T_RightAlign(AmmoText, 1);
    }

#ifdef T1M_FEAT_UI
    AmmoText->ypos = BarOffsetY[T1M_BL_TOP_RIGHT]
        ? 30 + (int)(BarOffsetY[T1M_BL_TOP_RIGHT] * 10 / GetRenderScale(10))
        : 22;
#endif
}

void MakeAmmoString(char* string)
{
    char* c;

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
    for (int i = 0; i < NUM_PU; i++) {
        Pickups[i].duration = 0;
    }
}

void DrawPickups()
{
    int old_game_timer = OldGameTimer;
    OldGameTimer = SaveGame[0].timer;
    int16_t time = SaveGame[0].timer - old_game_timer;

    if (time > 0 && time < 60) {
        int y = PhdWinHeight - PhdWinWidth / 10;
        int x = PhdWinWidth - PhdWinWidth / 10;
        int sprite_width = 4 * (PhdWinWidth / 10) / 3;
        for (int i = 0; i < NUM_PU; i++) {
            DISPLAYPU* pu = &Pickups[i];
            pu->duration -= time;
            if (pu->duration <= 0) {
                pu->duration = 0;
            } else {
#ifdef T1M_FEAT_UI
                S_DrawUISprite(x, y, GetRenderScale(12288), pu->sprnum, 4096);
#else
                S_DrawUISprite(x, y, 12288, pu->sprnum, 4096);
#endif
                x -= sprite_width;
            }
        }
    }
}

void AddDisplayPickup(int16_t objnum)
{
    for (int i = 0; i < NUM_PU; i++) {
        if (Pickups[i].duration <= 0) {
            Pickups[i].duration = 75;
            Pickups[i].sprnum = Objects[objnum].mesh_index;
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
