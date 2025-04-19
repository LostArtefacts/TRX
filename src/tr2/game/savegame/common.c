#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/requester.h"
#include "game/savegame.h"
#include "global/types_decomp.h"
#include "global/vars.h"

#include <libtrx/debug.h>
#include <libtrx/game/gun/const.h>
#include <libtrx/utils.h>

// TODO: make configurable
#define MAX_SAVE_SLOTS MAX_REQUESTER_ITEMS

static uint32_t m_ReqFlags1[MAX_REQUESTER_ITEMS] = {};
static uint32_t m_ReqFlags2[MAX_REQUESTER_ITEMS] = {};

int32_t Savegame_GetSlotCount(void)
{
    return MAX_SAVE_SLOTS;
}

void Savegame_HighlightNewestSlot(void)
{
    const int32_t slot = Savegame_GetMostRecentlyCreatedSlot();
    g_SaveGameRequester.selected = MAX(0, slot);
    g_LoadGameRequester.selected = MAX(0, slot);
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

void Savegame_FillAvailableSaves(REQUEST_INFO *const req)
{
    Requester_Init(req);

    for (int32_t i = 0; i < MAX_SAVE_SLOTS; i++) {
        const SAVEGAME_INFO *const savegame_info = Savegame_GetSavegameInfo(i);
        if (savegame_info->level_title != nullptr) {
            char save_num_text[16];
            sprintf(save_num_text, "%d", savegame_info->counter);
            Requester_AddItem(
                req, savegame_info->level_title, REQ_ALIGN_LEFT, save_num_text,
                REQ_ALIGN_RIGHT);
        } else {
            Requester_AddItem(req, GS(MISC_EMPTY_SLOT), 0, 0, 0);
        }
    }

    Requester_SetSize(req, 10, -32);
    if (req->selected >= req->visible_count) {
        req->line_offset = req->selected - req->visible_count + 1;
    } else if (req->selected < req->line_offset) {
        req->line_offset = req->selected;
    }
    memcpy(m_ReqFlags1, g_RequesterFlags1, sizeof(m_ReqFlags1));
    memcpy(m_ReqFlags2, g_RequesterFlags2, sizeof(m_ReqFlags2));
}

void Savegame_FillAvailableLevels(REQUEST_INFO *const req)
{
    ASSERT(req != nullptr);
    Requester_Init(req);
    Requester_SetSize(req, 10, -32);
    Requester_SetHeading(req, GS(PASSPORT_SELECT_LEVEL), 0, nullptr, 0);
    req->ready = true;
    req->selected = 0;

    Requester_RemoveAllItems(req);
    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < level_table->count; i++) {
        const GF_LEVEL *const level = &level_table->levels[i];
        if (level->type != GFL_GYM) {
            Requester_AddItem(req, level->title, 0, nullptr, 0);
        }
    }
}
