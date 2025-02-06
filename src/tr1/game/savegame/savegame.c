#include "game/savegame.h"

#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/objects/creatures/pod.h"
#include "game/objects/general/pickup.h"
#include "game/objects/general/puzzle_hole.h"
#include "game/objects/general/save_crystal.h"
#include "game/objects/general/scion1.h"
#include "game/objects/traps/movable_block.h"
#include "game/objects/traps/sliding_pillar.h"
#include "game/requester.h"
#include "game/room.h"
#include "game/savegame/savegame_bson.h"
#include "game/savegame/savegame_legacy.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/enum_map.h>
#include <libtrx/filesystem.h>
#include <libtrx/memory.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SAVES_DIR "saves"

typedef struct {
    bool allow_load;
    bool allow_save;
    SAVEGAME_FORMAT format;
    char *(*get_save_filename)(int32_t slot_num);
    bool (*fill_info)(MYFILE *fp, SAVEGAME_INFO *info);
    bool (*load_from_file)(MYFILE *fp, GAME_INFO *game_info);
    bool (*load_only_resume_info)(MYFILE *fp, GAME_INFO *game_info);
    void (*save_to_file)(MYFILE *fp, GAME_INFO *game_info);
    bool (*update_death_counters)(MYFILE *fp, GAME_INFO *game_info);
} SAVEGAME_STRATEGY;

static int32_t m_SaveSlots = 0;
static uint16_t m_NewestSlot = 0;
static SAVEGAME_INFO *m_SavegameInfo = nullptr;

static const SAVEGAME_STRATEGY m_Strategies[] = {
    {
        .allow_load = true,
        .allow_save = true,
        .format = SAVEGAME_FORMAT_BSON,
        .get_save_filename = Savegame_BSON_GetSaveFileName,
        .fill_info = Savegame_BSON_FillInfo,
        .load_from_file = Savegame_BSON_LoadFromFile,
        .load_only_resume_info = Savegame_BSON_LoadOnlyResumeInfo,
        .save_to_file = Savegame_BSON_SaveToFile,
        .update_death_counters = Savegame_BSON_UpdateDeathCounters,
    },
    {
        .allow_load = true,
        .allow_save = false,
        .format = SAVEGAME_FORMAT_LEGACY,
        .get_save_filename = Savegame_Legacy_GetSaveFileName,
        .fill_info = Savegame_Legacy_FillInfo,
        .load_from_file = Savegame_Legacy_LoadFromFile,
        .load_only_resume_info = Savegame_Legacy_LoadOnlyResumeInfo,
        .save_to_file = Savegame_Legacy_SaveToFile,
        .update_death_counters = Savegame_Legacy_UpdateDeathCounters,
    },
    { 0 },
};

static void M_Clear(void);
static void M_LoadPreprocess(void);
static void M_LoadPostprocess(void);

