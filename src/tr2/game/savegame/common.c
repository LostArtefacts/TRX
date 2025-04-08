#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/inventory.h"
#include "game/lara/misc.h"
#include "game/requester.h"
#include "game/savegame.h"
#include "global/vars.h"

#include <libtrx/benchmark.h>
#include <libtrx/debug.h>
#include <libtrx/enum_map.h>
#include <libtrx/filesystem.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/objects/traps/movable_block.h>
#include <libtrx/game/savegame.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>
#include <libtrx/strings.h>
#include <libtrx/utils.h>

#define MAX_STRATEGIES 2
#define SAVES_DIR "saves"

static STATS_COMMON *m_DefaultStats = nullptr;
static RESUME_INFO *m_ResumeInfos = nullptr;
static int32_t m_SaveSlots = 0;
static int32_t m_NewestSlot = -1;
static int32_t m_SaveCounter = 0;
static int32_t m_SavedGames = 0;
static SAVEGAME_INFO *m_SavegameInfo = nullptr;

static uint32_t m_ReqFlags1[MAX_REQUESTER_ITEMS] = {};
static uint32_t m_ReqFlags2[MAX_REQUESTER_ITEMS] = {};

static int32_t m_StrategyCount = 0;
static SAVEGAME_STRATEGY m_Strategies[MAX_STRATEGIES];
static SAVEGAME_VERSION m_InitialVersion = VERSION_LEGACY;

static void M_ClearSlots(void);
static bool M_FillSlot(
    SAVEGAME_STRATEGY strategy, int32_t slot_num, const char *path);
static void M_ScanSavedGamesDir(const char *dir_path);

static void M_ClearSlots(void)
{
    if (m_SavegameInfo == nullptr) {
        return;
    }

    for (int32_t i = 0; i < m_SaveSlots; i++) {
        SAVEGAME_INFO *const savegame_info = &m_SavegameInfo[i];
        savegame_info->format = SAVEGAME_FORMAT_INVALID;
        savegame_info->counter = -1;
        savegame_info->level_num = -1;
        Memory_FreePointer(&savegame_info->full_path);
        Memory_FreePointer(&savegame_info->level_title);
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
        if (strategy.fill_info_func(fp, savegame_info)) {
            savegame_info->format = strategy.format;
            Memory_FreePointer(&savegame_info->full_path);
            savegame_info->full_path = Memory_DupStr(path);
            result = true;
        }
        File_Close(fp);
    }
    return result;
}

static void M_ScanSavedGamesDir(const char *const dir_path)
{
    void *dir_handle = File_OpenDirectory(dir_path);
    if (dir_handle == nullptr) {
        return;
    }

    while (true) {
        const char *file_name = File_ReadDirectory(dir_handle);
        if (file_name == nullptr) {
            break;
        }
        if (strcmp(file_name, ".") == 0 || strcmp(file_name, "..") == 0) {
            continue;
        }

        for (int32_t i = 0; i < m_StrategyCount; i++) {
            const SAVEGAME_STRATEGY strategy = m_Strategies[i];
            if (!strategy.allow_load) {
                continue;
            }

            int32_t slot = -1;
            const int32_t parsed =
                sscanf(file_name, strategy.get_save_file_pattern_func(), &slot);
            if (parsed == 1 && slot >= 0 && slot < m_SaveSlots) {
                char *file_path = String_Format("%s/%s", dir_path, file_name);
                M_FillSlot(strategy, slot, file_path);
                Memory_FreePointer(&file_path);
            }
        }
    }

    File_CloseDirectory(dir_handle);
}

static void M_LoadPreprocess(void)
{
    Savegame_InitCurrentInfo();
}

static void M_LoadPostprocess(void)
{
    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        ITEM *const item = Item_Get(i);
        const OBJECT *const obj = Object_Get(item->object_id);

        if (obj->save_position && obj->shadow_size != 0) {
            int16_t room_num = item->room_num;
            const SECTOR *const sector = Room_GetSector(
                item->pos.x, item->pos.y, item->pos.z, &room_num);
            item->floor =
                Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
        }

        if (obj->save_flags != 0) {
            item->flags &= 0xFF00;
        }
    }

    MovableBlock_SetupFloor();

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->burn) {
        lara->burn = 0;
        Lara_CatchFire();
    }
}

void Savegame_RegisterStrategy(const SAVEGAME_STRATEGY strategy)
{
    ASSERT(m_StrategyCount < MAX_STRATEGIES);
    m_Strategies[m_StrategyCount] = strategy;
    m_StrategyCount++;
}

