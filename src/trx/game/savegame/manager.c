#include <trx/config.h>
#include <trx/core/benchmark.h>
#include <trx/core/file.h>
#include <trx/core/filesystem.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/debug.h>
#include <trx/game/game.h>
#include <trx/game/game_flow.h>
#include <trx/game/savegame.h>
#include <trx/game/savegame/file.h>
#include <trx/game/shell.h>

static SAVEGAME_INFO *m_NormalSavegameInfo = nullptr;
static SAVEGAME_INFO *m_QuickSavegameInfo = nullptr;
static int32_t m_SaveSlots = 0;
static int32_t m_QuickSaveSlots = 0;
static int32_t m_SavedGames = 0;
static int32_t m_SaveCounter = 0;
static int32_t m_NextQuickSlot = 0;
static SAVEGAME_SLOT_REF m_MostRecentlyUsedSlot = { .index = -1 };
static SAVEGAME_SLOT_REF m_MostRecentlyCreatedSlot = { .index = -1 };
static SAVEGAME_SLOT_REF m_BoundSlot = { .index = -1 };

static const char *M_GetSaveWriteDir(void)
{
    const char *const saves_dir = GamePath_Get(GAME_PATH_SAVES_DIR);
    const SHELL_ARGS *const args = Shell_GetArgs();
    if (args != nullptr && args->startup.mod != nullptr
        && args->startup.mod->name != nullptr) {
        return String_FormatStatic("%s/%s", saves_dir, args->startup.mod->name);
    }
    return saves_dir;
}

static char *M_GetSaveWritePath(const char *const file_name)
{
    ASSERT(file_name != nullptr);
    return String_Format("%s/%s", M_GetSaveWriteDir(), file_name);
}

static SAVEGAME_INFO *M_GetSavegameInfoSlot(const SAVEGAME_SLOT_REF slot)
{
    switch (slot.pool) {
    case SAVEGAME_SLOT_POOL_NORMAL:
        if (slot.index >= 0 && slot.index < m_SaveSlots) {
            return &m_NormalSavegameInfo[slot.index];
        }
        break;
    case SAVEGAME_SLOT_POOL_QUICK:
        if (slot.index >= 0 && slot.index < m_QuickSaveSlots) {
            return &m_QuickSavegameInfo[slot.index];
        }
        break;
    case SAVEGAME_SLOT_POOL_NUMBER_OF:
        break;
    }
    return nullptr;
}

static const char *M_GetSaveFilePatternForPool(const SAVEGAME_SLOT_POOL pool)
{
    switch (pool) {
    case SAVEGAME_SLOT_POOL_NORMAL:
        return SG_File_GetSaveFilePattern();
    case SAVEGAME_SLOT_POOL_QUICK:
        return SG_File_GetQuickSaveFilePattern();
    case SAVEGAME_SLOT_POOL_NUMBER_OF:
        break;
    }
    return nullptr;
}

static void M_ClearSlot(SAVEGAME_INFO *const savegame_info)
{
    savegame_info->counter = -1;
    savegame_info->level_num = -1;
    savegame_info->is_quick = false;
    Memory_FreePointer(&savegame_info->full_path);
    Memory_FreePointer(&savegame_info->level_title);
}

static void M_ClearSlots(void)
{
    if (m_NormalSavegameInfo != nullptr) {
        for (int32_t i = 0; i < m_SaveSlots; i++) {
            M_ClearSlot(&m_NormalSavegameInfo[i]);
        }
    }
    if (m_QuickSavegameInfo != nullptr) {
        for (int32_t i = 0; i < m_QuickSaveSlots; i++) {
            M_ClearSlot(&m_QuickSavegameInfo[i]);
        }
    }
}

static void M_AllocSlots(void)
{
    m_SaveSlots = SG_Manager_GetSlotCount(SAVEGAME_SLOT_POOL_NORMAL);
    m_QuickSaveSlots = SG_Manager_GetSlotCount(SAVEGAME_SLOT_POOL_QUICK);
    m_NormalSavegameInfo = Memory_Alloc(sizeof(SAVEGAME_INFO) * m_SaveSlots);
    m_QuickSavegameInfo = m_QuickSaveSlots > 0
        ? Memory_Alloc(sizeof(SAVEGAME_INFO) * m_QuickSaveSlots)
        : nullptr;
}

