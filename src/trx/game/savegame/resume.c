#include <trx/config.h>
#include <trx/core/enum_map.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/debug.h>
#include <trx/game/cutseq.h>
#include <trx/game/game.h>
#include <trx/game/game_flow.h>
#include <trx/game/gun.h>
#include <trx/game/gun/registry.h>
#include <trx/game/inventory.h>
#include <trx/game/lara.h>
#include <trx/game/lua/store.h>
#include <trx/game/objects.h>
#include <trx/game/rules.h>
#include <trx/game/savegame.h>
#include <trx/game/waypoint.h>
#include <trx/version.h>

#include <string.h>

// One entry per level of the main table, plus one the demos share.
static RESUME_INFO *m_ResumeInfo = nullptr;

static void M_CopyResumeInfo(
    RESUME_INFO *const target, const RESUME_INFO *const source)
{
    memcpy(target, source, sizeof(RESUME_INFO));
}

// What a level keeps for Lara's return is what the savegame names: her
// weapons and their ammunition, her supplies and her plot items. A key, a
// puzzle piece or a waterskin belongs to the level she found it in, and is
// left behind at its end.
static void M_PersistInventory(RESUME_INFO *const resume)
{
    const INVENTORY_STATE *const live = Inv_GetState();
    resume->inv = (INVENTORY_STATE) {};
    memcpy(resume->inv.ammo, live->ammo, sizeof(resume->inv.ammo));

    for (const SAVEGAME_RESUME_WEAPON *entry = g_Savegame_ResumeWeapons;
         entry->has_key != nullptr; entry++) {
        const OBJECT_ID gun_object = Gun_GetGunObject(entry->gun_type);
        Inv_State_SetCount(
            &resume->inv, gun_object, Inv_HasItem(gun_object) ? 1 : 0);
    }
    for (const SAVEGAME_RESUME_ITEM *entry = g_Savegame_ResumeItems;
         entry->key != nullptr; entry++) {
        Inv_State_SetCount(
            &resume->inv, entry->object_id, Inv_GetItemCount(entry->object_id));
    }
    Inv_State_SetCount(
        &resume->inv, O_BINOCULARS_ITEM,
        Inv_HasItem(O_BINOCULARS_ITEM) ? 1 : 0);
}

void SG_Resume_Init(void)
{
    m_ResumeInfo = Memory_Alloc(
        sizeof(RESUME_INFO)
        * (GF_GetLevelTable(GFLT_MAIN)->count
           + GF_GetLevelTable(GFLT_DEMOS)->count));

    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_DEMOS);
    for (int32_t i = 0; i < level_table->count; i++) {
        RESUME_INFO *const resume_info =
            SG_Resume_GetEntry(&level_table->levels[i]);
        resume_info->lara_hitpoints = LARA_MAX_HITPOINTS;
        resume_info->flags.available = true;
        Inv_State_SetCount(&resume_info->inv, O_PISTOL_ITEM, 1);
        resume_info->inv.ammo[LGT_PISTOLS] = Gun_GetInitialRounds(LGT_PISTOLS);
        resume_info->gun_status = LGS_ARMLESS;
        resume_info->equipped_gun_type = LGT_PISTOLS;
        resume_info->holsters_gun_type = LGT_PISTOLS;
        resume_info->back_gun_type = LGT_UNARMED;
        resume_info->prev_level = -1;
    }
}

void SG_Resume_Shutdown(void)
{
    Memory_FreePointer(&m_ResumeInfo);
}

RESUME_INFO *SG_Resume_GetEntry(const GF_LEVEL *const level)
{
    ASSERT(m_ResumeInfo != nullptr);
    if (level == nullptr) {
        return nullptr;
    }
    if (GF_GetLevelTableType(level->type) == GFLT_MAIN) {
        return &m_ResumeInfo[level->num];
    } else if (level->type == GFL_DEMO) {
        return &m_ResumeInfo[GF_GetLevelTable(GFLT_MAIN)->count];
    } else if (level->type == GFL_CUTSCENE || level->type == GFL_TITLE) {
        return nullptr;
    }
    LOG_WARNING(
        "Warning: unable to get resume info for level %d (type=%s)", level->num,
        ENUM_MAP_TO_STRING(GF_LEVEL_TYPE, level->type));
    return nullptr;
}

