#include <trx/game/ui/elements/ammo_label.h>

#include <trx/core/strings.h>
#include <trx/game/game.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/gun.h>
#include <trx/game/lara/common.h>
#include <trx/game/ui/elements/label.h>
#include <trx/version.h>

#include <stdio.h>

void UI_AmmoLabel(void)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->gun_status != LGS_READY || Game_IsBonusFlagSet(GBF_NGPLUS)) {
        return;
    }

    int32_t ammo = 0;
    const bool use_icon = g_TRVersion == 1;
    const char *icon_str = nullptr;

    switch (lara->gun_type) {
    case LGT_PISTOLS:
        return;
    case LGT_SHOTGUN:
        ammo = lara->shotgun_ammo.ammo / Gun_GetAmmoClipCount(LGT_SHOTGUN);
        if (use_icon) {
            icon_str = "\\{ammo shotgun}";
        }
        break;
    case LGT_UZIS:
        ammo = lara->uzi_ammo.ammo;
        if (use_icon) {
            icon_str = "\\{ammo uzis}";
        }
        break;
    case LGT_MAGNUMS:
        ammo = lara->magnum_ammo.ammo;
        if (use_icon) {
            icon_str = "\\{ammo magnums}";
        }
        break;
    case LGT_AUTOS:
        ammo = lara->autos_ammo.ammo;
        break;
    case LGT_DESERT_EAGLE:
        ammo = lara->desert_eagle_ammo.ammo;
        break;
    case LGT_M16:
        ammo = lara->m16_ammo.ammo;
        break;
    case LGT_MP5:
        ammo = lara->mp5_ammo.ammo;
        break;
    case LGT_GRENADE:
        ammo = lara->grenade_ammo.ammo;
        break;
    case LGT_ROCKET:
        ammo = lara->rocket_ammo.ammo;
        break;
    case LGT_HARPOON:
        ammo = lara->harpoon_ammo.ammo;
        break;
    default:
        return;
    }

    const char *inner_text = nullptr;
    if (icon_str != nullptr) {
        inner_text = String_FormatStatic("%6d %s", ammo, icon_str);
    } else {
        inner_text = String_FormatStatic("%6d", ammo);
    }
    const char *const outer_text =
        String_FormatStatic(GS(OVERLAY_ITEM_COUNT_FMT), inner_text);

    UI_LabelEx(outer_text, (UI_LABEL_SETTINGS) { .scale = 1.5f });
}