static void M_FreeSlots(void)
{
    M_ClearSlots();
    Memory_FreePointer(&m_NormalSavegameInfo);
    Memory_FreePointer(&m_QuickSavegameInfo);
    m_SaveSlots = 0;
    m_QuickSaveSlots = 0;
}

static bool M_FillSlot(const SAVEGAME_SLOT_REF slot, const char *const path)
{
    SAVEGAME_INFO *const savegame_info = M_GetSavegameInfoSlot(slot);
    if (savegame_info == nullptr) {
        return false;
    }
    bool result = false;
    TRX_FILE *fp = nullptr;
    if (SHOULD(File_OpenPath(path, FILE_OPEN_READ, &fp))) {
        SAVEGAME_INFO tmp_savegame_info;
        if (SG_File_FillInfo(fp, &tmp_savegame_info)) {
            M_ClearSlot(savegame_info);
            *savegame_info = tmp_savegame_info;
            savegame_info->is_quick = slot.pool == SAVEGAME_SLOT_POOL_QUICK;
            savegame_info->full_path = Memory_DupStr(path);
            result = true;
        }
        File_Close(fp);
    }
    return result;
}

static void M_ScanSavedGamesDir(const char *const dir_path)
{
    void *const dir_handle = FS_OpenDirectory(dir_path);
    if (dir_handle == nullptr) {
        return;
    }

    while (true) {
        const char *const file_name = FS_ReadDirectory(dir_handle);
        if (file_name == nullptr) {
            break;
        }
        if (strcmp(file_name, ".") == 0 || strcmp(file_name, "..") == 0) {
            continue;
        }

        char *file_name_ci = String_ToUpper(file_name);
        for (SAVEGAME_SLOT_POOL pool = 0; pool < SAVEGAME_SLOT_POOL_NUMBER_OF;
             pool++) {
            const char *const pattern = M_GetSaveFilePatternForPool(pool);
            char *pattern_ci = String_ToUpperPattern(pattern);
            int32_t slot_idx = -1;
            const int32_t parsed = sscanf(file_name_ci, pattern_ci, &slot_idx);
            Memory_FreePointer(&pattern_ci);

            if (parsed != 1 || slot_idx < 0
                || slot_idx >= SG_Manager_GetSlotCount(pool)) {
                continue;
            }

            char *file_path = String_Format("%s/%s", dir_path, file_name);
            M_FillSlot(
                (SAVEGAME_SLOT_REF) { .pool = pool, .index = slot_idx },
                file_path);
            Memory_FreePointer(&file_path);
            break;
        }
        Memory_FreePointer(&file_name_ci);
    }

    FS_CloseDirectory(dir_handle);
}

static bool M_IsQuickSlotSortedBefore(
    const SAVEGAME_SLOT_REF left, const SAVEGAME_SLOT_REF right)
{
    const SAVEGAME_INFO *const left_info = SG_Manager_GetSavegameInfo(left);
    const SAVEGAME_INFO *const right_info = SG_Manager_GetSavegameInfo(right);
    if (left_info == nullptr || right_info == nullptr) {
        return false;
    }
    if (left_info->counter != right_info->counter) {
        return left_info->counter > right_info->counter;
    }
    return left.index < right.index;
}

void SG_Manager_BindSlot(const SAVEGAME_SLOT_REF slot)
{
    if (!SG_Manager_IsValidSlotRef(slot)) {
        m_BoundSlot = SG_Manager_InvalidSlot();
        return;
    }
    m_BoundSlot = slot;
    m_MostRecentlyUsedSlot = slot;
    LOG_DEBUG("Binding save slot %d:%d", slot.pool, slot.index);
}

SAVEGAME_SLOT_REF SG_Manager_GetMostRecentlyUsedSlot(void)
{
    return m_MostRecentlyUsedSlot;
}

