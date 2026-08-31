#include <trx/game/option/common.h>

#include <trx/core/subsystem.h>
#include <trx/game/input.h>
#include <trx/game/objects.h>
#include <trx/game/objects/families.h>
#include <trx/game/option/controls.h>
#include <trx/game/option/examine.h>
#include <trx/game/option/gameplay.h>
#include <trx/game/option/globe_select.h>
#include <trx/game/option/graphics.h>
#include <trx/game/option/passport.h>
#include <trx/game/option/save_crystal.h>
#include <trx/game/option/sound.h>
#include <trx/game/option/stats.h>
#include <trx/version.h>

static void M_Shutdown(void)
{
    Option_Gameplay_Shutdown();
    Option_Graphics_Shutdown();
    Option_Sound_Shutdown();
    Option_Controls_Shutdown();
    Option_GlobeSelect_Shutdown();
}

void Option_Reset(void)
{
    M_Shutdown();
}

void Option_Control(INVENTORY_ITEM *const inv_item, const bool is_busy)
{
    if (inv_item->action == ACTION_EXAMINE) {
        Option_Examine_Control(inv_item, is_busy);
        return;
    }

    switch (inv_item->object_id) {
    case O_PASSPORT_OPTION:
        Option_Passport_Control(inv_item, is_busy);
        break;
    case O_COMPASS_OPTION:
    case O_STOPWATCH_OPTION:
        Option_Stats_Control(inv_item, is_busy);
        break;
    case O_PDA_OPTION:
        Option_Gameplay_Control(inv_item, is_busy);
        break;
    case O_GRAPHICS_OPTION:
        Option_Graphics_Control(inv_item, is_busy);
        break;
    case O_SOUND_OPTION:
        Option_Sound_Control(inv_item, is_busy);
        break;
    case O_CONTROLS_OPTION:
        Option_Controls_Control(inv_item, is_busy);
        break;
    case O_GLOBE_OPTION:
        Option_GlobeSelect_Control(inv_item, is_busy);
        break;
    case O_SAVE_CRYSTAL_OPTION:
        Option_SaveCrystal_Control(inv_item, is_busy);
        break;

    case O_PISTOLS_OPTION:
    case O_SHOTGUN_OPTION:
    case O_MAGNUMS_OPTION:
    case O_AUTOS_OPTION:
    case O_DESERT_EAGLE_OPTION:
    case O_UZIS_OPTION:
    case O_HARPOON_GUN_OPTION:
    case O_M16_OPTION:
    case O_MP5_OPTION:
    case O_GRENADE_GUN_OPTION:
    case O_ROCKET_GUN_OPTION:
    case O_SMALL_MEDIPACK_OPTION:
    case O_LARGE_MEDIPACK_OPTION:
        if (!is_busy) {
            g_InputDB.menu_confirm = 1;
        }
        break;

    case O_PISTOLS_AMMO_OPTION:
    case O_SHOTGUN_AMMO_OPTION:
    case O_MAGNUMS_AMMO_OPTION:
    case O_AUTOS_AMMO_OPTION:
    case O_DESERT_EAGLE_AMMO_OPTION:
    case O_UZIS_AMMO_OPTION:
    case O_HARPOON_GUN_AMMO_OPTION:
    case O_M16_AMMO_OPTION:
    case O_MP5_AMMO_OPTION:
    case O_GRENADE_AMMO_OPTION:
    case O_ROCKET_AMMO_OPTION:
        break;

    default:
        if (inv_item->object_id == O_SCION_OPTION
            || ObjectFamily_Has(
                inv_item->object_id, OBJ_FAMILY_GENERIC_INV_OPTION)) {
            if (!is_busy) {
                g_InputDB.menu_confirm = 1;
            }
        } else if (
            !is_busy && (g_InputDB.menu_confirm || g_InputDB.menu_back)) {
            inv_item->goal_frame = 0;
            inv_item->anim_direction = -1;
        }
        break;
    }
}

void Option_Draw(INVENTORY_ITEM *const inv_item)
{
    if (inv_item->action == ACTION_EXAMINE) {
        Option_Examine_Draw();
        return;
    }

    switch (inv_item->object_id) {
    case O_PASSPORT_OPTION:
        Option_Passport_Draw(inv_item);
        break;
    case O_COMPASS_OPTION:
    case O_STOPWATCH_OPTION:
        Option_Stats_Draw();
        break;
    case O_PDA_OPTION:
        Option_Gameplay_Draw(inv_item);
        break;
    case O_GRAPHICS_OPTION:
        Option_Graphics_Draw(inv_item);
        break;
    case O_SOUND_OPTION:
        Option_Sound_Draw(inv_item);
        break;
    case O_CONTROLS_OPTION:
        Option_Controls_Draw(inv_item);
        break;
    case O_GLOBE_OPTION:
        Option_GlobeSelect_Draw(inv_item);
        break;
    case O_SAVE_CRYSTAL_OPTION:
        Option_SaveCrystal_Draw();
        break;
    default:
        break;
    }
}

void Option_Close(const INVENTORY_ITEM *const inv_item)
{
    switch (inv_item->object_id) {
    case O_PASSPORT_OPTION:
        Option_Passport_Close();
        break;
    case O_COMPASS_OPTION:
    case O_STOPWATCH_OPTION:
        Option_Stats_Close();
        break;
    case O_PDA_OPTION:
        Option_Gameplay_Close();
        break;
    case O_GRAPHICS_OPTION:
        Option_Graphics_Close();
        break;
    case O_SOUND_OPTION:
        Option_Sound_Close();
        break;
    case O_CONTROLS_OPTION:
        Option_Controls_Close();
        break;
    case O_GLOBE_OPTION:
        Option_GlobeSelect_Close();
        break;
    case O_SAVE_CRYSTAL_OPTION:
        Option_SaveCrystal_Close();
        break;
    default:
        Option_Examine_Close();
        break;
    }
}

REGISTER_SUBSYSTEM(.shutdown = M_Shutdown)
