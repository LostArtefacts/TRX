#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/savegame.h"
#include "global/types_decomp.h"
#include "global/vars.h"

#include <libtrx/debug.h>
#include <libtrx/game/gun/const.h>
#include <libtrx/utils.h>

// TODO: make configurable
#define MAX_SAVE_SLOTS MAX_REQUESTER_ITEMS

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

    resume->flags.has_pistols = 1;
    resume->equipped_gun_type = LGT_PISTOLS;
    resume->pistol_ammo = 1000;

    if (level == GF_GetGymLevel()) {
        resume->flags.available = 1;

        resume->flags.has_pistols = 0;
        resume->flags.has_shotgun = 0;
        resume->flags.has_magnums = 0;
        resume->flags.has_uzis = 0;
        resume->flags.has_harpoon = 0;
        resume->flags.has_m16 = 0;
        resume->flags.has_grenade = 0;

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
        resume->flags.available = 1;

        resume->flags.has_pistols = 1;
        resume->flags.has_shotgun = 1;
        resume->flags.has_magnums = 0;
        resume->flags.has_uzis = 0;
        resume->flags.has_harpoon = 0;
        resume->flags.has_m16 = 0;
        resume->flags.has_grenade = 0;

        resume->shotgun_ammo = 2 * SHOTGUN_AMMO_CLIP;
        resume->magnum_ammo = 0;
        resume->uzi_ammo = 0;
        resume->harpoon_ammo = 0;
        resume->m16_ammo = 0;
        resume->grenade_ammo = 0;

        resume->flares = 2;
        resume->small_medipacks = 1;
        resume->large_medipacks = 1;
        resume->gun_status = LGS_ARMLESS;
    }

    if (Game_IsBonusFlagSet(GBF_NGPLUS) && level != GF_GetGymLevel()) {
        resume->flags.has_pistols = 1;
        resume->flags.has_shotgun = 1;
        resume->flags.has_magnums = 1;
        resume->flags.has_uzis = 1;
        resume->flags.has_grenade = 1;
        resume->flags.has_harpoon = 1;
        resume->flags.has_m16 = 1;
        resume->flags.has_grenade = 1;

        resume->shotgun_ammo = 10000;
        resume->magnum_ammo = 10000;
        resume->uzi_ammo = 10000;
        resume->harpoon_ammo = 10000;
        resume->m16_ammo = 10000;
        resume->grenade_ammo = 10000;

        resume->flares = -1;
        resume->equipped_gun_type = LGT_GRENADE;
    }

    if (g_GF_RemoveWeapons) {
        resume->flags.has_pistols = 0;
        resume->flags.has_magnums = 0;
        resume->flags.has_uzis = 0;
        resume->flags.has_shotgun = 0;
        resume->flags.has_m16 = 0;
        resume->flags.has_grenade = 0;
        resume->flags.has_harpoon = 0;
        resume->equipped_gun_type = LGT_UNARMED;
        resume->gun_status = LGS_ARMLESS;
        g_GF_RemoveWeapons = false;
    }

    if (g_GF_RemoveAmmo) {
        resume->m16_ammo = 0;
        resume->grenade_ammo = 0;
        resume->harpoon_ammo = 0;
        resume->shotgun_ammo = 0;
        resume->uzi_ammo = 0;
        resume->magnum_ammo = 0;
        resume->pistol_ammo = 0;
        resume->flares = 0;
        resume->large_medipacks = 0;
        resume->small_medipacks = 0;
        g_GF_RemoveAmmo = false;
    }

    const STATS_COMMON default_stats = Savegame_GetDefaultStats(level);
    resume->stats.max_secret_count = default_stats.max_secret_count;
}
