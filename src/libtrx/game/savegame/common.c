#include "benchmark.h"
#include "config.h"
#include "debug.h"
#include "enum_map.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/gun/const.h"
#include "game/inventory.h"
#include "game/lara.h"
#include "game/objects.h"
#include "game/objects/traps/movable_block.h"
#include "game/pathing/lot.h"
#include "game/savegame.h"
#include "memory.h"
#include "strings.h"

#define MAX_STRATEGIES 2
#define SAVES_DIR "saves"

static SAVEGAME_VERSION m_InitialVersion = VERSION_LEGACY;
static SAVEGAME_INFO *m_SavegameInfo = nullptr;
static RESUME_INFO *m_ResumeInfo = nullptr;
static STATS_COMMON *m_DefaultStats = nullptr;
static int32_t m_SaveSlots = 0;
static int32_t m_SavedGames = 0;
static int32_t m_SaveCounter = 0;
static int32_t m_MostRecentlyUsedSlot = -1;
static int32_t m_MostRecentlyCreatedSlot = -1;
static int32_t m_BoundSlot = -1;

static int32_t m_StrategyCount = 0;
static SAVEGAME_STRATEGY m_Strategies[MAX_STRATEGIES];

static void M_ClearSlot(SAVEGAME_INFO *const savegame_info)
{
    savegame_info->format = SAVEGAME_FORMAT_INVALID;
    savegame_info->counter = -1;
    savegame_info->level_num = -1;
    Memory_FreePointer(&savegame_info->full_path);
    Memory_FreePointer(&savegame_info->level_title);
}

static void M_ClearSlots(void)
{
    if (m_SavegameInfo == nullptr) {
        return;
    }

    for (int32_t i = 0; i < m_SaveSlots; i++) {
        M_ClearSlot(&m_SavegameInfo[i]);
    }
}

static bool M_FillSlot(
    const SAVEGAME_STRATEGY strategy, const int32_t slot_num,
    const char *const path)
{
    SAVEGAME_INFO *const savegame_info = &m_SavegameInfo[slot_num];
    if (strategy.format <= savegame_info->format) {
        return true;
    }

    bool result = false;
    MYFILE *const fp = File_Open(path, FILE_OPEN_READ);
    if (fp != nullptr) {
        SAVEGAME_INFO tmp_savegame_info;
        if (strategy.fill_info_func(fp, &tmp_savegame_info)) {
            M_ClearSlot(savegame_info);
            *savegame_info = tmp_savegame_info;
            savegame_info->format = strategy.format;
            savegame_info->full_path = Memory_DupStr(path);
            result = true;
        }
        File_Close(fp);
    }
    return result;
}

static void M_ScanSavedGamesDir(const char *const dir_path)
{
    void *const dir_handle = File_OpenDirectory(dir_path);
    if (dir_handle == nullptr) {
        return;
    }

    while (true) {
        const char *const file_name = File_ReadDirectory(dir_handle);
        if (file_name == nullptr) {
            break;
        }
        if (strcmp(file_name, ".") == 0 || strcmp(file_name, "..") == 0) {
            continue;
        }

        char *file_name_ci = String_ToUpper(file_name);
        for (int32_t i = 0; i < m_StrategyCount; i++) {
            const SAVEGAME_STRATEGY strategy = m_Strategies[i];
            if (!strategy.allow_load) {
                continue;
            }

            const char *const pattern = strategy.get_save_file_pattern_func();
            char *pattern_ci = String_ToUpperPattern(pattern);

            int32_t slot = -1;
            const int32_t parsed = sscanf(file_name_ci, pattern_ci, &slot);
            Memory_FreePointer(&pattern_ci);

            if (parsed == 1 && slot >= 0 && slot < m_SaveSlots) {
                char *file_path = String_Format("%s/%s", dir_path, file_name);
                M_FillSlot(strategy, slot, file_path);
                Memory_FreePointer(&file_path);
            }
        }
        Memory_FreePointer(&file_name_ci);
    }

    File_CloseDirectory(dir_handle);
}