void SG_Manager_UnbindSlot(void)
{
    LOG_DEBUG("Resetting the save slot");
    m_BoundSlot = SG_Manager_InvalidSlot();
}

SAVEGAME_SLOT_REF SG_Manager_GetBoundSlot(void)
{
    return m_BoundSlot;
}

int32_t SG_Manager_GetLevelNumber(const SAVEGAME_SLOT_REF slot)
{
    const SAVEGAME_INFO *const info = SG_Manager_GetSavegameInfo(slot);
    return info != nullptr ? info->level_num : -1;
}

bool SG_Manager_IsSlotFree(const SAVEGAME_SLOT_REF slot)
{
    const SAVEGAME_INFO *const info = SG_Manager_GetSavegameInfo(slot);
    return info == nullptr || info->level_num == -1;
}

int32_t SG_Manager_GetCounter(void)
{
    return m_SaveCounter;
}

int32_t SG_Manager_GetTotalCount(void)
{
    return m_SavedGames;
}

SAVEGAME_SLOT_REF SG_Manager_GetMostRecentlyCreatedSlot(void)
{
    return m_MostRecentlyCreatedSlot;
}

SAVEGAME_SLOT_REF SG_Manager_NormalSlot(const int32_t index)
{
    return (SAVEGAME_SLOT_REF) {
        .pool = SAVEGAME_SLOT_POOL_NORMAL,
        .index = index,
    };
}

SAVEGAME_SLOT_REF SG_Manager_QuickSlot(const int32_t index)
{
    return (SAVEGAME_SLOT_REF) {
        .pool = SAVEGAME_SLOT_POOL_QUICK,
        .index = index,
    };
}

SAVEGAME_SLOT_REF SG_Manager_InvalidSlot(void)
{
    return (SAVEGAME_SLOT_REF) {
        .pool = SAVEGAME_SLOT_POOL_NORMAL,
        .index = -1,
    };
}

bool SG_Manager_IsValidSlotRef(const SAVEGAME_SLOT_REF slot)
{
    return slot.pool >= SAVEGAME_SLOT_POOL_NORMAL
        && slot.pool < SAVEGAME_SLOT_POOL_NUMBER_OF && slot.index >= 0
        && slot.index < SG_Manager_GetSlotCount(slot.pool);
}

int32_t SG_Manager_SlotToParam(const SAVEGAME_SLOT_REF slot)
{
    if (!SG_Manager_IsValidSlotRef(slot)) {
        return -1;
    }
    const uint32_t packed = ((uint32_t)slot.pool << 31) | (uint32_t)slot.index;
    return (int32_t)packed;
}

SAVEGAME_SLOT_REF SG_Manager_SlotFromParam(const int32_t param)
{
    if (param == -1) {
        return SG_Manager_InvalidSlot();
    }

    const uint32_t packed = (uint32_t)param;
    const SAVEGAME_SLOT_POOL pool = (packed >> 31) & 1;
    const int32_t index = (int32_t)(packed & 0x7FFFFFFF);
    return (SAVEGAME_SLOT_REF) {
        .pool = pool,
        .index = index,
    };
}

void SG_Manager_Init(void)
{
    M_AllocSlots();
}

void SG_Manager_Shutdown(void)
{
    M_FreeSlots();
}

bool SG_Manager_IsInitialised(void)
{
    return m_NormalSavegameInfo != nullptr;
}

void SG_Manager_ResizeSlots(void)
{
    M_FreeSlots();
    M_AllocSlots();
}

int32_t SG_Manager_GetSlotCount(const SAVEGAME_SLOT_POOL pool)
{
    switch (pool) {
    case SAVEGAME_SLOT_POOL_NORMAL:
        return g_Config.gameplay.maximum_save_slots;
    case SAVEGAME_SLOT_POOL_QUICK:
        return g_Config.gameplay.maximum_quick_save_slots;
    case SAVEGAME_SLOT_POOL_NUMBER_OF:
        break;
    }
    return 0;
}

