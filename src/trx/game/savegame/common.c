#include <trx/config.h>
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
    Savegame_InitCurrentInfo();
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
    if (Game_GetBonusFlag() != GBF_NONE) {
        g_Config.profile.new_game_plus_unlock = true;
        Config_Update();
    }
    if (lara->burn && !g_Config.gameplay.enable_enhanced_saves) {
        lara->burn = false;
        Lara_CatchFire();
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

void Savegame_Init(void)
{
    Savegame_Resume_Init();
    Savegame_Manager_Init();
}

void Savegame_Shutdown(void)
{
    Savegame_Manager_Shutdown();
    Savegame_Resume_Shutdown();
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
    if (!Savegame_IsValidSlotRef(slot)) {
        return false;
    }
    Savegame_BindSlot(slot);

    const GF_LEVEL *const current_level = Game_GetCurrentLevel();
    Savegame_PersistGameToCurrentInfo(current_level);

    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < level_table->count; i++) {
        const GF_LEVEL *const level = &level_table->levels[i];
        if (level->type == GFL_CURRENT) {
            Savegame_SetCurrentInfo(i, current_level->num);
        }
    }

    return Savegame_Manager_WriteSlot(slot);
}

bool Savegame_Load(const SAVEGAME_SLOT_REF slot)
{
    const SAVEGAME_INFO *const savegame_info = Savegame_GetSavegameInfo(slot);
    if (savegame_info == nullptr) {
        return false;
    }
    ASSERT(savegame_info->full_path != nullptr);

    M_LoadPreprocess();

    bool result = false;
    MYFILE *const fp = File_Open(savegame_info->full_path, FILE_OPEN_READ);
    if (fp != nullptr) {
        result = SG_File_LoadFromFile(fp);
        File_Close(fp);
    }

    M_LoadPostprocess();
    m_InitialVersion = savegame_info->initial_version;
    return result;
}

bool Savegame_UpdateDeathCounters(
    const SAVEGAME_SLOT_REF slot, const int32_t death_count)
{
    const SAVEGAME_INFO *const savegame_info = Savegame_GetSavegameInfo(slot);
    if (savegame_info == nullptr) {
        return false;
    }
    ASSERT(savegame_info->full_path != nullptr);

    bool ret = false;
    MYFILE *const fp =
        File_Open(savegame_info->full_path, FILE_OPEN_READ_WRITE);
    if (fp != nullptr) {
        ret = SG_File_UpdateDeathCounters(
            fp, savegame_info->level_num, death_count, savegame_info->is_quick);
        File_Close(fp);
    }
    return ret;
}

bool Savegame_LoadOnlyResumeInfo(const SAVEGAME_SLOT_REF slot)
{
    const SAVEGAME_INFO *const savegame_info = Savegame_GetSavegameInfo(slot);
    if (savegame_info == nullptr) {
        return false;
    }
    ASSERT(savegame_info->full_path != nullptr);

    bool ret = false;
    MYFILE *const fp = File_Open(savegame_info->full_path, FILE_OPEN_READ);
    if (fp != nullptr) {
        ret = SG_File_LoadOnlyResumeInfo(fp);
        File_Close(fp);
    }

    Savegame_SetInitialVersion(savegame_info->initial_version);
    return ret;
}

bool Savegame_RestartAvailable(const SAVEGAME_SLOT_REF slot)
{
    if (!Savegame_IsValidSlotRef(slot)) {
        return true;
    }
    const SAVEGAME_INFO *const savegame_info = Savegame_GetSavegameInfo(slot);
    return savegame_info->features.restart;
}
