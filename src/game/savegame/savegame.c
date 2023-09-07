#include "game/savegame.h"

#include "config.h"
#include "filesystem.h"
#include "game/gameflow.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/objects/creatures/pod.h"
#include "game/objects/general/pickup.h"
#include "game/objects/general/puzzle_hole.h"
#include "game/objects/general/save_crystal.h"
#include "game/objects/general/scion.h"
#include "game/objects/traps/movable_block.h"
#include "game/objects/traps/rolling_block.h"
#include "game/room.h"
#include "game/savegame/savegame_bson.h"
#include "game/savegame/savegame_legacy.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "memory.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SAVES_DIR "saves"

typedef struct SAVEGAME_STRATEGY {
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

static uint16_t m_NewestSlot = 0;
static SAVEGAME_INFO *m_SavegameInfo = NULL;

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

static void Savegame_LoadPreprocess(void);
static void Savegame_LoadPostprocess(void);

static void Savegame_LoadPreprocess(void)
{
    Savegame_InitCurrentInfo();
}

static void Savegame_LoadPostprocess(void)
{
    for (int i = 0; i < g_LevelItemCount; i++) {
        ITEM_INFO *item = &g_Items[i];
        OBJECT_INFO *obj = &g_Objects[item->object_number];

        if (obj->save_position && obj->shadow_size) {
            int16_t room_num = item->room_number;
            FLOOR_INFO *floor =
                Room_GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
            item->floor =
                Room_GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
        }

        if (obj->save_flags) {
            item->flags &= 0xFF00;

            if (obj->collision == PuzzleHole_Collision
                && (item->status == IS_DEACTIVATED
                    || item->status == IS_ACTIVE)) {
                item->object_number += O_PUZZLE_DONE1 - O_PUZZLE_HOLE1;
            }

            if (obj->control == Pod_Control && item->status == IS_DEACTIVATED) {
                item->mesh_bits = 0x1FF;
                item->collidable = 0;
            }

            if ((obj->collision == Pickup_Collision
                 || obj->collision == SaveCrystal_Collision
                 || obj->collision == Scion_Collision)
                && item->status == IS_DEACTIVATED) {
                Item_RemoveDrawn(i);
            }
        }

        if (obj->control == MovableBlock_Control
            && item->status == IS_NOT_ACTIVE) {
            Room_AlterFloorHeight(item, -WALL_L);
        }

        if (obj->control == RollingBlock_Control
            && item->current_anim_state != RBS_MOVING) {
            Room_AlterFloorHeight(item, -WALL_L * 2);
        }

        if (item->object_number == O_PIERRE && item->hit_points <= 0
            && (item->flags & IF_ONESHOT)) {
            if (Inv_RequestItem(O_SCION_ITEM) == 1) {
                Item_Spawn(item, O_MAGNUM_ITEM);
                Item_Spawn(item, O_SCION_ITEM2);
                Item_Spawn(item, O_KEY_ITEM1);
            }
            g_MusicTrackFlags[MX_PIERRE_SPEECH] |= IF_ONESHOT;
        }

        if (item->object_number == O_SKATEKID && item->hit_points <= 0) {
            if (!Inv_RequestItem(O_UZI_ITEM)) {
                Item_Spawn(item, O_UZI_ITEM);
            }
        }

        if (item->object_number == O_COWBOY && item->hit_points <= 0) {
            if (!Inv_RequestItem(O_MAGNUM_ITEM)) {
                Item_Spawn(item, O_MAGNUM_ITEM);
            }
            g_MusicTrackFlags[MX_COWBOY_SPEECH] |= IF_ONESHOT;
        }

        if (item->object_number == O_BALDY && item->hit_points <= 0) {
            if (!Inv_RequestItem(O_SHOTGUN_ITEM)) {
                Item_Spawn(item, O_SHOTGUN_ITEM);
            }
            g_MusicTrackFlags[MX_BALDY_SPEECH] |= IF_ONESHOT;
        }

        if (item->object_number == O_LARSON && item->hit_points <= 0) {
            g_MusicTrackFlags[MX_BALDY_SPEECH] |= IF_ONESHOT;
        }
    }

    if (g_GameInfo.bonus_flag) {
        g_Config.runtime.new_game_plus_unlock = true;
    }

    LOT_ClearLOT(&g_Lara.LOT);
}

void Savegame_Init(void)
{
    m_SavegameInfo =
        Memory_Alloc(sizeof(SAVEGAME_INFO) * g_Config.maximum_save_slots);
}

void Savegame_Shutdown(void)
{
    if (!m_SavegameInfo) {
        return;
    }

    for (int i = 0; i < g_Config.maximum_save_slots; i++) {
        SAVEGAME_INFO *savegame_info = &m_SavegameInfo[i];
        savegame_info->format = 0;
        savegame_info->counter = -1;
        savegame_info->level_num = -1;
        Memory_FreePointer(&savegame_info->full_path);
        Memory_FreePointer(&savegame_info->level_title);
    }

    Memory_FreePointer(&m_SavegameInfo);
}

void Savegame_PreprocessItems(void)
{
    for (int i = 0; i < g_LevelItemCount; i++) {
        ITEM_INFO *item = &g_Items[i];
        OBJECT_INFO *obj = &g_Objects[item->object_number];

        if (obj->control == MovableBlock_Control) {
            Room_AlterFloorHeight(item, WALL_L);
        }
        if (obj->control == RollingBlock_Control) {
            Room_AlterFloorHeight(item, WALL_L * 2);
        }
    }
}

void Savegame_InitCurrentInfo(void)
{
    for (int i = 0; i < g_GameFlow.level_count; i++) {
        Savegame_ResetCurrentInfo(i);
        Savegame_ApplyLogicToCurrentInfo(i);
        g_GameInfo.current[i].flags.available = 0;
    }
    if (g_GameFlow.gym_level_num != -1) {
        g_GameInfo.current[g_GameFlow.gym_level_num].flags.available = 1;
    }
    g_GameInfo.current[g_GameFlow.first_level_num].flags.available = 1;
}

void Savegame_ApplyLogicToCurrentInfo(int level_num)
{
    RESUME_INFO *current = &g_GameInfo.current[level_num];

    if (!g_Config.disable_healing_between_levels
        || level_num == g_GameFlow.gym_level_num
        || level_num == g_GameFlow.first_level_num) {
        current->lara_hitpoints = g_Config.start_lara_hitpoints;
    }

    if (level_num == g_GameFlow.gym_level_num) {
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
        current->gun_type = LGT_UNARMED;
        current->gun_status = LGS_ARMLESS;
    }

    if (level_num == g_GameFlow.first_level_num) {
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
        current->gun_type = LGT_PISTOLS;
        current->gun_status = LGS_ARMLESS;
    }

    if ((g_GameInfo.bonus_flag & GBF_NGPLUS)
        && level_num != g_GameFlow.gym_level_num) {
        current->flags.got_pistols = 1;
        current->flags.got_shotgun = 1;
        current->flags.got_magnums = 1;
        current->flags.got_uzis = 1;
        current->shotgun_ammo = 1234;
        current->magnum_ammo = 1234;
        current->uzi_ammo = 1234;
        current->gun_type = LGT_UZIS;
    }
}

void Savegame_ResetCurrentInfo(int level_num)
{
    RESUME_INFO *current = &g_GameInfo.current[level_num];
    memset(current, 0, sizeof(RESUME_INFO));
}

void Savegame_CarryCurrentInfoToNextLevel(int32_t src_level, int32_t dst_level)
{
    memcpy(
        &g_GameInfo.current[dst_level], &g_GameInfo.current[src_level],
        sizeof(RESUME_INFO));
}

void Savegame_PersistGameToCurrentInfo(int level_num)
{
    // Persist Lara's inventory to the current info.
    // Used to carry over Lara's inventory between levels.

    RESUME_INFO *current = &g_GameInfo.current[level_num];

    current->lara_hitpoints = g_LaraItem->hit_points;
    current->flags.available = 1;
    current->flags.costume = 0;

    current->pistol_ammo = 1000;
    if (Inv_RequestItem(O_GUN_ITEM)) {
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
    current->num_scions = Inv_RequestItem(O_SCION_ITEM);

    current->gun_type = g_Lara.gun_type;
    if (g_Lara.gun_status == LGS_READY) {
        current->gun_status = LGS_READY;
    } else {
        current->gun_status = LGS_ARMLESS;
    }
}

int32_t Savegame_GetLevelNumber(int32_t slot_num)
{
    return m_SavegameInfo[slot_num].level_num;
}

bool Savegame_Load(int32_t slot_num, GAME_INFO *game_info)
{
    assert(game_info);
    SAVEGAME_INFO *savegame_info = &m_SavegameInfo[slot_num];
    assert(savegame_info->format);

    Savegame_LoadPreprocess();

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
        Savegame_LoadPostprocess();
    }

    g_GameInfo.save_initial_version = m_SavegameInfo[slot_num].initial_version;

    return ret;
}

bool Savegame_Save(int32_t slot_num, GAME_INFO *game_info)
{
    assert(game_info);
    bool ret = true;

    File_CreateDirectory(SAVES_DIR);

    Savegame_PersistGameToCurrentInfo(g_CurrentLevel);

    for (int i = 0; i < g_GameFlow.level_count; i++) {
        if (g_GameFlow.levels[i].level_type == GFL_CURRENT) {
            game_info->current[i] = game_info->current[g_CurrentLevel];
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
                savegame_info->level_num = g_CurrentLevel;
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
        req->item_flags[slot_num] &= ~RIF_BLOCKED;
        sprintf(
            &req->item_texts[req->item_text_len * slot_num], "%s %d",
            g_GameFlow.levels[g_CurrentLevel].level_title, g_SaveCounter);
    }

    Savegame_ScanSavedGames();

    return ret;
}

bool Savegame_UpdateDeathCounters(int32_t slot_num, GAME_INFO *game_info)
{
    assert(game_info);
    assert(slot_num >= 0);
    SAVEGAME_INFO *savegame_info = &m_SavegameInfo[slot_num];
    assert(savegame_info->format);

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
    assert(game_info);
    SAVEGAME_INFO *savegame_info = &m_SavegameInfo[slot_num];
    assert(savegame_info->format);

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
    Savegame_Shutdown();
    Savegame_Init();

    g_SaveCounter = 0;
    g_SavedGamesCount = 0;

    for (int i = 0; i < g_Config.maximum_save_slots; i++) {
        SAVEGAME_INFO *savegame_info = &m_SavegameInfo[i];

        const SAVEGAME_STRATEGY *strategy = &m_Strategies[0];
        while (strategy->format) {
            if (!savegame_info->format && strategy->allow_load) {
                char *filename = strategy->get_save_filename(i);

                char *full_path =
                    Memory_Alloc(strlen(SAVES_DIR) + strlen(filename) + 2);
                sprintf(full_path, "%s/%s", SAVES_DIR, filename);

                MYFILE *fp = NULL;
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

    req->items = 0;
    for (int i = 0; i < g_Config.maximum_save_slots; i++) {
        SAVEGAME_INFO *savegame_info = &m_SavegameInfo[i];

        if (savegame_info->level_title) {
            req->item_flags[req->items] &= ~RIF_BLOCKED;

            sprintf(
                &req->item_texts[req->items * req->item_text_len], "%s %d",
                savegame_info->level_title, savegame_info->counter);

            if (savegame_info->counter == g_SaveCounter) {
                m_NewestSlot = i;
            }
        } else {
            req->item_flags[req->items] |= RIF_BLOCKED;
            sprintf(
                &req->item_texts[req->items * req->item_text_len],
                g_GameFlow.strings[GS_MISC_EMPTY_SLOT_FMT], i + 1);
        }

        req->items++;
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
    SAVEGAME_INFO *savegame_info =
        &m_SavegameInfo[g_GameInfo.current_save_slot];
    req->items = 0;

    if (!savegame_info->features.select_level) {
        req->item_flags[req->items] |= RIF_BLOCKED;
        sprintf(
            &req->item_texts[req->items * req->item_text_len],
            g_GameFlow.strings[GS_PASSPORT_LEGACY_SELECT_LEVEL_1]);
        req->items++;
        req->item_flags[req->items] |= RIF_BLOCKED;
        sprintf(
            &req->item_texts[req->items * req->item_text_len],
            g_GameFlow.strings[GS_PASSPORT_LEGACY_SELECT_LEVEL_2]);
        req->items++;
        req->requested = 0;
        req->line_offset = 0;
        return;
    }

    for (int i = g_GameFlow.first_level_num; i <= savegame_info->level_num;
         i++) {
        req->item_flags[req->items] &= ~RIF_BLOCKED;
        sprintf(
            &req->item_texts[req->items * req->item_text_len], "%s",
            g_GameFlow.levels[i].level_title);
        req->items++;
    }

    if (g_InvMode == INV_TITLE_MODE) {
        req->item_flags[req->items] &= ~RIF_BLOCKED;
        sprintf(
            &req->item_texts[req->items * req->item_text_len], "%s",
            g_GameFlow.strings[GS_PASSPORT_STORY_SO_FAR]);
        req->items++;
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

GAMEFLOW_OPTION Savegame_PlayAvailableStory(int32_t slot_num)
{
    SAVEGAME_INFO *savegame_info = &m_SavegameInfo[slot_num];

    int32_t gf_option = GF_START_GAME | g_GameFlow.first_level_num;

    bool loop_continue = true;
    while (loop_continue) {
        int32_t gf_direction = gf_option & ~((1 << 6) - 1);
        int32_t gf_param = gf_option & ((1 << 6) - 1);

        gf_option = GameFlow_StorySoFar(gf_param, savegame_info->level_num);

        if (gf_param >= savegame_info->level_num
            && gf_param <= g_GameFlow.last_level_num) {
            break;
        }

        if (gf_direction == GF_EXIT_GAME) {
            break;
        }
    }

    return GF_EXIT_TO_TITLE;
}
