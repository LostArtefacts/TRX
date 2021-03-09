#include "game/savegame.h"
#include "game/vars.h"
#include "util.h"

void InitialiseStartInfo()
{
    if (!SaveGame[0].bonus_flag) {
        for (int i = 0; i < LV_NUMBER_OF; i++) {
            ModifyStartInfo(i);
            SaveGame[0].start[i].available = 0;
        }
        SaveGame[0].start[LV_GYM].available = 1;
        SaveGame[0].start[LV_FIRSTLEVEL].available = 1;
    }
}

void ModifyStartInfo(int32_t level_num)
{
    START_INFO *start = &SaveGame[0].start[level_num];

    start->got_pistols = 1;
    start->gun_type = LGT_PISTOLS;
    start->pistol_ammo = 1000;

    switch (level_num) {
    case LV_GYM:
        start->num_medis = 0;
        start->num_big_medis = 0;
        start->num_scions = 0;
        start->pistol_ammo = 0;
        start->shotgun_ammo = 0;
        start->magnum_ammo = 0;
        start->uzi_ammo = 0;
        start->got_pistols = 0;
        start->got_shotgun = 0;
        start->got_magnums = 0;
        start->got_uzis = 0;
        start->gun_type = LGT_UNARMED;
        start->gun_status = LGS_ARMLESS;
        break;

    case LV_FIRSTLEVEL:
        start->num_medis = 0;
        start->num_big_medis = 0;
        start->num_scions = 0;
        start->shotgun_ammo = 0;
        start->magnum_ammo = 0;
        start->uzi_ammo = 0;
        start->got_shotgun = 0;
        start->got_magnums = 0;
        start->got_uzis = 0;
        start->gun_status = LGS_ARMLESS;
        break;

    case LV_LEVEL10A:
        start->num_scions = 0;
        start->got_pistols = 0;
        start->got_shotgun = 0;
        start->got_magnums = 0;
        start->got_uzis = 0;
        start->gun_type = LGT_UNARMED;
        start->gun_status = LGS_ARMLESS;
        break;
    }

    if (SaveGame[0].bonus_flag && level_num != LV_GYM) {
        start->got_pistols = 1;
        start->got_shotgun = 1;
        start->got_magnums = 1;
        start->got_uzis = 1;
        start->shotgun_ammo = 1234;
        start->magnum_ammo = 1234;
        start->uzi_ammo = 1234;
        start->gun_type = LGT_UZIS;
    }
}

void T1MInjectGameSaveGame()
{
    INJECT(0x004344D0, InitialiseStartInfo);
    INJECT(0x00434520, ModifyStartInfo);
}