SAVEGAME_SLOT_REF SG_Manager_GetNextQuickSlot(void)
{
    if (m_QuickSaveSlots <= 0) {
        return SG_Manager_InvalidSlot();
    }
    if (m_NextQuickSlot < 0 || m_NextQuickSlot >= m_QuickSaveSlots) {
        m_NextQuickSlot = 0;
    }
    return SG_Manager_QuickSlot(m_NextQuickSlot);
}

int32_t SG_Manager_GetQuickVisualCount(void)
{
    int32_t count = 0;
    const int32_t quick_slot_count =
        SG_Manager_GetSlotCount(SAVEGAME_SLOT_POOL_QUICK);
    for (int32_t i = 0; i < quick_slot_count; i++) {
        if (!SG_Manager_IsSlotFree(SG_Manager_QuickSlot(i))) {
            count++;
        }
    }
    return count;
}

SAVEGAME_SLOT_REF SG_Manager_QuickFromVisualIndex(const int32_t visual_index)
{
    if (visual_index < 0) {
        return SG_Manager_InvalidSlot();
    }

    const int32_t quick_slot_count =
        SG_Manager_GetSlotCount(SAVEGAME_SLOT_POOL_QUICK);
    for (int32_t i = 0; i < quick_slot_count; i++) {
        const SAVEGAME_SLOT_REF candidate = SG_Manager_QuickSlot(i);
        if (SG_Manager_IsSlotFree(candidate)) {
            continue;
        }

        int32_t better_count = 0;
        for (int32_t j = 0; j < quick_slot_count; j++) {
            const SAVEGAME_SLOT_REF other = SG_Manager_QuickSlot(j);
            if (SG_Manager_IsSlotFree(other)) {
                continue;
            }
            if (M_IsQuickSlotSortedBefore(other, candidate)) {
                better_count++;
            }
        }

        if (better_count == visual_index) {
            return candidate;
        }
    }

    return SG_Manager_InvalidSlot();
}

int32_t SG_Manager_QuickToVisualIndex(const SAVEGAME_SLOT_REF slot)
{
    if (!SG_Manager_IsValidSlotRef(slot)
        || slot.pool != SAVEGAME_SLOT_POOL_QUICK
        || SG_Manager_IsSlotFree(slot)) {
        return -1;
    }

    const int32_t quick_slot_count =
        SG_Manager_GetSlotCount(SAVEGAME_SLOT_POOL_QUICK);
    int32_t better_count = 0;
    for (int32_t i = 0; i < quick_slot_count; i++) {
        const SAVEGAME_SLOT_REF other = SG_Manager_QuickSlot(i);
        if (SG_Manager_IsSlotFree(other)) {
            continue;
        }
        if (M_IsQuickSlotSortedBefore(other, slot)) {
            better_count++;
        }
    }
    return better_count;
}

const SAVEGAME_INFO *SG_Manager_GetSavegameInfo(const SAVEGAME_SLOT_REF slot)
{
    return M_GetSavegameInfoSlot(slot);
}