static void M_Clear(void)
{
    if (m_SavegameInfo == nullptr) {
        return;
    }

    for (int i = 0; i < m_SaveSlots; i++) {
        SAVEGAME_INFO *const savegame_info = &m_SavegameInfo[i];
        savegame_info->format = 0;
        savegame_info->counter = -1;
        savegame_info->level_num = -1;
        Memory_FreePointer(&savegame_info->full_path);
        Memory_FreePointer(&savegame_info->level_title);
    }
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

        if (obj->save_position && obj->shadow_size) {
            int16_t room_num = item->room_num;
            const SECTOR *const sector = Room_GetSector(
                item->pos.x, item->pos.y, item->pos.z, &room_num);
            item->floor =
                Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
        }

        if (obj->save_flags) {
            item->flags &= 0xFF00;

            if (obj->collision == PuzzleHole_Collision
                && (item->status == IS_DEACTIVATED
                    || item->status == IS_ACTIVE)) {
                item->object_id += O_PUZZLE_DONE_1 - O_PUZZLE_HOLE_1;
            }

            if (obj->control == Pod_Control && item->status == IS_DEACTIVATED) {
                item->mesh_bits = 0x1FF;
                item->collidable = 0;
            }

            if ((obj->collision == Pickup_Collision
                 || obj->collision == SaveCrystal_Collision
                 || obj->collision == Scion1_Collision)
                && item->status == IS_DEACTIVATED) {
                Item_RemoveDrawn(i);
            }
        }

        if (obj->control == MovableBlock_Control) {
            item->priv =
                item->status == IS_ACTIVE ? (void *)true : (void *)false;
            if (item->status == IS_INACTIVE) {
                Room_AlterFloorHeight(item, -WALL_L);
            }
        }

        if (obj->control == SlidingPillar_Control
            && item->current_anim_state != SPS_MOVING) {
            Room_AlterFloorHeight(item, -WALL_L * 2);
        }

        if (item->object_id == O_PIERRE && item->hit_points <= 0
            && (item->flags & IF_ONE_SHOT)) {
            g_MusicTrackFlags[MX_PIERRE_SPEECH] |= IF_ONE_SHOT;
        }

        if (item->object_id == O_COWBOY && item->hit_points <= 0) {
            g_MusicTrackFlags[MX_COWBOY_SPEECH] |= IF_ONE_SHOT;
        }

        if (item->object_id == O_BALDY && item->hit_points <= 0) {
            g_MusicTrackFlags[MX_BALDY_SPEECH] |= IF_ONE_SHOT;
        }

        if (item->object_id == O_LARSON && item->hit_points <= 0) {
            g_MusicTrackFlags[MX_BALDY_SPEECH] |= IF_ONE_SHOT;
        }
    }

    if (g_GameInfo.bonus_flag) {
        g_Config.profile.new_game_plus_unlock = true;
    }

    LOT_ClearLOT(&g_Lara.lot);
}

void Savegame_Init(void)
{
    g_GameInfo.current = Memory_Alloc(
        sizeof(RESUME_INFO)
        * (GF_GetLevelTable(GFLT_MAIN)->count
           + (GF_GetLevelTable(GFLT_DEMOS)->count >= 0 ? 1 : 0)));
    m_SaveSlots = g_Config.gameplay.maximum_save_slots;
    m_SavegameInfo = Memory_Alloc(sizeof(SAVEGAME_INFO) * m_SaveSlots);

    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_DEMOS);
    for (int32_t i = 0; i < level_table->count; i++) {
        RESUME_INFO *const resume_info =
            Savegame_GetCurrentInfo(&level_table->levels[i]);
        resume_info->flags.available = 1;
        resume_info->flags.costume = 0;
        resume_info->num_medis = 0;
        resume_info->num_big_medis = 0;
        resume_info->num_scions = 0;
        resume_info->flags.got_pistols = 1;
        resume_info->flags.got_shotgun = 0;
        resume_info->flags.got_magnums = 0;
        resume_info->flags.got_uzis = 0;
        resume_info->pistol_ammo = 1000;
        resume_info->shotgun_ammo = 0;
        resume_info->magnum_ammo = 0;
        resume_info->uzi_ammo = 0;
        resume_info->gun_status = LGS_ARMLESS;
        resume_info->equipped_gun_type = LGT_PISTOLS;
        resume_info->holsters_gun_type = LGT_PISTOLS;
        resume_info->back_gun_type = LGT_UNARMED;
        resume_info->lara_hitpoints = LARA_MAX_HITPOINTS;
    }
}

RESUME_INFO *Savegame_GetCurrentInfo(const GF_LEVEL *const level)
{
    ASSERT(level != nullptr);
    if (GF_GetLevelTableType(level->type) == GFLT_MAIN) {
        return &g_GameInfo.current[level->num];
    } else if (level->type == GFL_DEMO) {
        return &g_GameInfo.current[GF_GetLevelTable(GFLT_MAIN)->count];
    }
    LOG_WARNING(
        "Warning: unable to get resume info for level %d (type=%s)", level->num,
        ENUM_MAP_TO_STRING(GF_LEVEL_TYPE, level->type));
    return nullptr;
}

void Savegame_Shutdown(void)
{
    M_Clear();
    Memory_FreePointer(&m_SavegameInfo);
    Memory_FreePointer(&g_GameInfo.current);
}

bool Savegame_IsInitialised(void)
{
    return m_SavegameInfo != nullptr;
}

