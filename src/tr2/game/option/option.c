#include "game/option/option.h"

#include "game/input.h"
#include "global/funcs.h"
#include "global/vars.h"

void __cdecl Option_DoInventory(INVENTORY_ITEM *const item)
{
    switch (item->object_id) {
    case O_PASSPORT_OPTION:
        Option_Passport(item);
        break;
    case O_COMPASS_OPTION:
        Option_Compass(item);
        break;
    case O_DETAIL_OPTION:
        Option_Detail(item);
        break;
    case O_SOUND_OPTION:
        Option_Sound(item);
        break;
    case O_CONTROL_OPTION:
        Option_Controls(item);
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
        g_InputDB |= IN_SELECT;
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
        if ((g_InputDB & IN_SELECT) || (g_InputDB & IN_DESELECT)) {
            item->goal_frame = 0;
            item->anim_direction = -1;
        }
        break;
    }
}

void __cdecl Option_ShutdownInventory(INVENTORY_ITEM *const item)
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
    }
}
