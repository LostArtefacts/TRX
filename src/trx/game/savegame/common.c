#include <trx/config.h>
#include <trx/core/file.h>
#include <trx/core/subsystem.h>
#include <trx/debug.h>
#include <trx/game/game.h>
#include <trx/game/game_flow.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/rooms.h>
#include <trx/game/savegame.h>
#include <trx/game/savegame/file.h>
#include <trx/version.h>

static SAVEGAME_VERSION m_InitialVersion = SG_VERSION_LEGACY;

static void M_LoadPreprocess(void)
{
    SG_Resume_ResetAllEntries();
}

static void M_LoadPostprocess(void)
{
    // TODO: tidy this; skidoo drivers currently require handle_save_func to be
    // called immediately on load within the strategies.
    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        ITEM *const item = Item_Get(i);
        const OBJECT *const obj = Object_Get(item->object_id);

        if (obj->save_position && (obj->shadow_size != 0 || obj->load_floor)) {
            int16_t room_num = item->room_num;
            const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
            item->floor = Room_GetHeight(sector, item->pos);
        }

        // TODO: make this engine-agnostic
        if (g_TRVersion == 1 && obj->handle_save_func != nullptr) {
            obj->handle_save_func(item, SAVEGAME_STAGE_AFTER_LOAD);
        }
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    // The cached mesh positions describe Lara as she stood before the load.
    lara->mesh_pos_matrices_valid = false;
    if (Game_GetBonusFlag() != GBF_NONE) {
        CONFIG_SET(g_Config.profile.new_game_plus_unlock, true);
        Config_Update();
    }
    if (lara->burn && !g_Config.gameplay.enable_enhanced_saves) {
        lara->burn = false;
        Lara_CatchFire();
    }
}

static void M_Shutdown(void)
{
    SG_Manager_Shutdown();
    SG_Resume_Shutdown();
}

SAVEGAME_VERSION Savegame_GetInitialVersion(void)
{
    return m_InitialVersion;
}

void Savegame_SetInitialVersion(const SAVEGAME_VERSION version)
{
    m_InitialVersion = version;
}

void Savegame_Init(void)
{
    SG_Resume_Init();
    SG_Manager_Init();
}

bool Savegame_IsManualSaveAllowed(void)
{
    const SAVE_CRYSTAL_MODE crystal_mode = g_Config.gameplay.save_crystal_mode;
    return !g_Config.flow.load_save_disabled
        && crystal_mode != SAVE_CRYSTAL_SAVE
        && crystal_mode != SAVE_CRYSTAL_SAVE_PICKUP;
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

bool Savegame_Save(const SAVEGAME_SLOT_REF slot)
{
    if (!SG_Manager_IsValidSlotRef(slot)) {
        return false;
    }
    SG_Manager_BindSlot(slot);

    const GF_LEVEL *const current_level = Game_GetCurrentLevel();
    SG_Resume_StoreGameToEntry(current_level);
    SG_Resume_MirrorCurrentEntry(current_level);

    return SG_Manager_WriteSlot(slot);
}

RESULT Savegame_Load(const SAVEGAME_SLOT_REF slot)
{
    const SAVEGAME_INFO *const savegame_info = SG_Manager_GetSavegameInfo(slot);
    FAIL_IF(savegame_info == nullptr, "the slot holds no saved game");
    ASSERT(savegame_info->full_path != nullptr);

    M_LoadPreprocess();

    TRX_FILE *fp = nullptr;
    RESULT result =
        File_OpenPath(savegame_info->full_path, FILE_OPEN_READ, &fp);
    if (IS_OK(result)) {
        result = SG_File_LoadFromFile(fp);
        File_Close(fp);
    }

    M_LoadPostprocess();
    m_InitialVersion = savegame_info->initial_version;
    return result;
}

RESULT Savegame_UpdateDeathCounters(
    const SAVEGAME_SLOT_REF slot, const int32_t death_count)
{
    const SAVEGAME_INFO *const savegame_info = SG_Manager_GetSavegameInfo(slot);
    FAIL_IF(savegame_info == nullptr, "the slot holds no saved game");
    ASSERT(savegame_info->full_path != nullptr);

    TRX_FILE *fp = nullptr;
    MUST(File_OpenPath(savegame_info->full_path, FILE_OPEN_READ_WRITE, &fp));
    const RESULT result = SG_File_UpdateDeathCounters(
        fp, savegame_info->level_num, death_count, savegame_info->is_quick);
    File_Close(fp);
    return result;
}

RESULT Savegame_LoadOnlyResumeInfo(const SAVEGAME_SLOT_REF slot)
{
    const SAVEGAME_INFO *const savegame_info = SG_Manager_GetSavegameInfo(slot);
    FAIL_IF(savegame_info == nullptr, "the slot holds no saved game");
    ASSERT(savegame_info->full_path != nullptr);

    TRX_FILE *fp = nullptr;
    RESULT result =
        File_OpenPath(savegame_info->full_path, FILE_OPEN_READ, &fp);
    if (IS_OK(result)) {
        result = SG_File_LoadOnlyResumeInfo(fp);
        File_Close(fp);
    }

    Savegame_SetInitialVersion(savegame_info->initial_version);
    return result;
}

bool Savegame_RestartAvailable(const SAVEGAME_SLOT_REF slot)
{
    if (!SG_Manager_IsValidSlotRef(slot)) {
        return true;
    }
    const SAVEGAME_INFO *const savegame_info = SG_Manager_GetSavegameInfo(slot);
    return savegame_info->features.restart;
}

REGISTER_SUBSYSTEM(.shutdown = M_Shutdown)