void Savegame_ProcessItemsBeforeLoad(void)
{
    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        ITEM *const item = Item_Get(i);
        const OBJECT *const obj = Object_Get(item->object_id);

        if (obj->control == MovableBlock_Control && item->status != IS_INVISIBLE
            && item->pos.y >= Item_GetHeight(item)) {
            Room_AlterFloorHeight(item, WALL_L);
        }
        if (obj->control == SlidingPillar_Control) {
            Room_AlterFloorHeight(item, WALL_L * 2);
        }
    }
}

void Savegame_ProcessItemsBeforeSave(void)
{
    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        ITEM *const item = Item_Get(i);
        const OBJECT *const obj = Object_Get(item->object_id);

        if (obj->control == SaveCrystal_Control && item->data) {
            // need to reset the crystal status
            item->status = IS_DEACTIVATED;
            item->data = nullptr;
            Item_RemoveDrawn(i);
        }
    }
}

void Savegame_InitCurrentInfo(void)
{
    g_GameInfo.death_count = 0;
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
        current->num_medis = 0;
        current->num_big_medis = 0;
        current->num_scions = 0;
        current->pistol_ammo = 0;
        current->shotgun_ammo = 0;
        current->magnum_ammo = 0;
        current->uzi_ammo = 0;
        current->flags.got_pistols = 0;
        current->flags.got_shotgun = 0;
        current->flags.got_magnums = 0;
        current->flags.got_uzis = 0;
        current->equipped_gun_type = LGT_UNARMED;
        current->holsters_gun_type = LGT_UNARMED;
        current->back_gun_type = LGT_UNARMED;
        current->gun_status = LGS_ARMLESS;
    }

    if (level == GF_GetFirstLevel()) {
        current->flags.available = 1;
        current->flags.costume = 0;
        current->num_medis = 0;
        current->num_big_medis = 0;
        current->num_scions = 0;
        current->pistol_ammo = 1000;
        current->shotgun_ammo = 0;
        current->magnum_ammo = 0;
        current->uzi_ammo = 0;
        current->flags.got_pistols = 1;
        current->flags.got_shotgun = 0;
        current->flags.got_magnums = 0;
        current->flags.got_uzis = 0;
        current->equipped_gun_type = LGT_PISTOLS;
        current->holsters_gun_type = LGT_PISTOLS;
        current->back_gun_type = LGT_UNARMED;
        current->gun_status = LGS_ARMLESS;
    }

    if ((g_GameInfo.bonus_flag & GBF_NGPLUS) && level != GF_GetGymLevel()) {
        current->flags.got_pistols = 1;
        current->flags.got_shotgun = 1;
        current->flags.got_magnums = 1;
        current->flags.got_uzis = 1;
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
            if (current->flags.got_pistols) {
                current->holsters_gun_type = LGT_PISTOLS;
            } else if (current->flags.got_magnums) {
                current->holsters_gun_type = LGT_MAGNUMS;
            } else if (current->flags.got_uzis) {
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
        if (current->flags.got_shotgun) {
            current->back_gun_type = LGT_SHOTGUN;
        } else {
            current->back_gun_type = LGT_UNARMED;
        }
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
    ASSERT(level != nullptr);
    RESUME_INFO *current = Savegame_GetCurrentInfo(level);

    current->lara_hitpoints = g_LaraItem->hit_points;
    current->flags.available = 1;
    current->flags.costume = 0;

    current->pistol_ammo = 1000;
    if (Inv_RequestItem(O_PISTOL_ITEM)) {
        current->flags.got_pistols = 1;
    } else {
        current->flags.got_pistols = 0;
    }

    if (Inv_RequestItem(O_MAGNUM_ITEM)) {
        current->magnum_ammo = g_Lara.magnums.ammo;
        current->flags.got_magnums = 1;
    } else {
        current->magnum_ammo =
            Inv_RequestItem(O_MAG_AMMO_ITEM) * MAGNUM_AMMO_QTY;
        current->flags.got_magnums = 0;
    }

    if (Inv_RequestItem(O_UZI_ITEM)) {
        current->uzi_ammo = g_Lara.uzis.ammo;
        current->flags.got_uzis = 1;
    } else {
        current->uzi_ammo = Inv_RequestItem(O_UZI_AMMO_ITEM) * UZI_AMMO_QTY;
        current->flags.got_uzis = 0;
    }

    if (Inv_RequestItem(O_SHOTGUN_ITEM)) {
        current->shotgun_ammo = g_Lara.shotgun.ammo;
        current->flags.got_shotgun = 1;
    } else {
        current->shotgun_ammo =
            Inv_RequestItem(O_SG_AMMO_ITEM) * SHOTGUN_AMMO_QTY;
        current->flags.got_shotgun = 0;
    }

    current->num_medis = Inv_RequestItem(O_MEDI_ITEM);
    current->num_big_medis = Inv_RequestItem(O_BIGMEDI_ITEM);
    current->num_scions = Inv_RequestItem(O_SCION_ITEM_1);

    current->equipped_gun_type = g_Lara.gun_type;
    current->holsters_gun_type = g_Lara.holsters_gun_type;
    current->back_gun_type = g_Lara.back_gun_type;
    if (g_Lara.gun_status == LGS_READY) {
        current->gun_status = LGS_READY;
    } else {
        current->gun_status = LGS_ARMLESS;
    }
}

int32_t Savegame_GetLevelNumber(const int32_t slot_num)
{
    return m_SavegameInfo[slot_num].level_num;
}

int32_t Savegame_GetSlotCount(void)
{
    return m_SaveSlots;
}

bool Savegame_IsSlotFree(const int32_t slot_num)
{
    return m_SavegameInfo[slot_num].level_num == -1;
}

bool Savegame_Load(const int32_t slot_num)
{
    GAME_INFO *const game_info = &g_GameInfo;
    SAVEGAME_INFO *savegame_info = &m_SavegameInfo[slot_num];
    ASSERT(savegame_info->format != 0);

    M_LoadPreprocess();

    bool ret = false;
    const SAVEGAME_STRATEGY *strategy = &m_Strategies[0];
    while (strategy->format) {
        if (savegame_info->format == strategy->format) {
            MYFILE *fp = File_Open(savegame_info->full_path, FILE_OPEN_READ);
            if (fp) {
                ret = strategy->load_from_file(fp, game_info);
                File_Close(fp);
            }
            break;
        }
        strategy++;
    }

    if (ret) {
        M_LoadPostprocess();
    }

    g_GameInfo.save_initial_version = m_SavegameInfo[slot_num].initial_version;

    return ret;
}

bool Savegame_Save(const int32_t slot_num)
{
    GAME_INFO *const game_info = &g_GameInfo;
    bool ret = true;
    Savegame_BindSlot(slot_num);

    File_CreateDirectory(SAVES_DIR);

    const GF_LEVEL *const current_level = Game_GetCurrentLevel();
    Savegame_PersistGameToCurrentInfo(current_level);

    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < level_table->count; i++) {
        const GF_LEVEL *const level = &level_table->levels[i];
        if (level->type == GFL_CURRENT) {
            game_info->current[i] = game_info->current[current_level->num];
        }
    }

    SAVEGAME_INFO *savegame_info = &m_SavegameInfo[slot_num];
    const SAVEGAME_STRATEGY *strategy = &m_Strategies[0];
    while (strategy->format) {
        if (strategy->allow_save) {
            char *filename = strategy->get_save_filename(slot_num);
            char *full_path =
                Memory_Alloc(strlen(SAVES_DIR) + strlen(filename) + 2);
            sprintf(full_path, "%s/%s", SAVES_DIR, filename);

            MYFILE *fp = File_Open(full_path, FILE_OPEN_WRITE);
            if (fp) {
                strategy->save_to_file(fp, game_info);
                savegame_info->format = strategy->format;
                Memory_FreePointer(&savegame_info->full_path);
                savegame_info->full_path = Memory_DupStr(File_GetPath(fp));
                savegame_info->counter = g_SaveCounter;
                savegame_info->level_num = current_level->num;
                File_Close(fp);
            } else {
                ret = false;
            }

            Memory_FreePointer(&filename);
            Memory_FreePointer(&full_path);
        }
        strategy++;
    }

    if (ret) {
        REQUEST_INFO *req = &g_SavegameRequester;
        Requester_ChangeItem(
            req, slot_num, false, "%s %d", current_level->title, g_SaveCounter);
    }

    Savegame_ScanSavedGames();

    return ret;
}