void Savegame_Init(void)
{
    m_ResumeInfos = Memory_Alloc(
        sizeof(RESUME_INFO)
        * (GF_GetLevelTable(GFLT_MAIN)->count
           + GF_GetLevelTable(GFLT_DEMOS)->count));

    m_SaveSlots = MAX_SAVE_SLOTS; // TODO: make configurable
    m_SavegameInfo = Memory_Alloc(sizeof(SAVEGAME_INFO) * m_SaveSlots);

    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_DEMOS);
    for (int32_t i = 0; i < level_table->count; i++) {
        RESUME_INFO *const resume_info =
            Savegame_GetCurrentInfo(&level_table->levels[i]);
        resume_info->flags.available = 1;
        resume_info->flags.has_pistols = 1;
        resume_info->pistol_ammo = 1000;
        resume_info->gun_status = LGS_ARMLESS;
        resume_info->equipped_gun_type = LGT_PISTOLS;
    }
}

void Savegame_Shutdown(void)
{
    M_ClearSlots();
    Memory_FreePointer(&m_ResumeInfos);
    Memory_FreePointer(&m_SavegameInfo);
    Memory_FreePointer(&m_DefaultStats);
}

int32_t Savegame_GetSlotCount(void)
{
    return MAX_SAVE_SLOTS;
}

bool Savegame_IsSlotFree(const int32_t slot_idx)
{
    return m_SavegameInfo[slot_idx].level_num == -1;
}

int32_t Savegame_GetLevelNumber(const int32_t slot_idx)
{
    return m_SavegameInfo[slot_idx].level_num;
}

void Savegame_ScanSavedGames(void)
{
    BENCHMARK benchmark = Benchmark_Start();
    M_ClearSlots();

    m_SaveCounter = 0;
    m_SavedGames = 0;
    m_NewestSlot = -1;

    M_ScanSavedGamesDir(SAVES_DIR);
    M_ScanSavedGamesDir(".");

    for (int32_t i = 0; i < m_SaveSlots; i++) {
        SAVEGAME_INFO *const savegame_info = &m_SavegameInfo[i];
        if (savegame_info->level_title != nullptr) {
            if (savegame_info->counter > m_SaveCounter) {
                m_SaveCounter = savegame_info->counter;
                m_NewestSlot = i;
            }
            m_SavedGames++;
        }
    }

    Benchmark_End(&benchmark, nullptr);
}

