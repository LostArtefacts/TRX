
#include "game/option/option_compass.h"
#include "game/option/option_passport.h"

#include <libtrx/game/input.h>
#include <libtrx/game/option.h>
#include <libtrx/game/option/controls.h>
#include <libtrx/game/option/examine.h>
#include <libtrx/game/option/gameplay.h>
#include <libtrx/game/option/graphics.h>
#include <libtrx/game/option/sound.h>

void Option_Close(const INVENTORY_ITEM *const inv_item)
{
    switch (inv_item->object_id) {
    case O_PASSPORT_OPTION:
        Option_Passport_Close();
        break;
    case O_COMPASS_OPTION:
        Option_Compass_Close();
        break;
    case O_MAP_OPTION:
        Option_Gameplay_Close();
        break;
    case O_DETAIL_OPTION:
        Option_Graphics_Close();
        break;
    case O_SOUND_OPTION:
        Option_Sound_Close();
        break;
    case O_CONTROL_OPTION:
        Option_Controls_Close();
        break;
    default:
        Option_Examine_Close();
        break;
    }
}

void Option_Control(INVENTORY_ITEM *inv_item, const bool is_busy)
{
    if (inv_item->action == ACTION_EXAMINE) {
        Option_Examine_Control(inv_item->object_id, is_busy);
        return;
    }

    switch (inv_item->object_id) {
    case O_PASSPORT_OPTION:
        Option_Passport_Control(inv_item, is_busy);
        break;

    case O_COMPASS_OPTION:
        Option_Compass_Control(inv_item, is_busy);
        break;

    case O_MAP_OPTION:
        Option_Gameplay_Control(inv_item, is_busy);
        break;

    case O_DETAIL_OPTION:
        Option_Graphics_Control(inv_item, is_busy);
        break;

    case O_SOUND_OPTION:
        Option_Sound_Control(inv_item, is_busy);
        break;

    case O_CONTROL_OPTION:
        Option_Controls_Control(inv_item, is_busy);
        break;

    case O_GAMMA_OPTION:
        // not implemented in TombATI
        break;

    case O_PISTOL_OPTION:
    case O_SHOTGUN_OPTION:
    case O_MAGNUM_OPTION:
    case O_UZI_OPTION:
    case O_EXPLOSIVE_OPTION:
    case O_SMALL_MEDIPACK_OPTION:
    case O_LARGE_MEDIPACK_OPTION:
        if (!is_busy) {
            g_InputDB.menu_confirm = 1;
        }
        break;

    case O_PISTOL_AMMO_OPTION:
    case O_SHOTGUN_AMMO_OPTION:
    case O_MAGNUM_AMMO_OPTION:
    case O_UZI_AMMO_OPTION:
        break;

    case O_PICKUP_OPTION_1:
    case O_PICKUP_OPTION_2:
    case O_PUZZLE_OPTION_1:
    case O_PUZZLE_OPTION_2:
    case O_PUZZLE_OPTION_3:
    case O_PUZZLE_OPTION_4:
    case O_KEY_OPTION_1:
    case O_KEY_OPTION_2:
    case O_KEY_OPTION_3:
    case O_KEY_OPTION_4:
    case O_SCION_OPTION:
    case O_LEADBAR_OPTION:
        if (!is_busy) {
            g_InputDB.menu_confirm = 1;
        }
        break;

    default:
        if (!is_busy && (g_InputDB.menu_confirm || g_InputDB.menu_back)) {
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
    case O_MAP_OPTION:
        Option_Gameplay_Draw(inv_item);
        break;
    case O_DETAIL_OPTION:
        Option_Graphics_Draw(inv_item);
        break;
    case O_CONTROL_OPTION:
        Option_Controls_Draw(inv_item);
        break;
    case O_COMPASS_OPTION:
        Option_Compass_Draw();
        break;
    case O_SOUND_OPTION:
        Option_Sound_Draw(inv_item);
        break;
    default:
        break;
    }
}