void SG_Manager_ScanSavedGames(void)
{
    BENCHMARK benchmark = Benchmark_Start();
    M_ClearSlots();

    m_SaveCounter = 0;
    m_SavedGames = 0;
    m_MostRecentlyCreatedSlot = SG_Manager_InvalidSlot();
    m_NextQuickSlot = 0;
    int32_t newest_quick_counter = -1;
    int32_t newest_quick_slot = -1;

    // Scan low-priority locations first; the write directory is authoritative.
    M_ScanSavedGamesDir(".");
    M_ScanSavedGamesDir(GamePath_Get(GAME_PATH_LEGACY_SAVES_DIR));
    M_ScanSavedGamesDir(GamePath_Get(GAME_PATH_SAVES_DIR));

    {
        // M_GetSaveWriteDir may use static formatting storage, so copy it
        // before scanning because nested formatting calls during scan can
        // overwrite it.
        AUTO_FREE char *write_dir = Memory_DupStr(M_GetSaveWriteDir());
        M_ScanSavedGamesDir(write_dir);
    }

    for (SAVEGAME_SLOT_POOL pool = 0; pool < SAVEGAME_SLOT_POOL_NUMBER_OF;
         pool++) {
        for (int32_t i = 0; i < SG_Manager_GetSlotCount(pool); i++) {
            SAVEGAME_INFO *const savegame_info = M_GetSavegameInfoSlot(
                (SAVEGAME_SLOT_REF) { .pool = pool, .index = i });
            if (savegame_info->level_title == nullptr) {
                continue;
            }
            if (savegame_info->counter > m_SaveCounter) {
                m_SaveCounter = savegame_info->counter;
                m_MostRecentlyCreatedSlot =
                    (SAVEGAME_SLOT_REF) { .pool = pool, .index = i };
            }
            m_SavedGames++;

            if (pool == SAVEGAME_SLOT_POOL_QUICK
                && savegame_info->counter > newest_quick_counter) {
                newest_quick_counter = savegame_info->counter;
                newest_quick_slot = i;
            }
        }
    }

    if (m_QuickSaveSlots > 0 && newest_quick_slot >= 0) {
        m_NextQuickSlot = (newest_quick_slot + 1) % m_QuickSaveSlots;
    }

    Benchmark_End(&benchmark, nullptr);
}

bool SG_Manager_WriteSlot(const SAVEGAME_SLOT_REF slot)
{
    SAVEGAME_INFO *const savegame_info = M_GetSavegameInfoSlot(slot);
    if (savegame_info == nullptr) {
        return false;
    }
    const bool was_slot_empty = savegame_info->full_path == nullptr;

    bool result = false;
    m_SaveCounter++;
    const char *const save_pattern = M_GetSaveFilePatternForPool(slot.pool);
    char *file_name = String_Format(save_pattern, slot.index);
    char *full_path = M_GetSaveWritePath(file_name);
    SHOULD(FS_EnsureParentDirectories(full_path));
    TRX_FILE *fp = nullptr;
    if (SHOULD(File_OpenPath(full_path, FILE_OPEN_WRITE, &fp))) {
        savegame_info->is_quick = slot.pool == SAVEGAME_SLOT_POOL_QUICK;
        SG_File_SaveToFile(fp, savegame_info);
        File_Close(fp);
        result = true;
    }
    if (result) {
        M_FillSlot(slot, full_path);
    }

    Memory_FreePointer(&file_name);
    Memory_FreePointer(&full_path);

    if (result) {
        m_MostRecentlyCreatedSlot = slot;
        if (was_slot_empty) {
            m_SavedGames++;
        }

        if (slot.pool == SAVEGAME_SLOT_POOL_QUICK && m_QuickSaveSlots > 0) {
            m_NextQuickSlot = (slot.index + 1) % m_QuickSaveSlots;
        }
    } else {
        m_SaveCounter--;
    }

    return result;
}

bool SG_Manager_Delete(const SAVEGAME_SLOT_REF slot)
{
    if (!SG_Manager_IsValidSlotRef(slot) || SG_Manager_IsSlotFree(slot)) {
        return false;
    }

    SAVEGAME_INFO *const savegame_info = M_GetSavegameInfoSlot(slot);
    if (savegame_info == nullptr || savegame_info->full_path == nullptr) {
        return false;
    }

    if (!SHOULD(FS_Delete(savegame_info->full_path))) {
        return false;
    }

    M_ClearSlot(savegame_info);
    if (m_SavedGames > 0) {
        m_SavedGames--;
    }
    if (m_BoundSlot.pool == slot.pool && m_BoundSlot.index == slot.index) {
        m_BoundSlot = SG_Manager_InvalidSlot();
    }
    if (m_MostRecentlyUsedSlot.pool == slot.pool
        && m_MostRecentlyUsedSlot.index == slot.index) {
        m_MostRecentlyUsedSlot = SG_Manager_InvalidSlot();
    }
    if (m_MostRecentlyCreatedSlot.pool == slot.pool
        && m_MostRecentlyCreatedSlot.index == slot.index) {
        m_MostRecentlyCreatedSlot = SG_Manager_InvalidSlot();
    }
    return true;
}