static void M_LoadPreprocess(void)
{
    Savegame_InitCurrentInfo();
}

static void M_LoadPostprocess(void)
{
    // TODO: tidy this; skidoo drivers currently require handle_save_func to be
    // called immediately on load within the strategies.
    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        ITEM *const item = Item_Get(i);
        const OBJECT *const obj = Object_Get(item->object_id);

        if (obj->save_position && obj->shadow_size) {
            int16_t room_num = item->room_num;
            const SECTOR *const sector = Room_GetSector(
                item->pos.x, item->pos.y, item->pos.z, &room_num);
            item->floor =
                Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
        }

        if (obj->save_flags != 0) {
            item->flags &= 0xFF00;
        }
#if TR_VERSION == 1
        if (obj->handle_save_func != nullptr) {
            obj->handle_save_func(item, SAVEGAME_STAGE_AFTER_LOAD);
        }
#endif
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
#if TR_VERSION == 1
    if (Game_GetBonusFlag() != GBF_NONE) {
        g_Config.profile.new_game_plus_unlock = true;
        Config_Update();
    }
    LOT_ClearLOT(&lara->lot);
#endif
    if (lara->burn && !g_Config.gameplay.enable_enhanced_saves) {
        lara->burn = false;
        Lara_CatchFire();
    }
}

static void M_DetermineLegacyGunTypes(RESUME_INFO *const resume)
{
    // Fallback logic to figure out holster and back gun items for saves from
    // TR1X 4.2 and earlier (including TombATI) and TR2X 1.2 and earlier, where
    // these values are missing. Make educated guesses based on the type of gun
    // equipped.
    if (resume->holsters_gun_type == LGT_UNKNOWN) {
        switch (resume->equipped_gun_type) {
        case LGT_PISTOLS:
        case LGT_MAGNUMS:
        case LGT_UZIS:
            resume->holsters_gun_type = resume->equipped_gun_type;
            break;
        case LGT_SHOTGUN:
#if TR_VERSION >= 2
        case LGT_M16:
        case LGT_GRENADE:
        case LGT_HARPOON:
#endif
            if (resume->flags.has_pistols) {
                resume->holsters_gun_type = LGT_PISTOLS;
            } else if (resume->flags.has_magnums) {
                resume->holsters_gun_type = LGT_MAGNUMS;
            } else if (resume->flags.has_uzis) {
                resume->holsters_gun_type = LGT_UZIS;
            } else {
                resume->holsters_gun_type = LGT_UNARMED;
            }
            break;
        default:
            resume->holsters_gun_type = LGT_UNARMED;
            break;
        }
    }
    if (resume->back_gun_type == LGT_UNKNOWN) {
        resume->back_gun_type = LGT_UNARMED;
        if (resume->flags.has_shotgun) {
            resume->back_gun_type = LGT_SHOTGUN;
        }
#if TR_VERSION >= 2
        else if (resume->flags.has_m16) {
            resume->back_gun_type = LGT_M16;
        } else if (resume->flags.has_grenade) {
            resume->back_gun_type = LGT_GRENADE;
        } else if (resume->flags.has_harpoon) {
            resume->back_gun_type = LGT_HARPOON;
        }
#endif
    }
}

SAVEGAME_VERSION Savegame_GetInitialVersion(void)
{
    return m_InitialVersion;
}

void Savegame_SetInitialVersion(const SAVEGAME_VERSION version)
{
    m_InitialVersion = version;
}

void Savegame_BindSlot(const int32_t slot_num)
{
    m_BoundSlot = slot_num;
    m_MostRecentlyUsedSlot = slot_num;
    LOG_DEBUG("Binding save slot %d", slot_num);
}

int32_t Savegame_GetMostRecentlyUsedSlot(void)
{
    return m_MostRecentlyUsedSlot;
}

void Savegame_UnbindSlot(void)
{
    LOG_DEBUG("Resetting the save slot");
    m_BoundSlot = -1;
}

