#include "game/savegame.h"

#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/requester.h"
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
    g_SavegameRequester.requested = MAX(0, Savegame_GetHighestSlot());
}

void Savegame_ApplyLogicToCurrentInfo(const GF_LEVEL *const level)
{
    RESUME_INFO *const current = Savegame_GetCurrentInfo(level);
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

void Savegame_FillAvailableSaves(REQUEST_INFO *const req)
{
    Requester_ClearTextstrings(req);
    Requester_Init(req, Savegame_GetSlotCount());

    for (int32_t i = 0; i < req->max_items; i++) {
        const SAVEGAME_INFO *const savegame_info = Savegame_GetSavegameInfo(i);

        if (savegame_info->level_title != nullptr) {
            Requester_AddItem(
                req, false, "%s %d", savegame_info->level_title,
                savegame_info->counter);
        } else {
            Requester_AddItem(req, true, GS(MISC_EMPTY_SLOT_FMT), i + 1);
        }
    }

    if (req->requested >= req->vis_lines) {
        req->line_offset = req->requested - req->vis_lines + 1;
    } else if (req->requested < req->line_offset) {
        req->line_offset = req->requested;
    }
}

void Savegame_FillAvailableLevels(REQUEST_INFO *const req)
{
    ASSERT(req != nullptr);
    const int32_t slot_num = g_GameInfo.select_save_slot;
    if (slot_num == -1) {
        return;
    }

    const SAVEGAME_INFO *const savegame_info =
        Savegame_GetSavegameInfo(slot_num);
    if (!savegame_info->features.select_level) {
        Requester_AddItem(req, true, "%s", GS(PASSPORT_LEGACY_SELECT_LEVEL_1));
        Requester_AddItem(req, true, "%s", GS(PASSPORT_LEGACY_SELECT_LEVEL_2));
        req->requested = 0;
        req->line_offset = 0;
        return;
    }

    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i <= MIN(savegame_info->level_num, level_table->count);
         i++) {
        const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, i);
        if (level->type != GFL_GYM) {
            Requester_AddItem(req, false, "%s", level->title);
        }
    }

    if (g_InvMode == INV_TITLE_MODE) {
        Requester_AddItem(req, false, "%s", GS(PASSPORT_STORY_SO_FAR));
    }

    req->requested = 0;
    req->line_offset = 0;
}
