#include "game/ui/elements/ammo_label.h"

#include "game/game.h"
#include "game/gun/const.h"
#include "game/lara/common.h"
#include "game/ui/elements/label.h"

#include <stdio.h>
#include <string.h>

void UI_AmmoLabel_MakeString(char *const string)
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

void UI_AmmoLabel(void)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    char ammo_string[128] = "";
    switch (lara->gun_type) {
    case LGT_PISTOLS:
        return;
    case LGT_SHOTGUN:
        sprintf(
            ammo_string, "%6d A", lara->shotgun_ammo.ammo / SHOTGUN_AMMO_CLIP);
        break;
    case LGT_UZIS:
        sprintf(ammo_string, "%6d C", lara->uzi_ammo.ammo);
        break;
    case LGT_MAGNUMS:
        sprintf(ammo_string, "%6d B", lara->magnum_ammo.ammo);
        break;
    default:
        return;
    }
    UI_AmmoLabel_MakeString(ammo_string);
    if (lara->gun_status == LGS_READY && !Game_IsBonusFlagSet(GBF_NGPLUS)) {
        UI_LabelEx(ammo_string, (UI_LABEL_SETTINGS) { .scale = 1.5f });
    }
}
