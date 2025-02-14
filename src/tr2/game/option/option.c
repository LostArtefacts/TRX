#include "game/option/option.h"

#include "game/input.h"
#include "global/vars.h"

void Option_Control(INVENTORY_ITEM *const item, const bool is_busy)
{
    switch (item->object_id) {
    case O_PASSPORT_OPTION:
        Option_Passport_Control(item, is_busy);
        break;
    case O_COMPASS_OPTION:
        Option_Compass_Control(item, is_busy);
        break;
    case O_DETAIL_OPTION:
        Option_Detail_Control(item, is_busy);
        break;
    case O_SOUND_OPTION:
        Option_Sound_Control(item, is_busy);
        break;
    case O_CONTROL_OPTION:
        Option_Controls_Control(item, is_busy);
        break;
    case O_GAMMA_OPTION:
        break;

    case O_PISTOL_OPTION:
    case O_SHOTGUN_OPTION:
    case O_MAGNUM_OPTION:
    case O_UZI_OPTION:
    case O_HARPOON_OPTION:
    case O_M16_OPTION:
    case O_GRENADE_OPTION:
    case O_SMALL_MEDIPACK_OPTION:
    case O_LARGE_MEDIPACK_OPTION:
    case O_PUZZLE_OPTION_1:
    case O_PUZZLE_OPTION_2:
    case O_PUZZLE_OPTION_3:
    case O_PUZZLE_OPTION_4:
    case O_KEY_OPTION_1:
    case O_KEY_OPTION_2:
    case O_KEY_OPTION_3:
    case O_KEY_OPTION_4:
    case O_PICKUP_OPTION_1:
    case O_PICKUP_OPTION_2:
        if (!is_busy) {
            g_InputDB.menu_confirm = 1;
        }
        break;

    case O_PISTOL_AMMO_OPTION:
    case O_SHOTGUN_AMMO_OPTION:
    case O_MAGNUM_AMMO_OPTION:
    case O_UZI_AMMO_OPTION:
    case O_HARPOON_AMMO_OPTION:
    case O_M16_AMMO_OPTION:
    case O_GRENADE_AMMO_OPTION:
        return;

    default:
        if (!is_busy && (g_InputDB.menu_confirm || g_InputDB.menu_back)) {
            item->goal_frame = 0;
            item->anim_direction = -1;
        }
        break;
    }
}

void Option_Draw(INVENTORY_ITEM *const item)
{
    switch (item->object_id) {
    case O_PASSPORT_OPTION:
        Option_Passport_Draw(item);
        break;
    case O_COMPASS_OPTION:
        Option_Compass_Draw(item);
        break;
    case O_DETAIL_OPTION:
        Option_Detail_Draw(item);
        break;
    case O_SOUND_OPTION:
        Option_Sound_Draw(item);
        break;
    case O_CONTROL_OPTION:
        Option_Controls_Draw(item);
        break;
    case O_GAMMA_OPTION:
        break;
    default:
        break;
    }
}

void Option_Shutdown(INVENTORY_ITEM *const item)
{
    switch (item->object_id) {
    case O_PASSPORT_OPTION:
        Option_Passport_Shutdown();
        break;

    case O_DETAIL_OPTION:
        Option_Detail_Shutdown();
        break;
    case O_SOUND_OPTION:
        Option_Sound_Shutdown();
        break;
    case O_CONTROL_OPTION:
        Option_Controls_Shutdown();
        break;
    case O_COMPASS_OPTION:
        Option_Compass_Shutdown();
        break;
    default:
        break;
    }
}
