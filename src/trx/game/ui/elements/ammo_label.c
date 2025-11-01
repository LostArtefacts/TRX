#include <trx/game/ui/elements/ammo_label.h>

#include <trx/game/game.h>
#include <trx/game/gun/const.h>
#include <trx/game/lara/common.h>
#include <trx/game/ui/elements/label.h>
#include <trx/strings.h>
#include <trx/version.h>

#include <stdio.h>

void UI_AmmoLabel(void)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    const char *string = nullptr;

    if (g_TRVersion == 1) {
        switch (lara->gun_type) {
        case LGT_PISTOLS:
            return;
        case LGT_SHOTGUN:
            string = String_FormatStatic(
                "%6d \\{ammo shotgun}",
                lara->shotgun_ammo.ammo / SHOTGUN_AMMO_CLIP);
            break;
        case LGT_UZIS:
            string =
                String_FormatStatic("%6d \\{ammo uzis}", lara->uzi_ammo.ammo);
            break;
        case LGT_MAGNUMS:
            string = String_FormatStatic(
                "%6d \\{ammo magnums}", lara->magnum_ammo.ammo);
            break;
        default:
            return;
        }
    } else {
        switch (lara->gun_type) {
        case LGT_MAGNUMS:
            string = String_FormatStatic("%6d", lara->magnum_ammo.ammo);
            break;
        case LGT_UZIS:
            string = String_FormatStatic("%6d", lara->uzi_ammo.ammo);
            break;
        case LGT_SHOTGUN:
            string = String_FormatStatic(
                "%6d", lara->shotgun_ammo.ammo / SHOTGUN_AMMO_CLIP);
            break;
#if TR_VERSION >= 2
        case LGT_M16:
            string = String_FormatStatic("%6d", lara->m16_ammo.ammo);
            break;
        case LGT_GRENADE:
            string = String_FormatStatic("%6d", lara->grenade_ammo.ammo);
            break;
        case LGT_HARPOON:
            string = String_FormatStatic("%6d", lara->harpoon_ammo.ammo);
            break;
#endif
        default:
            return;
        }
    }

    if (lara->gun_status == LGS_READY && !Game_IsBonusFlagSet(GBF_NGPLUS)) {
        UI_LabelEx(
            String_StylizeSmallDigitsStatic(string),
            (UI_LABEL_SETTINGS) { .scale = 1.5f });
    }
}
