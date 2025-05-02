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
    char buf[128] = "";

#if TR_VERSION == 1
    switch (lara->gun_type) {
    case LGT_PISTOLS:
        return;
    case LGT_SHOTGUN:
        sprintf(buf, "%6d A", lara->shotgun_ammo.ammo / SHOTGUN_AMMO_CLIP);
        break;
    case LGT_UZIS:
        sprintf(buf, "%6d C", lara->uzi_ammo.ammo);
        break;
    case LGT_MAGNUMS:
        sprintf(buf, "%6d B", lara->magnum_ammo.ammo);
        break;
    default:
        return;
    }
#else
    switch (lara->gun_type) {
    case LGT_MAGNUMS:
        sprintf(buf, "%6d", lara->magnum_ammo.ammo);
        break;
    case LGT_UZIS:
        sprintf(buf, "%6d", lara->uzi_ammo.ammo);
        break;
    case LGT_SHOTGUN:
        sprintf(buf, "%6d", lara->shotgun_ammo.ammo / SHOTGUN_AMMO_CLIP);
        break;
    case LGT_M16:
        sprintf(buf, "%6d", lara->m16_ammo.ammo);
        break;
    case LGT_GRENADE:
        sprintf(buf, "%6d", lara->grenade_ammo.ammo);
        break;
    case LGT_HARPOON:
        sprintf(buf, "%6d", lara->harpoon_ammo.ammo);
        break;
    default:
        return;
    }
#endif

    UI_AmmoLabel_MakeString(buf);
    if (lara->gun_status == LGS_READY && !Game_IsBonusFlagSet(GBF_NGPLUS)) {
        UI_LabelEx(buf, (UI_LABEL_SETTINGS) { .scale = 1.5f });
    }
}
