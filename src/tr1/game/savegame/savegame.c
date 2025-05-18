#include "game/savegame.h"

#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/gun/const.h>

int32_t Savegame_GetSlotCount(void)
{
    return g_Config.gameplay.maximum_save_slots;
}

void Savegame_HighlightNewestSlot(void)
{
    g_GameInfo.select_save_slot = Savegame_GetMostRecentlyCreatedSlot();
}

void Savegame_ApplyLogicToCurrentInfo(const GF_LEVEL *const level)
{
    RESUME_INFO *const current = Savegame_GetCurrentInfo(level);
    if (current == nullptr) {
        return;
    }

    LOG_INFO("Applying game logic to level #%d", level->num);

    if (!g_Config.gameplay.disable_healing_between_levels
        || level == GF_GetGymLevel() || level == GF_GetFirstLevel()) {
        current->lara_hitpoints = g_Config.gameplay.start_lara_hitpoints;
    }

    if (level == GF_GetGymLevel()) {
        current->flags.available = 1;
        current->flags.costume = 1;
        current->small_medipacks = 0;
        current->large_medipacks = 0;
        current->num_scions = 0;
        current->pistol_ammo = 0;
        current->shotgun_ammo = 0;
        current->magnum_ammo = 0;
        current->uzi_ammo = 0;
        current->flags.has_pistols = 0;
        current->flags.has_shotgun = 0;
        current->flags.has_magnums = 0;
        current->flags.has_uzis = 0;
        current->equipped_gun_type = LGT_UNARMED;
        current->holsters_gun_type = LGT_UNARMED;
        current->back_gun_type = LGT_UNARMED;
        current->gun_status = LGS_ARMLESS;
    }

    if (level == GF_GetFirstLevel()) {
        current->flags.available = 1;
        current->flags.costume = 0;
        current->small_medipacks = 0;
        current->large_medipacks = 0;
        current->num_scions = 0;
        current->pistol_ammo = 1000;
        current->shotgun_ammo = 0;
        current->magnum_ammo = 0;
        current->uzi_ammo = 0;
        current->flags.has_pistols = 1;
        current->flags.has_shotgun = 0;
        current->flags.has_magnums = 0;
        current->flags.has_uzis = 0;
        current->equipped_gun_type = LGT_PISTOLS;
        current->holsters_gun_type = LGT_PISTOLS;
        current->back_gun_type = LGT_UNARMED;
        current->gun_status = LGS_ARMLESS;
    }

    if (Game_IsBonusFlagSet(GBF_NGPLUS) && level != GF_GetGymLevel()) {
        current->flags.has_pistols = 1;
        current->flags.has_shotgun = 1;
        current->flags.has_magnums = 1;
        current->flags.has_uzis = 1;
        current->shotgun_ammo = 1234;
        current->magnum_ammo = 1234;
        current->uzi_ammo = 1234;
        current->equipped_gun_type = LGT_UZIS;
        current->holsters_gun_type = LGT_UZIS;
    }

    // Fallback logic to figure out holster and back gun items for versions 4.2
    // and earlier, as well as TombATI saves, where these values are missing.
    // Make educated guesses based on the type of gun equipped.
    if (current->holsters_gun_type == LGT_UNKNOWN) {
        switch (current->equipped_gun_type) {
        case LGT_PISTOLS:
        case LGT_MAGNUMS:
        case LGT_UZIS:
            current->holsters_gun_type = current->equipped_gun_type;
            break;
        case LGT_SHOTGUN:
            if (current->flags.has_pistols) {
                current->holsters_gun_type = LGT_PISTOLS;
            } else if (current->flags.has_magnums) {
                current->holsters_gun_type = LGT_MAGNUMS;
            } else if (current->flags.has_uzis) {
                current->holsters_gun_type = LGT_UZIS;
            } else {
                current->holsters_gun_type = LGT_UNARMED;
            }
            break;
        default:
            current->holsters_gun_type = LGT_UNARMED;
            break;
        }
    }
    if (current->back_gun_type == LGT_UNKNOWN) {
        if (current->flags.has_shotgun) {
            current->back_gun_type = LGT_SHOTGUN;
        } else {
            current->back_gun_type = LGT_UNARMED;
        }
    }
}
