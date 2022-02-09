#include "game/savegame.h"

#include "config.h"
#include "filesystem.h"
#include "game/ai/pod.h"
#include "game/control.h"
#include "game/gameflow.h"
#include "game/inv.h"
#include "game/items.h"
#include "game/objects/pickup.h"
#include "game/objects/puzzle_hole.h"
#include "game/savegame_bson.h"
#include "game/savegame_legacy.h"
#include "game/traps/movable_block.h"
#include "game/traps/rolling_block.h"
#include "global/vars.h"
#include "log.h"
#include "memory.h"

#include <assert.h>
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
    void (*save_to_file)(MYFILE *fp, GAME_INFO *game_info);
    bool (*update_death_counters)(MYFILE *fp, GAME_INFO *game_info);
} SAVEGAME_STRATEGY;

static BOX_NODE *m_OldLaraLOTNode;
static SAVEGAME_INFO m_SavegameInfo[MAX_SAVE_SLOTS] = { 0 };

static const SAVEGAME_STRATEGY m_Strategies[] = {
    {
        .allow_load = true,
        .allow_save = true,
        .format = SAVEGAME_FORMAT_BSON,
        .get_save_filename = Savegame_BSON_GetSaveFileName,
        .fill_info = Savegame_BSON_FillInfo,
        .load_from_file = Savegame_BSON_LoadFromFile,
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
        .save_to_file = Savegame_Legacy_SaveToFile,
        .update_death_counters = Savegame_Legacy_UpdateDeathCounters,
    },
    { 0 },
};

static void Savegame_LoadPreprocess();
static void Savegame_LoadPostrocess();

static void Savegame_LoadPreprocess()
{
    m_OldLaraLOTNode = g_Lara.LOT.node;

    for (int i = 0; i < g_LevelItemCount; i++) {
        ITEM_INFO *item = &g_Items[i];
        OBJECT_INFO *obj = &g_Objects[item->object_number];

        if (obj->control == MovableBlockControl) {
            AlterFloorHeight(item, WALL_L);
        }
        if (obj->control == RollingBlockControl) {
            AlterFloorHeight(item, WALL_L * 2);
        }
    }
}

static void Savegame_LoadPostProcess()
{
    for (int i = 0; i < g_LevelItemCount; i++) {
        ITEM_INFO *item = &g_Items[i];
        OBJECT_INFO *obj = &g_Objects[item->object_number];

        if (obj->save_position && obj->shadow_size) {
            int16_t room_num = item->room_number;
            FLOOR_INFO *floor =
                GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
            item->floor =
                GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
        }

        if (obj->save_flags) {
            item->flags &= 0xFF00;

            if (obj->collision == PuzzleHoleCollision
                && (item->status == IS_DEACTIVATED
                    || item->status == IS_ACTIVE)) {
                item->object_number += O_PUZZLE_DONE1 - O_PUZZLE_HOLE1;
            }

            if (obj->control == PodControl && item->status == IS_DEACTIVATED) {
                item->mesh_bits = 0x1FF;
                item->collidable = 0;
            }

            if (obj->collision == PickUpCollision
                && item->status == IS_DEACTIVATED) {
                RemoveDrawnItem(i);
            }
        }

        if (obj->control == MovableBlockControl
            && item->status == IS_NOT_ACTIVE) {
            AlterFloorHeight(item, -WALL_L);
        }

        if (obj->control == RollingBlockControl
            && item->current_anim_state != RBS_MOVING) {
            AlterFloorHeight(item, -WALL_L * 2);
        }

        if (item->object_number == O_PIERRE && item->hit_points <= 0
            && (item->flags & IF_ONESHOT)) {
            if (Inv_RequestItem(O_SCION_ITEM) == 1) {
                SpawnItem(item, O_MAGNUM_ITEM);
                SpawnItem(item, O_SCION_ITEM2);
                SpawnItem(item, O_KEY_ITEM1);
            }
            g_MusicTrackFlags[MX_PIERRE_SPEECH] |= IF_ONESHOT;
        }

        if (item->object_number == O_SKATEKID && item->hit_points <= 0) {
            if (!Inv_RequestItem(O_UZI_ITEM)) {
                SpawnItem(item, O_UZI_ITEM);
            }
        }

        if (item->object_number == O_COWBOY && item->hit_points <= 0) {
            if (!Inv_RequestItem(O_MAGNUM_ITEM)) {
                SpawnItem(item, O_MAGNUM_ITEM);
            }
            g_MusicTrackFlags[MX_COWBOY_SPEECH] |= IF_ONESHOT;
        }

        if (item->object_number == O_BALDY && item->hit_points <= 0) {
            if (!Inv_RequestItem(O_SHOTGUN_ITEM)) {
                SpawnItem(item, O_SHOTGUN_ITEM);
            }
            g_MusicTrackFlags[MX_BALDY_SPEECH] |= IF_ONESHOT;
        }

        if (item->object_number == O_LARSON && item->hit_points <= 0) {
            g_MusicTrackFlags[MX_BALDY_SPEECH] |= IF_ONESHOT;
        }
    }

    for (int i = 0; i < g_GameFlow.level_count; i++) {
        if (g_GameFlow.levels[i].level_type == GFL_CURRENT) {
            g_GameInfo.start[g_CurrentLevel] = g_GameInfo.start[i];
        }
    }

    g_Lara.LOT.node = m_OldLaraLOTNode;
    g_Lara.LOT.target_box = NO_BOX;
}