bool Savegame_UpdateDeathCounters(int32_t slot_num, GAME_INFO *game_info)
{
    ASSERT(game_info != nullptr);
    ASSERT(slot_num >= 0);
    SAVEGAME_INFO *savegame_info = &m_SavegameInfo[slot_num];
    ASSERT(savegame_info->format != 0);

    bool ret = false;
    const SAVEGAME_STRATEGY *strategy = &m_Strategies[0];
    while (strategy->format) {
        if (savegame_info->format == strategy->format) {
            MYFILE *fp =
                File_Open(savegame_info->full_path, FILE_OPEN_READ_WRITE);
            if (fp) {
                ret = strategy->update_death_counters(fp, game_info);
                File_Close(fp);
            } else
                break;
        }
        strategy++;
    }
    return ret;
}

bool Savegame_LoadOnlyResumeInfo(int32_t slot_num, GAME_INFO *game_info)
{
    ASSERT(game_info != nullptr);
    SAVEGAME_INFO *savegame_info = &m_SavegameInfo[slot_num];
    ASSERT(savegame_info->format != 0);

    bool ret = false;
    const SAVEGAME_STRATEGY *strategy = &m_Strategies[0];
    while (strategy->format) {
        if (savegame_info->format == strategy->format) {
            MYFILE *fp = File_Open(savegame_info->full_path, FILE_OPEN_READ);
            if (fp) {
                ret = strategy->load_only_resume_info(fp, game_info);
                File_Close(fp);
            }
            break;
        }
        strategy++;
    }

    g_GameInfo.save_initial_version = m_SavegameInfo[slot_num].initial_version;

    return ret;
}

