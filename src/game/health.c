#include "3dsystem/scalespr.h"
#include "game/health.h"
#include "game/text.h"
#include "game/vars.h"
#include "specific/output.h"
#include "mod.h"
#include "util.h"

void __cdecl DrawGameInfo()
{
    DrawAmmoInfo();
    if (OverlayFlag > 0) {
        DrawHealthBar();
        DrawAirBar();
#ifdef TOMB1M_FEAT_GAMEPLAY
        DrawEnemyBar();
#endif
        DrawPickups();
    }

    T_DrawText();
}

void __cdecl DrawHealthBar()
{
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

    if (HealthBarTimer > 0 || hit_points <= 0 || Lara.gun_status == LGS_READY) {
        S_DrawHealthBar(hit_points * 100 / LARA_HITPOINTS);
    }
}

void __cdecl DrawAirBar()
{
    if (Lara.water_status != LWS_UNDERWATER
        && Lara.water_status != LWS_SURFACE) {
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

#ifdef TOMB1M_FEAT_GAMEPLAY
void __cdecl DrawEnemyBar()
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

void __cdecl DrawAmmoInfo()
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
}

void __cdecl MakeAmmoString(char* string)
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

void __cdecl InitialisePickUpDisplay()
{
    for (int i = 0; i < NUM_PU; i++) {
        Pickups[i].duration = 0;
    }
}

void __cdecl DrawPickups()
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
#ifdef TOMB1M_FEAT_UI
                S_DrawUISprite(x, y, GetRenderScale(12288), pu->sprnum, 4096);
#else
                S_DrawUISprite(x, y, 12288, pu->sprnum, 4096);
#endif
                x -= sprite_width;
            }
        }
    }
}

void __cdecl AddDisplayPickup(int16_t objnum)
{
    for (int i = 0; i < NUM_PU; i++) {
        if (Pickups[i].duration <= 0) {
            Pickups[i].duration = 75;
            Pickups[i].sprnum = Objects[objnum].mesh_index;
            return;
        }
    }
}

void Tomb1MInjectGameHealth()
{
    INJECT(0x0041DD00, DrawGameInfo);
    INJECT(0x0041DEA0, DrawHealthBar);
    INJECT(0x0041DF20, MakeAmmoString);
    INJECT(0x0041DF50, DrawAmmoInfo);
    INJECT(0x0041E0A0, InitialisePickUpDisplay);
    INJECT(0x0041E0C0, AddDisplayPickup);
}