int32_t Savegame_GetBoundSlot(void)
{
    return m_BoundSlot;
}

int32_t Savegame_GetLevelNumber(const int32_t slot_num)
{
    return m_SavegameInfo[slot_num].level_num;
}

bool Savegame_IsSlotFree(const int32_t slot_num)
{
    return m_SavegameInfo[slot_num].level_num == -1;
}

int32_t Savegame_GetCounter(void)
{
    return m_SaveCounter;
}

int32_t Savegame_GetTotalCount(void)
{
    return m_SavedGames;
}

int32_t Savegame_GetMostRecentlyCreatedSlot(void)
{
    return m_MostRecentlyCreatedSlot;
}

bool Savegame_RestartAvailable(const int32_t slot_num)
{
#if TR_VERSION == 1
    if (slot_num == -1) {
        return true;
    }

    const SAVEGAME_INFO *const savegame_info = &m_SavegameInfo[slot_num];
    return savegame_info->features.restart;
#else
    return false;
#endif
}

void Savegame_RegisterStrategy(const SAVEGAME_STRATEGY strategy)
{
    ASSERT(m_StrategyCount < MAX_STRATEGIES);
    m_Strategies[m_StrategyCount] = strategy;
    m_StrategyCount++;
}

void Savegame_Init(void)
{
    m_ResumeInfo = Memory_Alloc(
        sizeof(RESUME_INFO)
        * (GF_GetLevelTable(GFLT_MAIN)->count
           + GF_GetLevelTable(GFLT_DEMOS)->count));

    m_SaveSlots = Savegame_GetSlotCount();
    m_SavegameInfo = Memory_Alloc(sizeof(SAVEGAME_INFO) * m_SaveSlots);

    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_DEMOS);
    for (int32_t i = 0; i < level_table->count; i++) {
        RESUME_INFO *const resume_info =
            Savegame_GetCurrentInfo(&level_table->levels[i]);
        resume_info->lara_hitpoints = LARA_MAX_HITPOINTS;
        resume_info->flags.available = true;
        resume_info->flags.has_pistols = true;
        resume_info->pistol_ammo = 1000;
        resume_info->gun_status = LGS_ARMLESS;
        resume_info->equipped_gun_type = LGT_PISTOLS;
        resume_info->holsters_gun_type = LGT_PISTOLS;
        resume_info->back_gun_type = LGT_UNARMED;
    }
}

bool Savegame_IsInitialised(void)
{
    return m_SavegameInfo != nullptr;
}

void Savegame_Shutdown(void)
{
    M_ClearSlots();
    Memory_FreePointer(&m_ResumeInfo);
    Memory_FreePointer(&m_SavegameInfo);
    Memory_FreePointer(&m_DefaultStats);
}

RESUME_INFO *Savegame_GetCurrentInfo(const GF_LEVEL *const level)
{
    ASSERT(m_ResumeInfo != nullptr);
    ASSERT(level != nullptr);
    if (GF_GetLevelTableType(level->type) == GFLT_MAIN) {
        return &m_ResumeInfo[level->num];
    } else if (level->type == GFL_DEMO) {
        return &m_ResumeInfo[GF_GetLevelTable(GFLT_MAIN)->count];
    }
    LOG_WARNING(
        "Warning: unable to get resume info for level %d (type=%s)", level->num,
        ENUM_MAP_TO_STRING(GF_LEVEL_TYPE, level->type));
    return nullptr;
}

void Savegame_SetCurrentInfo(const int32_t current_slot, const int32_t src_slot)
{
    m_ResumeInfo[current_slot] = m_ResumeInfo[src_slot];
}

const SAVEGAME_INFO *Savegame_GetSavegameInfo(const int32_t slot_num)
{
    return &m_SavegameInfo[slot_num];
}