void Savegame_InitStartEndInfo()
{
    for (int i = 0; i < g_GameFlow.level_count; i++) {
        Savegame_ResetStartInfo(i);
        Savegame_ResetEndInfo(i);
        Savegame_ApplyLogicToStartInfo(i);
        g_GameInfo.start[i].flags.available = 0;
    }
    g_GameInfo.start[g_GameFlow.gym_level_num].flags.available = 1;
    g_GameInfo.start[g_GameFlow.first_level_num].flags.available = 1;
}

void Savegame_ResetStartInfo(int level_num)
{
    START_INFO *start = &g_GameInfo.start[level_num];
    memset(start, 0, sizeof(START_INFO));
    Savegame_ApplyLogicToStartInfo(level_num);
}

void Savegame_ApplyLogicToStartInfo(int level_num)
{
    START_INFO *start = &g_GameInfo.start[level_num];

    if (!g_Config.disable_healing_between_levels
        || level_num == g_GameFlow.gym_level_num
        || level_num == g_GameFlow.first_level_num) {
        start->lara_hitpoints = g_Config.start_lara_hitpoints;
    }

    if (level_num == g_GameFlow.gym_level_num) {
        start->flags.available = 1;
        start->flags.costume = 1;
        start->num_medis = 0;
        start->num_big_medis = 0;
        start->num_scions = 0;
        start->pistol_ammo = 0;
        start->shotgun_ammo = 0;
        start->magnum_ammo = 0;
        start->uzi_ammo = 0;
        start->flags.got_pistols = 0;
        start->flags.got_shotgun = 0;
        start->flags.got_magnums = 0;
        start->flags.got_uzis = 0;
        start->gun_type = LGT_UNARMED;
        start->gun_status = LGS_ARMLESS;
    }

    if (level_num == g_GameFlow.first_level_num) {
        start->flags.available = 1;
        start->flags.costume = 0;
        start->num_medis = 0;
        start->num_big_medis = 0;
        start->num_scions = 0;
        start->pistol_ammo = 1000;
        start->shotgun_ammo = 0;
        start->magnum_ammo = 0;
        start->uzi_ammo = 0;
        start->flags.got_pistols = 1;
        start->flags.got_shotgun = 0;
        start->flags.got_magnums = 0;
        start->flags.got_uzis = 0;
        start->gun_type = LGT_PISTOLS;
        start->gun_status = LGS_ARMLESS;
    }

    if ((g_GameInfo.bonus_flag & GBF_NGPLUS)
        && level_num != g_GameFlow.gym_level_num) {
        start->flags.got_pistols = 1;
        start->flags.got_shotgun = 1;
        start->flags.got_magnums = 1;
        start->flags.got_uzis = 1;
        start->shotgun_ammo = 1234;
        start->magnum_ammo = 1234;
        start->uzi_ammo = 1234;
        start->gun_type = LGT_UZIS;
    }
}

