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
        current->flags.available = true;
        current->flags.costume = true;
        current->small_medipacks = 0;
        current->large_medipacks = 0;
        current->num_scions = 0;
        current->pistol_ammo = 0;
        current->shotgun_ammo = 0;
        current->magnum_ammo = 0;
        current->uzi_ammo = 0;
        current->flags.has_pistols = false;
        current->flags.has_shotgun = false;
        current->flags.has_magnums = false;
        current->flags.has_uzis = false;
        current->equipped_gun_type = LGT_UNARMED;
        current->holsters_gun_type = LGT_UNARMED;
        current->back_gun_type = LGT_UNARMED;
        current->gun_status = LGS_ARMLESS;
    }

    if (level == GF_GetFirstLevel()) {
        current->flags.available = true;
        current->flags.costume = false;
        current->small_medipacks = 0;
        current->large_medipacks = 0;
        current->num_scions = 0;
        current->pistol_ammo = 1000;
        current->shotgun_ammo = 0;
        current->magnum_ammo = 0;
        current->uzi_ammo = 0;
        current->flags.has_pistols = true;
        current->flags.has_shotgun = false;
        current->flags.has_magnums = false;
        current->flags.has_uzis = false;
        current->equipped_gun_type = LGT_PISTOLS;
        current->holsters_gun_type = LGT_PISTOLS;
        current->back_gun_type = LGT_UNARMED;
        current->gun_status = LGS_ARMLESS;
    }

    if (Game_IsBonusFlagSet(GBF_NGPLUS) && level != GF_GetGymLevel()) {
        current->flags.has_pistols = true;
        current->flags.has_shotgun = true;
        current->flags.has_magnums = true;
        current->flags.has_uzis = true;
        current->shotgun_ammo = 1234;
        current->magnum_ammo = 1234;
        current->uzi_ammo = 1234;
        current->equipped_gun_type = LGT_UZIS;
        current->holsters_gun_type = LGT_UZIS;
        current->back_gun_type = LGT_SHOTGUN;
    }

    Savegame_DetermineLegacyGunTypes(current);
}
