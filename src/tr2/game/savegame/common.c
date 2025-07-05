#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/savegame.h"
#include "global/types_decomp.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/gun/const.h>
#include <libtrx/utils.h>

// TODO: make configurable (legacy MAX_REQUESTER_ITEMS)
#define MAX_SAVE_SLOTS 24

int32_t Savegame_GetSlotCount(void)
{
    return MAX_SAVE_SLOTS;
}

void Savegame_HighlightNewestSlot(void)
{
}

void Savegame_ApplyLogicToCurrentInfo(const GF_LEVEL *const level)
{
    RESUME_INFO *resume = Savegame_GetCurrentInfo(level);
    if (resume == nullptr) {
        return;
    }

    if (!g_Config.gameplay.disable_healing_between_levels
        || level == GF_GetGymLevel() || level == GF_GetFirstLevel()) {
        resume->lara_hitpoints = g_Config.gameplay.start_lara_hitpoints;
    }

    resume->flags.has_pistols = true;
    resume->equipped_gun_type = LGT_PISTOLS;
    resume->pistol_ammo = 1000;

    if (level == GF_GetGymLevel()) {
        resume->flags.available = true;

        resume->flags.has_pistols = false;
        resume->flags.has_shotgun = false;
        resume->flags.has_magnums = false;
        resume->flags.has_uzis = false;
        resume->flags.has_harpoon = false;
        resume->flags.has_m16 = false;
        resume->flags.has_grenade = false;

        resume->pistol_ammo = 0;
        resume->shotgun_ammo = 0;
        resume->magnum_ammo = 0;
        resume->uzi_ammo = 0;
        resume->harpoon_ammo = 0;
        resume->m16_ammo = 0;
        resume->grenade_ammo = 0;

        resume->flares = 0;
        resume->large_medipacks = 0;
        resume->small_medipacks = 0;
        resume->equipped_gun_type = LGT_UNARMED;
        resume->gun_status = LGS_ARMLESS;
    } else if (level == GF_GetFirstLevel()) {
        resume->flags.available = true;

        resume->flags.has_pistols = true;
        resume->flags.has_shotgun = false;
        resume->flags.has_magnums = false;
        resume->flags.has_uzis = false;
        resume->flags.has_harpoon = false;
        resume->flags.has_m16 = false;
        resume->flags.has_grenade = false;

        resume->shotgun_ammo = 0;
        resume->magnum_ammo = 0;
        resume->uzi_ammo = 0;
        resume->harpoon_ammo = 0;
        resume->m16_ammo = 0;
        resume->grenade_ammo = 0;

        resume->flares = 0;
        resume->small_medipacks = 0;
        resume->large_medipacks = 0;
        resume->gun_status = LGS_ARMLESS;
    }

    if (Game_IsBonusFlagSet(GBF_NGPLUS) && level != GF_GetGymLevel()) {
        resume->flags.has_pistols = true;
        resume->flags.has_shotgun = true;
        resume->flags.has_magnums = true;
        resume->flags.has_uzis = true;
        resume->flags.has_grenade = true;
        resume->flags.has_harpoon = true;
        resume->flags.has_m16 = true;
        resume->flags.has_grenade = true;

        resume->shotgun_ammo = 10000;
        resume->magnum_ammo = 10000;
        resume->uzi_ammo = 10000;
        resume->harpoon_ammo = 10000;
        resume->m16_ammo = 10000;
        resume->grenade_ammo = 10000;

        resume->flares = -1;
        resume->equipped_gun_type = LGT_GRENADE;
    }

    const STATS_COMMON default_stats = Savegame_GetDefaultStats(level);
    resume->stats.max_secret_count = default_stats.max_secret_count;
    resume->stats.all_secrets_mask = default_stats.all_secrets_mask;
}