void SG_Resume_MirrorCurrentEntry(const GF_LEVEL *const level)
{
    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < level_table->count; i++) {
        if (level_table->levels[i].type == GFL_CURRENT) {
            m_ResumeInfo[i] = m_ResumeInfo[level->num];
        }
    }
}

void SG_Resume_ResetAllEntries(void)
{
    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < level_table->count; i++) {
        const GF_LEVEL *const level = &level_table->levels[i];
        SG_Resume_ResetEntry(level);
        SG_Resume_ApplyRulesToEntry(level);
        RESUME_INFO *const current = SG_Resume_GetEntry(level);
        current->level_completed = false;
        current->flags.available = false;
    }

    if (GF_GetGymLevel() != nullptr) {
        SG_Resume_GetEntry(GF_GetGymLevel())->flags.available = true;
    }
    if (GF_GetFirstLevel() != nullptr) {
        SG_Resume_GetEntry(GF_GetFirstLevel())->flags.available = true;
    }

    // Lara's wetness survives level transitions; a fresh playthrough starts
    // dry.
    LARA_INFO *const lara = Lara_GetLaraInfo();
    memset(lara->wet, 0, sizeof(lara->wet));

    // The rules last as long as the playthrough too; a load restores the saved
    // ones over these once the file is read.
    Rules_Reset();

    // Which cutscenes have run lasts as long as the playthrough as well, so a
    // fresh one sees all of them again.
    CutSeq_SetPlayedMask(0);

    // And so does how far Lara has got along a level's own progression.
    Waypoint_Reset();

    // The game store spans a playthrough, so a fresh one starts empty.
    LUA_Store_ClearGame();
}

void SG_Resume_ResetEntry(const GF_LEVEL *const level)
{
    LOG_INFO("Resetting resume info for level #%d", level->num);
    RESUME_INFO *const current = SG_Resume_GetEntry(level);
    *current = (RESUME_INFO) { .prev_level = -1, .level_completed = false };
}

void SG_Resume_CarryEntry(
    const GF_LEVEL *const src_level, const GF_LEVEL *const dst_level)
{
    LOG_INFO(
        "Copying resume info from level #%d to level #%d", src_level->num,
        dst_level->num);
    RESUME_INFO *const src_resume = SG_Resume_GetEntry(src_level);
    RESUME_INFO *const dst_resume = SG_Resume_GetEntry(dst_level);
    if (src_resume != nullptr && dst_resume != nullptr) {
        const bool dst_level_completed = dst_resume->level_completed;
        M_CopyResumeInfo(dst_resume, src_resume);
        dst_resume->level_completed = dst_level_completed;
        dst_resume->prev_level = src_level->num;
    }
}

void SG_Resume_StoreGameToEntry(const GF_LEVEL *const level)
{
    RESUME_INFO *const resume = SG_Resume_GetEntry(level);
    if (resume == nullptr) {
        return;
    }

    resume->flags.available = true;

    LARA_INFO *const lara = Lara_GetLaraInfo();
    const ITEM *const lara_item = Lara_GetItem();

    if (lara_item != nullptr) {
        resume->lara_hitpoints = lara_item->hit_points;
    }
    resume->burning = g_TRVersion >= 4 && lara->burn;
    M_PersistInventory(resume);

    resume->equipped_gun_type = lara->last_gun_type;
    resume->holsters_gun_type = lara->holsters_gun_type;
    resume->back_gun_type = lara->back_gun_type;
    if (resume->back_gun_type == LGT_UNARMED
        && Gun_IsRifleType(resume->equipped_gun_type)
        && Inv_HasItem(Gun_GetGunObject(resume->equipped_gun_type))) {
        // If a rifle is currently drawn, Lara's back mesh is temporarily
        // unarmed. Preserve the preferred rifle for next-level mesh restore.
        resume->back_gun_type = resume->equipped_gun_type;
    }
    if (lara->gun_status == LGS_READY) {
        resume->gun_status = LGS_READY;
    } else {
        resume->gun_status = LGS_ARMLESS;
    }
}

