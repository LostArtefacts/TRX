#include "game/option.h"

#include "game/input.h"
#include "game/option/option_compass.h"
#include "game/option/option_controls.h"
#include "game/option/option_controls_pick.h"
#include "game/option/option_examine.h"
#include "game/option/option_graphics.h"
#include "game/option/option_passport.h"
#include "game/option/option_sound.h"
#include "global/types.h"
#include "global/vars.h"

static CONTROL_MODE m_ControlMode = CM_PICK;

void Option_Shutdown(INVENTORY_ITEM *inv_item)
{
    switch (inv_item->object_id) {
    case O_PASSPORT_OPTION:
        Option_Passport_Shutdown();
        break;

    case O_COMPASS_OPTION:
        Option_Compass_Shutdown();
        break;

    case O_DETAIL_OPTION:
        Option_Graphics_Shutdown();
        break;

    case O_SOUND_OPTION:
        Option_Sound_Shutdown();
        break;

    case O_CONTROL_OPTION:
        if (m_ControlMode == CM_PICK) {
            Option_ControlsPick_Shutdown();
        } else {
            Option_Control_Shutdown();
        }
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
        Option_Examine_Shutdown();
        break;

    default:
        break;
    }
}

void Option_Control(INVENTORY_ITEM *inv_item, const bool is_busy)
{
    switch (inv_item->object_id) {
    case O_PASSPORT_OPTION:
        Option_Passport_Control(inv_item, is_busy);
        break;

    case O_COMPASS_OPTION:
        Option_Compass_Control(inv_item, is_busy);
        break;

    case O_DETAIL_OPTION:
        Option_Graphics_Control(inv_item, is_busy);
        break;

    case O_SOUND_OPTION:
        Option_Sound_Control(inv_item, is_busy);
        break;

    case O_CONTROL_OPTION:
        switch (m_ControlMode) {
        case CM_PICK:
            m_ControlMode = Option_ControlsPick_Control(is_busy);
            break;
        case CM_KEYBOARD:
            m_ControlMode = Option_Controls_Control(
                inv_item, is_busy, INPUT_BACKEND_KEYBOARD);
            break;
        case CM_CONTROLLER:
            m_ControlMode = Option_Controls_Control(
                inv_item, is_busy, INPUT_BACKEND_CONTROLLER);
            break;
        }
        break;

    case O_GAMMA_OPTION:
        // not implemented in TombATI
        break;

    case O_PISTOL_OPTION:
    case O_SHOTGUN_OPTION:
    case O_MAGNUM_OPTION:
    case O_UZI_OPTION:
    case O_EXPLOSIVE_OPTION:
    case O_MEDI_OPTION:
    case O_BIGMEDI_OPTION:
        if (!is_busy) {
            g_InputDB.menu_confirm = 1;
        }
        break;

    case O_PISTOL_AMMO_OPTION:
    case O_SG_AMMO_OPTION:
    case O_MAG_AMMO_OPTION:
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
        if (inv_item->action == ACTION_EXAMINE) {
            Option_Examine_Control(inv_item->object_id, is_busy);
        } else if (!is_busy) {
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

void Option_Draw(INVENTORY_ITEM *inv_item)
{
    switch (inv_item->object_id) {
    case O_CONTROL_OPTION:
        switch (m_ControlMode) {
        case CM_KEYBOARD:
            Option_Controls_Draw(inv_item, INPUT_BACKEND_KEYBOARD);
            break;
        case CM_CONTROLLER:
            Option_Controls_Draw(inv_item, INPUT_BACKEND_CONTROLLER);
            break;
        default:
            break;
        }
        break;

    case O_COMPASS_OPTION:
        Option_Compass_Draw();
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
        Option_Examine_Draw();
        break;

    default:
        break;
    }
}
