#include "game/savegame.h"

#include "filesystem.h"
#include "game/gameflow.h"
#include "game/inv.h"
#include "game/savegame_legacy.h"
#include "global/vars.h"
#include "memory.h"

#include <assert.h>
#include <stdio.h>

// Loading a saved game is divided into two phases. First, the game reads the
// savegame file contents to look for the level number. The rest of the save
// data is stored in a special buffer in the g_GameInfo. Then the engine
// continues to execute the normal game flow and loads the specified level.
// Second phase occurs after everything finishes loading, e.g. items,
// creatures, triggers etc., and is what actually sets Lara's health, creatures
// status, triggers, inventory etc.

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

int16_t SaveGame_LoadSaveBufferFromFile(GAME_INFO *game_info, int32_t slot)
{
    assert(game_info);

    int16_t level_num = -1;
    char *filename;

    filename = SaveGame_Legacy_GetSavePath(slot);

    MYFILE *fp = File_Open(filename, FILE_OPEN_READ);
    if (fp) {
        File_Read(
            &game_info->savegame_buffer[0], sizeof(char), MAX_SAVEGAME_BUFFER,
            fp);
        level_num = SaveGame_Legacy_GetLevelNumber(fp);
        File_Close(fp);
    }

    Memory_FreePointer(&filename);

    return level_num;
}

void SaveGame_ApplySaveBuffer(GAME_INFO *game_info)
{
    SaveGame_Legacy_ApplySaveBuffer(game_info);
}

bool SaveGame_SaveToFile(GAME_INFO *game_info, int32_t slot)
{
    assert(game_info);
    bool ret = true;

    CreateStartInfo(g_CurrentLevel);

    SaveGame_Legacy_FillSaveBuffer(game_info);

    char *filename = SaveGame_Legacy_GetSavePath(slot);

    MYFILE *fp = File_Open(filename, FILE_OPEN_WRITE);
    if (fp) {
        File_Write(
            &game_info->savegame_buffer[0], sizeof(char),
            game_info->savegame_buffer_size, fp);
        File_Close(fp);
    } else {
        ret = false;
    }

    Memory_FreePointer(&filename);

    if (ret) {
        REQUEST_INFO *req = &g_LoadSaveGameRequester;
        req->item_flags[slot] &= ~RIF_BLOCKED;
        sprintf(
            &req->item_texts[req->item_text_len * slot], "%s %d",
            g_GameFlow.levels[g_CurrentLevel].level_title, g_SaveCounter);
        g_SavedGamesCount++;
        g_SaveCounter++;
    }

    return ret;
}

void SaveGame_ScanSavedGames()
{
    REQUEST_INFO *req = &g_LoadSaveGameRequester;

    req->items = 0;
    g_SaveCounter = 0;
    g_SavedGamesCount = 0;
    for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
        char *filename = SaveGame_Legacy_GetSavePath(i);
        char *level_title = NULL;
        int32_t counter = -1;

        MYFILE *fp = File_Open(filename, FILE_OPEN_READ);
        if (fp) {
            level_title = SaveGame_Legacy_GetLevelTitle(fp);
            counter = SaveGame_Legacy_GetSaveCounter(fp);
            File_Close(fp);
        }

        if (level_title) {
            req->item_flags[req->items] &= ~RIF_BLOCKED;

            sprintf(
                &req->item_texts[req->items * req->item_text_len], "%s %d",
                level_title, counter);

            if (counter > g_SaveCounter) {
                g_SaveCounter = counter;
                req->requested = i;
            }

            g_SavedGamesCount++;
        } else {
            req->item_flags[req->items] |= RIF_BLOCKED;

            sprintf(
                &req->item_texts[req->items * req->item_text_len],
                g_GameFlow.strings[GS_MISC_EMPTY_SLOT_FMT], i + 1);
        }

        Memory_FreePointer(&level_title);
        Memory_FreePointer(&filename);

        req->items++;
    }

    if (req->requested >= req->vis_lines) {
        req->line_offset = req->requested - req->vis_lines + 1;
    } else if (req->requested < req->line_offset) {
        req->line_offset = req->requested;
    }

    g_SaveCounter++;
}