void Savegame_InitCurrentInfo(void)
{
    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < level_table->count; i++) {
        const GF_LEVEL *const level = &level_table->levels[i];
        Savegame_ResetCurrentInfo(level);
        Savegame_ApplyLogicToCurrentInfo(level);
        Savegame_GetCurrentInfo(level)->flags.available = false;
    }

    if (GF_GetGymLevel() != nullptr) {
        Savegame_GetCurrentInfo(GF_GetGymLevel())->flags.available = true;
    }
    if (GF_GetFirstLevel() != nullptr) {
        Savegame_GetCurrentInfo(GF_GetFirstLevel())->flags.available = true;
    }
}

void Savegame_ResetCurrentInfo(const GF_LEVEL *const level)
{
    LOG_INFO("Resetting resume info for level #%d", level->num);
    RESUME_INFO *const current = Savegame_GetCurrentInfo(level);
    memset(current, 0, sizeof(RESUME_INFO));
}

void Savegame_CarryCurrentInfoToNextLevel(
    const GF_LEVEL *const src_level, const GF_LEVEL *const dst_level)
{
    LOG_INFO(
        "Copying resume info from level #%d to level #%d", src_level->num,
        dst_level->num);
    RESUME_INFO *const src_resume = Savegame_GetCurrentInfo(src_level);
    RESUME_INFO *const dst_resume = Savegame_GetCurrentInfo(dst_level);
    memcpy(dst_resume, src_resume, sizeof(RESUME_INFO));
}

void Savegame_PersistGameToCurrentInfo(const GF_LEVEL *const level)
{
    RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const ITEM *const lara_item = Lara_GetItem();

    if (lara_item != nullptr) {
        resume->lara_hitpoints = lara_item->hit_points;
    }
    resume->flags.available = true;
    resume->small_medipacks = Inv_RequestItem(O_SMALL_MEDIPACK_ITEM);
    resume->large_medipacks = Inv_RequestItem(O_LARGE_MEDIPACK_ITEM);

    resume->pistol_ammo = 1000;
    if (Inv_RequestItem(O_PISTOL_ITEM)) {
        resume->flags.has_pistols = true;
    } else {
        resume->flags.has_pistols = false;
    }

    if (Inv_RequestItem(O_SHOTGUN_ITEM)) {
        resume->flags.has_shotgun = true;
        resume->shotgun_ammo = lara->shotgun_ammo.ammo;
    } else {
        resume->flags.has_shotgun = false;
        resume->shotgun_ammo =
            Inv_RequestItem(O_SHOTGUN_AMMO_ITEM) * SHOTGUN_AMMO_QTY;
    }

    if (Inv_RequestItem(O_MAGNUM_ITEM)) {
        resume->flags.has_magnums = true;
        resume->magnum_ammo = lara->magnum_ammo.ammo;
    } else {
        resume->flags.has_magnums = false;
        resume->magnum_ammo =
            Inv_RequestItem(O_MAGNUM_AMMO_ITEM) * MAGNUM_AMMO_QTY;
    }

    if (Inv_RequestItem(O_UZI_ITEM)) {
        resume->flags.has_uzis = true;
        resume->uzi_ammo = lara->uzi_ammo.ammo;
    } else {
        resume->flags.has_uzis = false;
        resume->uzi_ammo = Inv_RequestItem(O_UZI_AMMO_ITEM) * UZI_AMMO_QTY;
    }

#if TR_VERSION == 1
    resume->num_scions = Inv_RequestItem(O_SCION_ITEM_1);
#elif TR_VERSION == 2
    if (Inv_RequestItem(O_M16_ITEM)) {
        resume->flags.has_m16 = true;
        resume->m16_ammo = lara->m16_ammo.ammo;
    } else {
        resume->flags.has_m16 = false;
        resume->m16_ammo = Inv_RequestItem(O_M16_AMMO_ITEM) * M16_AMMO_QTY;
    }

    if (Inv_RequestItem(O_HARPOON_ITEM)) {
        resume->flags.has_harpoon = true;
        resume->harpoon_ammo = lara->harpoon_ammo.ammo;
    } else {
        resume->flags.has_harpoon = false;
        resume->harpoon_ammo =
            Inv_RequestItem(O_HARPOON_AMMO_ITEM) * HARPOON_AMMO_QTY;
    }

    if (Inv_RequestItem(O_GRENADE_ITEM)) {
        resume->flags.has_grenade = true;
        resume->grenade_ammo = lara->grenade_ammo.ammo;
    } else {
        resume->flags.has_grenade = false;
        resume->grenade_ammo =
            Inv_RequestItem(O_GRENADE_AMMO_ITEM) * GRENADE_AMMO_QTY;
    }

    resume->flares = Inv_RequestItem(O_FLARE_ITEM);
#endif

    resume->equipped_gun_type = lara->last_gun_type;
    resume->holsters_gun_type = lara->holsters_gun_type;
    resume->back_gun_type = lara->back_gun_type;
    if (TR_VERSION == 1 && lara->gun_status == LGS_READY) { // TODO: TR2
        resume->gun_status = LGS_READY;
    } else {
        resume->gun_status = LGS_ARMLESS;
    }
}