void Savegame_PersistGameToStartInfo(int level_num)
{
    // Persist Lara's inventory to the start info.
    // Used to carry over Lara's inventory between levels.

    START_INFO *start = &g_GameInfo.start[level_num];

    if (g_LaraItem) {
        start->lara_hitpoints = g_LaraItem->hit_points;
    } else {
        // Carry over variables from previous levels if the current level
        // has no Lara object (such as the cutscene levels)
        for (int l = level_num - 1; l >= 0; l--) {
            START_INFO *prev_start = &g_GameInfo.start[l];
            if (g_GameFlow.levels[l].level_type == GFL_NORMAL) {
                start->lara_hitpoints = prev_start->lara_hitpoints;
                break;
            }
        }
    }

    start->flags.available = 1;
    start->flags.costume = 0;

    start->pistol_ammo = 1000;
    if (Inv_RequestItem(O_GUN_ITEM)) {
        start->flags.got_pistols = 1;
    } else {
        start->flags.got_pistols = 0;
    }

    if (Inv_RequestItem(O_MAGNUM_ITEM)) {
        start->magnum_ammo = g_Lara.magnums.ammo;
        start->flags.got_magnums = 1;
    } else {
        start->magnum_ammo = Inv_RequestItem(O_MAG_AMMO_ITEM) * MAGNUM_AMMO_QTY;
        start->flags.got_magnums = 0;
    }

    if (Inv_RequestItem(O_UZI_ITEM)) {
        start->uzi_ammo = g_Lara.uzis.ammo;
        start->flags.got_uzis = 1;
    } else {
        start->uzi_ammo = Inv_RequestItem(O_UZI_AMMO_ITEM) * UZI_AMMO_QTY;
        start->flags.got_uzis = 0;
    }

    if (Inv_RequestItem(O_SHOTGUN_ITEM)) {
        start->shotgun_ammo = g_Lara.shotgun.ammo;
        start->flags.got_shotgun = 1;
    } else {
        start->shotgun_ammo =
            Inv_RequestItem(O_SG_AMMO_ITEM) * SHOTGUN_AMMO_QTY;
        start->flags.got_shotgun = 0;
    }

    start->num_medis = Inv_RequestItem(O_MEDI_ITEM);
    start->num_big_medis = Inv_RequestItem(O_BIGMEDI_ITEM);
    start->num_scions = Inv_RequestItem(O_SCION_ITEM);

    start->gun_type = g_Lara.gun_type;
    if (g_Lara.gun_status == LGS_READY) {
        start->gun_status = LGS_READY;
    } else {
        start->gun_status = LGS_ARMLESS;
    }
}

void Savegame_ResetEndInfo(int level_num)
{
    END_INFO *end = &g_GameInfo.end[level_num];
    memset(end, 0, sizeof(END_INFO));
}

void Savegame_PersistGameToEndInfo(int level_num)
{
    END_INFO *end = &g_GameInfo.end[level_num];
    end->stats = g_GameInfo.stats;
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
        Savegame_LoadPostProcess();
    }

    return ret;
}

bool Savegame_Save(int32_t slot_num, GAME_INFO *game_info)
{
    assert(game_info);
    bool ret = true;

    File_CreateDirectory(SAVES_DIR);

    Savegame_PersistGameToStartInfo(g_CurrentLevel);
    Savegame_PersistGameToEndInfo(g_CurrentLevel);

    for (int i = 0; i < g_GameFlow.level_count; i++) {
        if (g_GameFlow.levels[i].level_type == GFL_CURRENT) {
            game_info->start[i] = game_info->start[g_CurrentLevel];
        }
    }

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
        REQUEST_INFO *req = &g_LoadSavegameRequester;
        req->item_flags[slot_num] &= ~RIF_BLOCKED;
        sprintf(
            &req->item_texts[req->item_text_len * slot_num], "%s %d",
            g_GameFlow.levels[g_CurrentLevel].level_title, g_SaveCounter);
        g_SavedGamesCount++;
        g_SaveCounter++;
        game_info->current_save_slot = slot_num;
    }

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

void Savegame_Shutdown()
{
    for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
        SAVEGAME_INFO *savegame_info = &m_SavegameInfo[i];
        savegame_info->format = 0;
        savegame_info->counter = -1;
        savegame_info->level_num = -1;
        Memory_FreePointer(&savegame_info->full_path);
        Memory_FreePointer(&savegame_info->level_title);
    }
}

void Savegame_ScanSavedGames()
{
    Savegame_Shutdown();

    g_SaveCounter = 0;
    g_SavedGamesCount = 0;

    for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
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
                        savegame_info->full_path = Memory_Dup(File_GetPath(fp));
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

    REQUEST_INFO *req = &g_LoadSavegameRequester;

    req->items = 0;
    for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
        SAVEGAME_INFO *savegame_info = &m_SavegameInfo[i];

        if (savegame_info->level_title) {
            req->item_flags[req->items] &= ~RIF_BLOCKED;

            sprintf(
                &req->item_texts[req->items * req->item_text_len], "%s %d",
                savegame_info->level_title, savegame_info->counter);

            if (savegame_info->counter == g_SaveCounter) {
                req->requested = i;
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
