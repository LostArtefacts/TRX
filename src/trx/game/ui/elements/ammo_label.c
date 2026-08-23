#include <trx/game/ui/elements/ammo_label.h>

#include <trx/config.h>
#include <trx/core/strings.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/gun.h>
#include <trx/game/gun/registry.h>
#include <trx/game/inventory.h>
#include <trx/game/lara/common.h>
#include <trx/game/lara/vehicle.h>
#include <trx/game/ui/elements/label.h>
#include <trx/version.h>

#include <stdio.h>

bool UI_AmmoLabel(void)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();

    int32_t ammo = 0;
    const bool use_icon = g_TRVersion == 1;
    const char *icon_str = nullptr;

    const LARA_GUN_TYPE vehicle_gun = Lara_Vehicle_GetGunType();
    if (vehicle_gun != LGT_UNARMED) {
        if (Gun_HasInfiniteAmmo(vehicle_gun)) {
            return false;
        }
        ammo = Inv_GetAmmo(vehicle_gun);
    } else {
        if (lara->gun_status != LGS_READY) {
            return false;
        }

        // A weapon that spends nothing carries no label, and neither does one
        // with no box of ammunition to draw on.
        if (Gun_HasInfiniteAmmo(lara->gun_type)
            || Gun_GetAmmoObject(lara->gun_type) == NO_OBJECT) {
            return false;
        }

        // What the label counts is shots, which for the shotgun is fewer than
        // the rounds behind them.
        ammo =
            Inv_GetAmmo(lara->gun_type) / Gun_GetRoundsPerShot(lara->gun_type);

        const WEAPON_INFO *const info = Gun_Registry_Get(lara->gun_type);
        if (use_icon) {
            icon_str = info->ammo_icon;
        }
    }

    const char *inner_text = nullptr;
    if (icon_str != nullptr) {
        inner_text = String_FormatStatic("%6d %s", ammo, icon_str);
    } else {
        inner_text = String_FormatStatic("%6d", ammo);
    }
    const char *const outer_text = String_FormatStatic(
        g_Config.ui.menu_style == UI_STYLE_PS1
            ? GS("general/overlay/item_count_fmt_ps1")
            : GS("general/overlay/item_count_fmt_pc"),
        inner_text);

    UI_LabelEx(outer_text, (UI_LABEL_SETTINGS) { .scale = 1.5f });
    return true;
}