void Savegame_ScanSavedGames(void)
{
    M_Clear();

    g_SaveCounter = 0;
    g_SavedGamesCount = 0;

    for (int i = 0; i < m_SaveSlots; i++) {
        SAVEGAME_INFO *savegame_info = &m_SavegameInfo[i];
        const SAVEGAME_STRATEGY *strategy = &m_Strategies[0];
        while (strategy->format) {
            if (!savegame_info->format && strategy->allow_load) {
                char *filename = strategy->get_save_filename(i);

                char *full_path =
                    Memory_Alloc(strlen(SAVES_DIR) + strlen(filename) + 2);
                sprintf(full_path, "%s/%s", SAVES_DIR, filename);

                MYFILE *fp = nullptr;
                if (!fp) {
                    fp = File_Open(full_path, FILE_OPEN_READ);
                }
                if (!fp) {
                    fp = File_Open(filename, FILE_OPEN_READ);
                }

                if (fp) {
                    if (strategy->fill_info(fp, savegame_info)) {
                        savegame_info->format = strategy->format;
                        Memory_FreePointer(&savegame_info->full_path);
                        savegame_info->full_path =
                            Memory_DupStr(File_GetPath(fp));
                    }
                    File_Close(fp);
                }

                Memory_FreePointer(&filename);
                Memory_FreePointer(&full_path);
            }
            strategy++;
        }

        if (savegame_info->level_title) {
            if (savegame_info->counter > g_SaveCounter) {
                g_SaveCounter = savegame_info->counter;
            }
            g_SavedGamesCount++;
        }
    }

    REQUEST_INFO *req = &g_SavegameRequester;
    Requester_ClearTextstrings(req);
    Requester_Init(&g_SavegameRequester, Savegame_GetSlotCount());

    for (int i = 0; i < req->max_items; i++) {
        SAVEGAME_INFO *savegame_info = &m_SavegameInfo[i];

        if (savegame_info->level_title) {
            if (savegame_info->counter == g_SaveCounter) {
                m_NewestSlot = i;
            }
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

    g_SaveCounter++;
}

void Savegame_ScanAvailableLevels(REQUEST_INFO *req)
{
    const int32_t slot_num = g_GameInfo.select_save_slot;
    if (slot_num == -1) {
        return;
    }

    const SAVEGAME_INFO *const savegame_info = &m_SavegameInfo[slot_num];
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

void Savegame_HighlightNewestSlot(void)
{
    g_SavegameRequester.requested = m_NewestSlot;
}

bool Savegame_RestartAvailable(int32_t slot_num)
{
    if (slot_num == -1) {
        return true;
    }

    SAVEGAME_INFO *savegame_info = &m_SavegameInfo[slot_num];
    return savegame_info->features.restart;
}
