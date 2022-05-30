#include "game/option.h"

#include "game/input.h"
#include "game/option/option_compass.h"
#include "game/option/option_control.h"
#include "game/option/option_graphics.h"
#include "game/option/option_passport.h"
#include "game/option/option_sound.h"
#include "global/types.h"

void Option_Init(void)
{
    Option_PassportInit();
}

void Option_Shutdown(void)
{
    Option_PassportShutdown();
}

void Option_DoInventory(INVENTORY_ITEM *inv_item)
{
    switch (inv_item->object_number) {
    case O_PASSPORT_OPTION:
        Option_Passport(inv_item);
        break;

    case O_MAP_OPTION:
        Option_Compass(inv_item);
        break;

    case O_DETAIL_OPTION:
        Option_Graphics(inv_item);
        break;

    case O_SOUND_OPTION:
        Option_Sound(inv_item);
        break;

    case O_CONTROL_OPTION:
        Option_Control(inv_item);
        break;

    case O_GAMMA_OPTION:
        // not implemented in TombATI
        break;

    case O_GUN_OPTION:
    case O_SHOTGUN_OPTION:
    case O_MAGNUM_OPTION:
    case O_UZI_OPTION:
    case O_EXPLOSIVE_OPTION:
    case O_MEDI_OPTION:
    case O_BIGMEDI_OPTION:
    case O_PUZZLE_OPTION1:
    case O_PUZZLE_OPTION2:
    case O_PUZZLE_OPTION3:
    case O_PUZZLE_OPTION4:
    case O_KEY_OPTION1:
    case O_KEY_OPTION2:
    case O_KEY_OPTION3:
    case O_KEY_OPTION4:
    case O_PICKUP_OPTION1:
    case O_PICKUP_OPTION2:
    case O_SCION_OPTION:
        g_InputDB.select = 1;
        break;

    case O_GUN_AMMO_OPTION:
    case O_SG_AMMO_OPTION:
    case O_MAG_AMMO_OPTION:
    case O_UZI_AMMO_OPTION:
        break;

    default:
        if (g_InputDB.deselect || g_InputDB.select) {
            inv_item->goal_frame = 0;
            inv_item->anim_direction = -1;
        }
        break;
    }
}
