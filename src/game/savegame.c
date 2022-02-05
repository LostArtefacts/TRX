#include "game/savegame.h"

#include "filesystem.h"
#include "game/gameflow.h"
#include "game/inv.h"
#include "game/savegame_bson.h"
#include "game/savegame_legacy.h"
#include "global/vars.h"
#include "log.h"
#include "memory.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

// Loading a saved game is divided into two phases. First, the game reads the
// savegame file contents to look for the level number. The rest of the save
// data is stored in a special buffer in the g_GameInfo. Then the engine
// continues to execute the normal game flow and loads the specified level.
// Second phase occurs after everything finishes loading, e.g. items,
// creatures, triggers etc., and is what actually sets Lara's health, creatures
// status, triggers, inventory etc.

typedef enum SAVEGAME_FORMAT {
    SAVEGAME_FORMAT_LEGACY = 1,
    SAVEGAME_FORMAT_BSON = 2,
} SAVEGAME_FORMAT;

typedef struct SAVEGAME_INFO {
    SAVEGAME_FORMAT format;
    int32_t counter;
    int32_t level_num;
    char *level_title;
} SAVEGAME_INFO;

typedef struct SAVEGAME_STRATEGY {
    bool allow_load;
    bool allow_save;
    SAVEGAME_FORMAT format;
    char *(*get_save_path)(int32_t slot_num);
    int16_t (*get_level_number)(MYFILE *fp);
    int32_t (*get_save_counter)(MYFILE *fp);
    char *(*get_level_title)(MYFILE *fp);
    bool (*load_from_file)(MYFILE *fp, GAME_INFO *game_info);
    void (*save_to_file)(MYFILE *fp, GAME_INFO *game_info);
} SAVEGAME_STRATEGY;

static SAVEGAME_INFO m_SaveGameInfo[MAX_SAVE_SLOTS] = { 0 };

static const SAVEGAME_STRATEGY m_Strategies[] = {
    {
        .allow_load = true,
        .allow_save = true,
        .format = SAVEGAME_FORMAT_BSON,
        .get_save_path = SaveGame_BSON_GetSavePath,
        .get_level_number = SaveGame_BSON_GetLevelNumber,
        .get_save_counter = SaveGame_BSON_GetSaveCounter,
        .get_level_title = SaveGame_BSON_GetLevelTitle,
        .load_from_file = SaveGame_BSON_LoadFromFile,
        .save_to_file = SaveGame_BSON_SaveToFile,
    },
    {
        .allow_load = true,
        .allow_save = false,
        .format = SAVEGAME_FORMAT_LEGACY,
        .get_save_path = SaveGame_Legacy_GetSavePath,
        .get_level_number = SaveGame_Legacy_GetLevelNumber,
        .get_save_counter = SaveGame_Legacy_GetSaveCounter,
        .get_level_title = SaveGame_Legacy_GetLevelTitle,
        .load_from_file = SaveGame_Legacy_LoadFromFile,
        .save_to_file = SaveGame_Legacy_SaveToFile,
    },
    { 0 },
};

void InitialiseStartInfo()
{
    for (int i = 0; i < g_GameFlow.level_count; i++) {
        ModifyStartInfo(i);
        g_GameInfo.start[i].flags.available = 0;
    }
    g_GameInfo.start[g_GameFlow.gym_level_num].flags.available = 1;
    g_GameInfo.start[g_GameFlow.first_level_num].flags.available = 1;
}

void ModifyStartInfo(int32_t level_num)
{
    START_INFO *start = &g_GameInfo.start[level_num];

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

void CreateStartInfo(int level_num)
{
    START_INFO *start = &g_GameInfo.start[level_num];

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

int32_t SaveGame_GetLevelNumber(int32_t slot_num)
{
    return m_SaveGameInfo[slot_num].level_num;
}

bool SaveGame_Load(int32_t slot_num, GAME_INFO *game_info)
{
    assert(game_info);
    SAVEGAME_INFO *savegame_info = &m_SaveGameInfo[slot_num];
    assert(savegame_info->format);

    bool ret = false;
    const SAVEGAME_STRATEGY *strategy = &m_Strategies[0];
    while (strategy->format) {
        if (savegame_info->format == strategy->format) {
            char *filename = strategy->get_save_path(slot_num);
            MYFILE *fp = File_Open(filename, FILE_OPEN_READ);
            if (fp) {
                ret = strategy->load_from_file(fp, game_info);
                File_Close(fp);
            }
            Memory_FreePointer(&filename);
            break;
        }
        strategy++;
    }

    if (ret) {
        for (int i = 0; i < g_GameFlow.level_count; i++) {
            if (g_GameFlow.levels[i].level_type == GFL_CURRENT) {
                game_info->start[g_CurrentLevel] = game_info->start[i];
            }
        }
    }

    return ret;
}

bool SaveGame_Save(int32_t slot_num, GAME_INFO *game_info)
{
    assert(game_info);
    bool ret = true;

    CreateStartInfo(g_CurrentLevel);

    for (int i = 0; i < g_GameFlow.level_count; i++) {
        if (g_GameFlow.levels[i].level_type == GFL_CURRENT) {
            game_info->start[i] = game_info->start[g_CurrentLevel];
        }
    }

    const SAVEGAME_STRATEGY *strategy = &m_Strategies[0];
    while (strategy->format) {
        if (strategy->allow_save) {
            char *filename = strategy->get_save_path(slot_num);

            MYFILE *fp = File_Open(filename, FILE_OPEN_WRITE);
            if (fp) {
                strategy->save_to_file(fp, game_info);
                File_Close(fp);
            } else {
                ret = false;
            }

            Memory_FreePointer(&filename);
        }
        strategy++;
    }

    if (ret) {
        REQUEST_INFO *req = &g_LoadSaveGameRequester;
        req->item_flags[slot_num] &= ~RIF_BLOCKED;
        sprintf(
            &req->item_texts[req->item_text_len * slot_num], "%s %d",
            g_GameFlow.levels[g_CurrentLevel].level_title, g_SaveCounter);
        g_SavedGamesCount++;
        g_SaveCounter++;
    }

    return ret;
}

void SaveGame_Shutdown()
{
    for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
        SAVEGAME_INFO *savegame_info = &m_SaveGameInfo[i];
        savegame_info->format = 0;
        savegame_info->counter = -1;
        savegame_info->level_num = -1;
        Memory_FreePointer(&savegame_info->level_title);
    }
}

void SaveGame_ScanSavedGames()
{
    SaveGame_Shutdown();

    g_SaveCounter = 0;
    g_SavedGamesCount = 0;

    for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
        SAVEGAME_INFO *savegame_info = &m_SaveGameInfo[i];

        const SAVEGAME_STRATEGY *strategy = &m_Strategies[0];
        while (strategy->format) {
            if (savegame_info->counter == -1 && strategy->allow_load) {
                char *filename = strategy->get_save_path(i);
                MYFILE *fp = File_Open(filename, FILE_OPEN_READ);
                if (fp) {
                    savegame_info->format = strategy->format;
                    savegame_info->counter = strategy->get_save_counter(fp);
                    savegame_info->level_num = strategy->get_level_number(fp);
                    savegame_info->level_title = strategy->get_level_title(fp);
                    File_Close(fp);
                }
                Memory_FreePointer(&filename);
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

    REQUEST_INFO *req = &g_LoadSaveGameRequester;

    req->items = 0;
    for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
        SAVEGAME_INFO *savegame_info = &m_SaveGameInfo[i];

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