void Savegame_FillAvailableSaves(REQUEST_INFO *const req)
{
    Requester_Init(req);

    for (int32_t i = 0; i < MAX_SAVE_SLOTS; i++) {
        const SAVEGAME_INFO *const savegame_info = &m_SavegameInfo[i];
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

void Savegame_HighlightNewestSlot(void)
{
    g_LoadGameRequester.selected = MAX(0, m_NewestSlot);
}

int32_t Savegame_GetCounter(void)
{
    return m_SaveCounter;
}

int32_t Savegame_GetTotalCount(void)
{
    return m_SavedGames;
}

SAVEGAME_VERSION Savegame_GetInitialVersion(void)
{
    return m_InitialVersion;
}

void Savegame_SetInitialVersion(const SAVEGAME_VERSION version)
{
    m_InitialVersion = version;
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

void Savegame_InitCurrentInfo(void)
{
    if (Game_IsBonusFlagSet(GBF_NGPLUS)) {
        return;
    }

    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < level_table->count; i++) {
        const GF_LEVEL *const level = &level_table->levels[i];
        Savegame_ResetCurrentInfo(level);
        Savegame_ApplyLogicToCurrentInfo(level);
        Savegame_GetCurrentInfo(level)->flags.available = 0;
    }

    if (GF_GetGymLevel() != nullptr) {
        Savegame_GetCurrentInfo(GF_GetGymLevel())->flags.available = 1;
    }
    if (GF_GetFirstLevel() != nullptr) {
        Savegame_GetCurrentInfo(GF_GetFirstLevel())->flags.available = 1;
    }
    Game_SetBonusFlag(GBF_NONE);
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

void Savegame_ResetCurrentInfo(const GF_LEVEL *const level)
{
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

    resume->flags.available = 1;

    if (Inv_RequestItem(O_PISTOL_ITEM)) {
        resume->flags.has_pistols = 1;
        resume->pistol_ammo = 1000;
    } else {
        resume->flags.has_pistols = 0;
        resume->pistol_ammo = 1000;
    }

    if (Inv_RequestItem(O_SHOTGUN_ITEM)) {
        resume->flags.has_shotgun = 1;
        resume->shotgun_ammo = g_Lara.shotgun_ammo.ammo;
    } else {
        resume->flags.has_shotgun = 0;
        resume->shotgun_ammo =
            Inv_RequestItem(O_SHOTGUN_AMMO_ITEM) * SHOTGUN_AMMO_QTY;
    }

    if (Inv_RequestItem(O_MAGNUM_ITEM)) {
        resume->flags.has_magnums = 1;
        resume->magnum_ammo = g_Lara.magnum_ammo.ammo;
    } else {
        resume->flags.has_magnums = 0;
        resume->magnum_ammo =
            Inv_RequestItem(O_MAGNUM_AMMO_ITEM) * MAGNUM_AMMO_QTY;
    }

    if (Inv_RequestItem(O_UZI_ITEM)) {
        resume->flags.has_uzis = 1;
        resume->uzi_ammo = g_Lara.uzi_ammo.ammo;
    } else {
        resume->flags.has_uzis = 0;
        resume->uzi_ammo = Inv_RequestItem(O_UZI_AMMO_ITEM) * UZI_AMMO_QTY;
    }

    if (Inv_RequestItem(O_M16_ITEM)) {
        resume->flags.has_m16 = 1;
        resume->m16_ammo = g_Lara.m16_ammo.ammo;
    } else {
        resume->flags.has_m16 = 0;
        resume->m16_ammo = Inv_RequestItem(O_M16_AMMO_ITEM) * M16_AMMO_QTY;
    }

    if (Inv_RequestItem(O_HARPOON_ITEM)) {
        resume->flags.has_harpoon = 1;
        resume->harpoon_ammo = g_Lara.harpoon_ammo.ammo;
    } else {
        resume->flags.has_harpoon = 0;
        resume->harpoon_ammo =
            Inv_RequestItem(O_HARPOON_AMMO_ITEM) * HARPOON_AMMO_QTY;
    }

    if (Inv_RequestItem(O_GRENADE_ITEM)) {
        resume->flags.has_grenade = 1;
        resume->grenade_ammo = g_Lara.grenade_ammo.ammo;
    } else {
        resume->flags.has_grenade = 0;
        resume->grenade_ammo =
            Inv_RequestItem(O_GRENADE_AMMO_ITEM) * GRENADE_AMMO_QTY;
    }

    resume->flares = Inv_RequestItem(O_FLARE_ITEM);
    resume->small_medipacks = Inv_RequestItem(O_SMALL_MEDIPACK_ITEM);
    resume->large_medipacks = Inv_RequestItem(O_LARGE_MEDIPACK_ITEM);

    if (g_Lara.gun_type == LGT_FLARE) {
        resume->equipped_gun_type = g_Lara.last_gun_type;
    } else {
        resume->equipped_gun_type = g_Lara.gun_type;
    }
    resume->gun_status = LGS_ARMLESS;
}

bool Savegame_Save(const int32_t slot_idx)
{
    bool result = false;
    Savegame_BindSlot(slot_idx);

    File_CreateDirectory(SAVES_DIR);

    const GF_LEVEL *const current_level = Game_GetCurrentLevel();
    const char *const level_title = current_level->title;

    Savegame_PersistGameToCurrentInfo(current_level);

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
        char save_num_text[16];
        sprintf(save_num_text, "%d", m_SaveCounter);
        Requester_ChangeItem(
            &g_LoadGameRequester, slot_idx, level_title, REQ_ALIGN_LEFT,
            save_num_text, REQ_ALIGN_RIGHT);

        m_ReqFlags1[slot_idx] = g_RequesterFlags1[slot_idx];
        m_ReqFlags2[slot_idx] = g_RequesterFlags2[slot_idx];

        m_NewestSlot = slot_idx;
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
    Savegame_SetInitialVersion(m_SavegameInfo[slot_idx].initial_version);
    return result;
}

RESUME_INFO *Savegame_GetCurrentInfo(const GF_LEVEL *const level)
{
    ASSERT(m_ResumeInfos != nullptr);
    ASSERT(level != nullptr);
    if (GF_GetLevelTableType(level->type) == GFLT_MAIN) {
        return &m_ResumeInfos[level->num];
    } else if (level->type == GFL_DEMO) {
        return &m_ResumeInfos[GF_GetLevelTable(GFLT_MAIN)->count];
    }
    LOG_WARNING(
        "Warning: unable to get resume info for level %d (type=%s)", level->num,
        ENUM_MAP_TO_STRING(GF_LEVEL_TYPE, level->type));
    return nullptr;
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