void Savegame_ApplyLogicToCurrentInfo(const GF_LEVEL *const level)
{
    RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
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
        resume->flags.costume = TR_VERSION == 1;

        resume->flags.has_pistols = false;
        resume->flags.has_shotgun = false;
        resume->flags.has_magnums = false;
        resume->flags.has_uzis = false;

        resume->small_medipacks = 0;
        resume->large_medipacks = 0;
        resume->pistol_ammo = 0;
        resume->shotgun_ammo = 0;
        resume->magnum_ammo = 0;
        resume->uzi_ammo = 0;
#if TR_VERSION == 1
        resume->num_scions = 0;
#else
        resume->flags.has_harpoon = false;
        resume->flags.has_m16 = false;
        resume->flags.has_grenade = false;
        resume->harpoon_ammo = 0;
        resume->m16_ammo = 0;
        resume->grenade_ammo = 0;
        resume->flares = 0;
#endif
        resume->equipped_gun_type = LGT_UNARMED;
        resume->holsters_gun_type = LGT_UNARMED;
        resume->back_gun_type = LGT_UNARMED;
        resume->gun_status = LGS_ARMLESS;
    }

    if (level == GF_GetFirstLevel()) {
        resume->flags.available = true;
        resume->flags.costume = false;

        resume->flags.has_pistols = true;
        resume->flags.has_shotgun = false;
        resume->flags.has_magnums = false;
        resume->flags.has_uzis = false;

        resume->small_medipacks = 0;
        resume->large_medipacks = 0;
        resume->pistol_ammo = 1000;
        resume->shotgun_ammo = 0;
        resume->magnum_ammo = 0;
        resume->uzi_ammo = 0;
#if TR_VERSION == 1
        resume->num_scions = 0;
#else
        resume->flags.has_harpoon = false;
        resume->flags.has_m16 = false;
        resume->flags.has_grenade = false;
        resume->harpoon_ammo = 0;
        resume->m16_ammo = 0;
        resume->grenade_ammo = 0;
        resume->flares = 0;
#endif
        resume->equipped_gun_type = LGT_PISTOLS;
        resume->holsters_gun_type = LGT_PISTOLS;
        resume->back_gun_type = LGT_UNARMED;
        resume->gun_status = LGS_ARMLESS;
    }

    if (Game_IsBonusFlagSet(GBF_NGPLUS) && level != GF_GetGymLevel()) {
        resume->flags.has_pistols = true;
        resume->flags.has_shotgun = true;
        resume->flags.has_magnums = true;
        resume->flags.has_uzis = true;

        resume->shotgun_ammo = 10000;
        resume->magnum_ammo = 10000;
        resume->uzi_ammo = 10000;

#if TR_VERSION == 1
        resume->equipped_gun_type = LGT_UZIS;
        resume->holsters_gun_type = LGT_UZIS;
        resume->back_gun_type = LGT_SHOTGUN;
#else
        resume->flags.has_m16 = true;
        resume->flags.has_grenade = true;
        resume->flags.has_harpoon = true;
        resume->m16_ammo = 10000;
        resume->grenade_ammo = 10000;
        resume->harpoon_ammo = 10000;

        resume->flares = -1;
        resume->equipped_gun_type = LGT_GRENADE;
        resume->holsters_gun_type = LGT_PISTOLS;
        resume->back_gun_type = LGT_GRENADE;
#endif
    }