void SG_Resume_ApplyRulesToEntry(const GF_LEVEL *const level)
{
    RESUME_INFO *const resume = SG_Resume_GetEntry(level);
    if (resume == nullptr) {
        return;
    }

    LOG_INFO("Applying game logic to level #%d", level->num);

    if (!g_Config.gameplay.disable_healing_between_levels
        || level == GF_GetGymLevel() || level == GF_GetFirstLevel()) {
        resume->lara_hitpoints = g_Config.gameplay.start_lara_hitpoints;
    }

    if (level == GF_GetGymLevel()) {
        resume->flags.available = true;
        resume->flags.costume = g_TRVersion == 1;

        // The gym is a house tour: she arrives with nothing.
        resume->inv = (INVENTORY_STATE) {};

        resume->equipped_gun_type = LGT_UNARMED;
        resume->holsters_gun_type = LGT_UNARMED;
        resume->back_gun_type = LGT_UNARMED;
        resume->gun_status = LGS_ARMLESS;
    }

    if (level == GF_GetFirstLevel()) {
        resume->flags.available = true;
        resume->flags.costume = false;

        // She starts the game with her pistols and nothing else.
        resume->inv = (INVENTORY_STATE) {};
        Inv_State_SetCount(&resume->inv, O_PISTOL_ITEM, 1);
        resume->inv.ammo[LGT_PISTOLS] = Gun_GetInitialRounds(LGT_PISTOLS);

        resume->equipped_gun_type = LGT_PISTOLS;
        resume->holsters_gun_type = LGT_PISTOLS;
        resume->back_gun_type = LGT_UNARMED;
        resume->gun_status = LGS_ARMLESS;
    }

    if (Game_IsBonusFlagSet(GBF_NGPLUS) && level != GF_GetGymLevel()) {
        // A bonus game hands her every weapon the game has, loaded.
        for (LARA_GUN_TYPE gun_type = LGT_UNARMED + 1; gun_type < NUM_WEAPONS;
             gun_type++) {
            const OBJECT_ID gun_object = Gun_GetGunObject(gun_type);
            if (gun_object == NO_OBJECT
                || !Gun_Registry_Get(gun_type)->is_available) {
                continue;
            }
            Inv_State_SetCount(&resume->inv, gun_object, 1);
            resume->inv.ammo[gun_type] = gun_type == LGT_PISTOLS
                ? Gun_GetInitialRounds(LGT_PISTOLS)
                : 10000;
        }
        if (g_TRVersion > 1) {
            Inv_State_SetCount(&resume->inv, O_FLARE_ITEM, MAX_QTY);
        }

        const bool should_force_ngplus_gun_setup =
            !g_Config.gameplay.remember_gun_status || resume->prev_level == -1;
        if (should_force_ngplus_gun_setup) {
            switch (g_TRVersion) {
            case 1:
                resume->equipped_gun_type = LGT_UZIS;
                resume->back_gun_type = LGT_SHOTGUN;
                resume->holsters_gun_type = LGT_UZIS;
                break;
            case 2:
                resume->equipped_gun_type = LGT_GRENADE;
                resume->back_gun_type = LGT_GRENADE;
                resume->holsters_gun_type = LGT_PISTOLS;
                break;
            case 3:
            default:
                resume->equipped_gun_type = LGT_ROCKET;
                resume->back_gun_type = LGT_ROCKET;
                resume->holsters_gun_type = LGT_PISTOLS;
                break;
            }
        }
    }

    resume->stats.secret_flags = 0;
}

int32_t SG_Resume_CountCompletedLevels(void)
{
    int32_t count = 0;
    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < level_table->count; i++) {
        const GF_LEVEL *const level = &level_table->levels[i];
        const RESUME_INFO *const current = SG_Resume_GetEntry(level);
        if (current != nullptr && current->level_completed) {
            count++;
        }
    }
    return count;
}