#if TR_VERSION == 2
    const STATS_COMMON default_stats = Savegame_GetDefaultStats(level);
    resume->stats.max_secret_count = default_stats.max_secret_count;
    resume->stats.all_secrets_mask = default_stats.all_secrets_mask;
    resume->stats.secret_flags = 0;
#endif

    M_DetermineLegacyGunTypes(resume);
}

void Savegame_ProcessItemsBeforeSave(void)
{
    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        ITEM *const item = Item_Get(i);
        const OBJECT *const obj = Object_Get(item->object_id);
        if (obj->handle_save_func != nullptr) {
            obj->handle_save_func(item, SAVEGAME_STAGE_BEFORE_SAVE);
        }
    }
}

void Savegame_ProcessItemsBeforeLoad(void)
{
    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        ITEM *const item = Item_Get(i);
        const OBJECT *const obj = Object_Get(item->object_id);
        if (obj->handle_save_func != nullptr) {
            obj->handle_save_func(item, SAVEGAME_STAGE_BEFORE_LOAD);
        }
    }
}

void Savegame_SetDefaultStats(
    const GF_LEVEL *const level, const STATS_COMMON stats)
{
    if (m_DefaultStats == nullptr) {
        m_DefaultStats = Memory_Alloc(
            sizeof(STATS_COMMON) * GF_GetLevelTable(GFLT_MAIN)->count);
    }
    m_DefaultStats[level->num] = stats;
}

STATS_COMMON Savegame_GetDefaultStats(const GF_LEVEL *const level)
{
    if (m_DefaultStats == nullptr
        || (level->type != GFL_NORMAL && level->type != GFL_BONUS)) {
        return (STATS_COMMON) {};
    }
    return m_DefaultStats[level->num];
}

void Savegame_ScanSavedGames(void)
{
    BENCHMARK benchmark = Benchmark_Start();
    M_ClearSlots();

    m_SaveCounter = 0;
    m_SavedGames = 0;
    m_MostRecentlyCreatedSlot = -1;

    M_ScanSavedGamesDir(SAVES_DIR);
    M_ScanSavedGamesDir(".");

    for (int32_t i = 0; i < m_SaveSlots; i++) {
        SAVEGAME_INFO *const savegame_info = &m_SavegameInfo[i];
        if (savegame_info->level_title != nullptr) {
            if (savegame_info->counter > m_SaveCounter) {
                m_SaveCounter = savegame_info->counter;
                m_MostRecentlyCreatedSlot = i;
            }
            m_SavedGames++;
        }
    }

    Benchmark_End(&benchmark, nullptr);
}

bool Savegame_Save(const int32_t slot_idx)
{
    bool result = false;
    Savegame_BindSlot(slot_idx);

    File_CreateDirectory(SAVES_DIR);

    const GF_LEVEL *const current_level = Game_GetCurrentLevel();
    const char *const level_title = current_level->title;

    Savegame_PersistGameToCurrentInfo(current_level);

    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < level_table->count; i++) {
        const GF_LEVEL *const level = &level_table->levels[i];
        if (level->type == GFL_CURRENT) {
            Savegame_SetCurrentInfo(i, current_level->num);
        }
    }

    SAVEGAME_INFO *const savegame_info = &m_SavegameInfo[slot_idx];
    const bool was_slot_empty = savegame_info->full_path == nullptr;

    m_SaveCounter++;
    for (int32_t i = 0; i < m_StrategyCount; i++) {
        const SAVEGAME_STRATEGY strategy = m_Strategies[i];
        if (!strategy.allow_save || strategy.save_to_file_func == nullptr) {
            continue;
        }

        char *file_name =
            String_Format(strategy.get_save_file_pattern_func(), slot_idx);
        char *full_path = String_Format("%s/%s", SAVES_DIR, file_name);
        MYFILE *const fp = File_Open(full_path, FILE_OPEN_WRITE);
        if (fp != nullptr) {
            strategy.save_to_file_func(fp, savegame_info);
            savegame_info->format = strategy.format;
            Memory_FreePointer(&savegame_info->full_path);
            savegame_info->full_path = Memory_DupStr(File_GetPath(fp));
            savegame_info->counter = m_SaveCounter;
            savegame_info->level_num = current_level->num;
            savegame_info->level_title =
                level_title != nullptr ? Memory_DupStr(level_title) : nullptr;
            File_Close(fp);
            result = true;
        }

        Memory_FreePointer(&file_name);
        Memory_FreePointer(&full_path);
    }

    if (result) {
        m_MostRecentlyCreatedSlot = slot_idx;
        if (was_slot_empty) {
            m_SavedGames++;
        }
        Savegame_HighlightNewestSlot();
    } else {
        m_SaveCounter--;
    }

    return result;
}

bool Savegame_Load(const int32_t slot_idx)
{
    const SAVEGAME_INFO *const savegame_info = &m_SavegameInfo[slot_idx];
    ASSERT(savegame_info->format != 0);

    M_LoadPreprocess();

    bool result = false;
    for (int32_t i = 0; i < m_StrategyCount; i++) {
        const SAVEGAME_STRATEGY strategy = m_Strategies[i];
        if (strategy.format != savegame_info->format) {
            continue;
        }

        MYFILE *const fp = File_Open(savegame_info->full_path, FILE_OPEN_READ);
        if (fp != nullptr) {
            result = strategy.load_from_file_func(fp);
            File_Close(fp);
        }
        break;
    }

    M_LoadPostprocess();
    m_InitialVersion = m_SavegameInfo[slot_idx].initial_version;
    return result;
}

bool Savegame_UpdateDeathCounters(
    const int32_t slot_num, const int32_t death_count)
{
    ASSERT(slot_num >= 0);
    const SAVEGAME_INFO *const savegame_info = &m_SavegameInfo[slot_num];
    ASSERT(savegame_info->format != SAVEGAME_FORMAT_INVALID);

    bool ret = false;
    for (int32_t i = 0; i < m_StrategyCount; i++) {
        const SAVEGAME_STRATEGY strategy = m_Strategies[i];
        if (savegame_info->format == strategy.format
            && strategy.update_death_counters_func != nullptr) {
            MYFILE *const fp =
                File_Open(savegame_info->full_path, FILE_OPEN_READ_WRITE);
            if (fp != nullptr) {
                ret = strategy.update_death_counters_func(
                    fp, savegame_info->level_num, death_count);
                File_Close(fp);
            }
            break;
        }
    }
    return ret;
}

bool Savegame_LoadOnlyResumeInfo(int32_t slot_num)
{
    const SAVEGAME_INFO *const savegame_info = &m_SavegameInfo[slot_num];
    ASSERT(savegame_info->format != SAVEGAME_FORMAT_INVALID);

    bool ret = false;
    for (int32_t i = 0; i < m_StrategyCount; i++) {
        const SAVEGAME_STRATEGY strategy = m_Strategies[i];
        if (savegame_info->format == strategy.format
            && strategy.load_only_resume_info_func != nullptr) {
            MYFILE *const fp =
                File_Open(savegame_info->full_path, FILE_OPEN_READ);
            if (fp != nullptr) {
                ret = strategy.load_only_resume_info_func(fp);
                File_Close(fp);
            }
            break;
        }
    }

    Savegame_SetInitialVersion(m_SavegameInfo[slot_num].initial_version);
    return ret;
}
