#include "game/game.h"

#include "config.h"
#include "filesystem.h"
#include "game/camera.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/music.h"
#include "game/output.h"
#include "game/savegame.h"
#include "game/screen.h"
#include "game/setup.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/const.h"
#include "global/vars.h"
#include "log.h"
#include "specific/s_misc.h"

#include <stdio.h>

int32_t StartGame(int32_t level_num, GAMEFLOW_LEVEL_TYPE level_type)
{
    g_CurrentLevel = level_num;
    if (level_type != GFL_SAVED) {
        InitialiseLevelFlags();
    }

    if (!InitialiseLevel(level_num, level_type)) {
        g_CurrentLevel = 0;
        return GF_EXIT_TO_TITLE;
    }

    return GF_NOP;
}

int32_t StopGame()
{
    if (g_LevelComplete) {
        S_FadeInInventory(1);
        return GF_LEVEL_COMPLETE | g_CurrentLevel;
    }

    S_FadeToBlack();
    if (!g_InvChosen) {
        return GF_EXIT_TO_TITLE;
    }

    if (g_InvExtraData[0] == 0) {
        return GF_START_SAVED_GAME | g_InvExtraData[1];
    } else if (g_InvExtraData[0] == 1) {
        return GF_START_GAME | g_GameFlow.first_level_num;
    } else {
        return GF_EXIT_TO_TITLE;
    }
}

int32_t GameLoop(int32_t demo_mode)
{
    g_NoInputCount = 0;
    g_ResetFlag = false;
    g_OverlayFlag = 1;
    InitialiseCamera();

    int32_t nframes = 1;
    int32_t ret;
    while (1) {
        ret = ControlPhase(nframes, demo_mode);
        if (ret != GF_NOP) {
            break;
        }
        nframes = DrawPhaseGame();
    }

    Sound_StopAllSamples();
    Music_Stop();
    Music_SetVolume(g_Config.music_volume);

    if (ret == GF_NOP_BREAK) {
        return GF_NOP;
    }

    return ret;
}

int32_t LevelCompleteSequence(int32_t level_num)
{
    return GF_EXIT_TO_TITLE;
}

void LevelStats(int32_t level_num)
{
    char string[100];
    char time_str[100];
    TEXTSTRING *txt;

    Screen_ApplyResolution();
    Text_RemoveAll();

    // heading
    sprintf(string, "%s", g_GameFlow.levels[level_num].level_title);
    txt = Text_Create(0, -50, string);
    Text_CentreH(txt, 1);
    Text_CentreV(txt, 1);

    // time taken
    int32_t seconds = g_SaveGame.timer / 30;
    int32_t hours = seconds / 3600;
    int32_t minutes = (seconds / 60) % 60;
    seconds %= 60;
    if (hours) {
        sprintf(
            time_str, "%d:%d%d:%d%d", hours, minutes / 10, minutes % 10,
            seconds / 10, seconds % 10);
    } else {
        sprintf(time_str, "%d:%d%d", minutes, seconds / 10, seconds % 10);
    }
    sprintf(string, g_GameFlow.strings[GS_STATS_TIME_TAKEN_FMT], time_str);
    txt = Text_Create(0, 70, string);
    Text_CentreH(txt, 1);
    Text_CentreV(txt, 1);

    // secrets
    int32_t secrets_taken = 0;
    int32_t secrets_total = MAX_SECRETS;
    do {
        if (g_SaveGame.secrets & 1) {
            secrets_taken++;
        }
        g_SaveGame.secrets >>= 1;
        secrets_total--;
    } while (secrets_total);
    sprintf(
        string, g_GameFlow.strings[GS_STATS_SECRETS_FMT], secrets_taken,
        g_GameFlow.levels[level_num].secrets);
    txt = Text_Create(0, 40, string);
    Text_CentreH(txt, 1);
    Text_CentreV(txt, 1);

    // pickups
    sprintf(
        string, g_GameFlow.strings[GS_STATS_PICKUPS_FMT], g_SaveGame.pickups);
    txt = Text_Create(0, 10, string);
    Text_CentreH(txt, 1);
    Text_CentreV(txt, 1);

    // kills
    sprintf(string, g_GameFlow.strings[GS_STATS_KILLS_FMT], g_SaveGame.kills);
    txt = Text_Create(0, -20, string);
    Text_CentreH(txt, 1);
    Text_CentreV(txt, 1);

    // wait till action key release
    while (g_Input.select || g_Input.deselect) {
        Input_Update();
        Output_InitialisePolyList();
        Output_CopyBufferToScreen();
        Input_Update();
        Text_Draw();
        Output_DumpScreen();
    }

    // wait till action or escape key press
    while (!g_Input.select && !g_Input.deselect) {
        if (g_ResetFlag) {
            break;
        }
        Output_InitialisePolyList();
        Output_CopyBufferToScreen();
        Input_Update();
        Text_Draw();
        Output_DumpScreen();
    }

    // wait till escape key release
    while (g_Input.deselect) {
        Output_InitialisePolyList();
        Output_CopyBufferToScreen();
        Input_Update();
        Text_Draw();
        Output_DumpScreen();
    }

    if (level_num == g_GameFlow.last_level_num) {
        g_SaveGame.bonus_flag = GBF_NGPLUS;
    } else {
        CreateStartInfo(level_num + 1);
        ModifyStartInfo(level_num + 1);
    }

    g_SaveGame.start[g_CurrentLevel].available = 0;
    S_FadeToBlack();
    Screen_ApplyResolution();
}

int32_t S_LoadGame(SAVEGAME_INFO *save, int32_t slot)
{
    char filename[80];
    sprintf(filename, g_GameFlow.save_game_fmt, slot);
    LOG_DEBUG("%s", filename);
    MYFILE *fp = File_Open(filename, FILE_OPEN_READ);
    if (!fp) {
        return 0;
    }
    File_Read(filename, sizeof(char), 75, fp);
    int32_t counter;

    File_Read(&counter, sizeof(int32_t), 1, fp);

    if (!save->start) {
        Shell_ExitSystem("null save->start");
        return 0;
    }
    File_Read(&save->start[0], sizeof(START_INFO), g_GameFlow.level_count, fp);
    File_Read(&save->timer, sizeof(uint32_t), 1, fp);
    File_Read(&save->kills, sizeof(uint32_t), 1, fp);
    File_Read(&save->secrets, sizeof(uint16_t), 1, fp);
    File_Read(&save->current_level, sizeof(uint16_t), 1, fp);
    File_Read(&save->pickups, sizeof(uint8_t), 1, fp);
    File_Read(&save->bonus_flag, sizeof(uint8_t), 1, fp);
    File_Read(&save->num_pickup1, sizeof(uint8_t), 1, fp);
    File_Read(&save->num_pickup2, sizeof(uint8_t), 1, fp);
    File_Read(&save->num_puzzle1, sizeof(uint8_t), 1, fp);
    File_Read(&save->num_puzzle2, sizeof(uint8_t), 1, fp);
    File_Read(&save->num_puzzle3, sizeof(uint8_t), 1, fp);
    File_Read(&save->num_puzzle4, sizeof(uint8_t), 1, fp);
    File_Read(&save->num_key1, sizeof(uint8_t), 1, fp);
    File_Read(&save->num_key2, sizeof(uint8_t), 1, fp);
    File_Read(&save->num_key3, sizeof(uint8_t), 1, fp);
    File_Read(&save->num_key4, sizeof(uint8_t), 1, fp);
    File_Read(&save->num_leadbar, sizeof(uint8_t), 1, fp);
    File_Read(&save->challenge_failed, sizeof(uint8_t), 1, fp);
    File_Read(&save->buffer[0], sizeof(char), MAX_SAVEGAME_BUFFER, fp);
    File_Close(fp);

    for (int i = 0; i < g_GameFlow.level_count; i++) {
        if (g_GameFlow.levels[i].level_type == GFL_CURRENT) {
            save->start[save->current_level] = save->start[i];
        }
    }

    return 1;
}

void GetSavedGamesList(REQUEST_INFO *req)
{
    int32_t height = Screen_GetResHeight();

    if (height <= 200) {
        req->y = -32;
        req->vis_lines = 5;
    } else if (height <= 384) {
        req->y = -62;
        req->vis_lines = 8;
    } else if (height <= 480) {
        req->y = -90;
        req->vis_lines = 10;
    } else {
        req->y = -100;
        req->vis_lines = 12;
    }

    if (req->requested >= req->vis_lines) {
        req->line_offset = req->requested - req->vis_lines + 1;
    }
}

int32_t S_FrontEndCheck()
{
    REQUEST_INFO *req = &g_LoadSaveGameRequester;

    req->items = 0;
    g_SaveCounter = 0;
    g_SavedGamesCount = 0;
    for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
        char filename[80];
        sprintf(filename, g_GameFlow.save_game_fmt, i);

        MYFILE *fp = File_Open(filename, FILE_OPEN_READ);
        if (fp) {
            File_Read(filename, sizeof(char), 75, fp);
            int32_t counter;
            File_Read(&counter, sizeof(int32_t), 1, fp);
            File_Close(fp);

            req->item_flags[req->items] &= ~RIF_BLOCKED;

            sprintf(
                &req->item_texts[req->items * req->item_text_len], "%s %d",
                filename, counter);

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

        req->items++;
    }

    g_SaveCounter++;
    return 1;
}

int32_t S_SaveGame(SAVEGAME_INFO *save, int32_t slot)
{
    char filename[80];
    sprintf(filename, g_GameFlow.save_game_fmt, slot);
    LOG_DEBUG("%s", filename);

    MYFILE *fp = File_Open(filename, FILE_OPEN_WRITE);
    if (!fp) {
        return 0;
    }

    for (int i = 0; i < g_GameFlow.level_count; i++) {
        if (g_GameFlow.levels[i].level_type == GFL_CURRENT) {
            save->start[i] = save->start[save->current_level];
        }
    }

    sprintf(
        filename, "%s",
        g_GameFlow.levels[g_SaveGame.current_level].level_title);
    File_Write(filename, sizeof(char), 75, fp);
    File_Write(&g_SaveCounter, sizeof(int32_t), 1, fp);

    if (!save->start) {
        Shell_ExitSystem("null save->start");
        return 0;
    }
    File_Write(&save->start[0], sizeof(START_INFO), g_GameFlow.level_count, fp);
    File_Write(&save->timer, sizeof(uint32_t), 1, fp);
    File_Write(&save->kills, sizeof(uint32_t), 1, fp);
    File_Write(&save->secrets, sizeof(uint16_t), 1, fp);
    File_Write(&save->current_level, sizeof(uint16_t), 1, fp);
    File_Write(&save->pickups, sizeof(uint8_t), 1, fp);
    File_Write(&save->bonus_flag, sizeof(uint8_t), 1, fp);
    File_Write(&save->num_pickup1, sizeof(uint8_t), 1, fp);
    File_Write(&save->num_pickup2, sizeof(uint8_t), 1, fp);
    File_Write(&save->num_puzzle1, sizeof(uint8_t), 1, fp);
    File_Write(&save->num_puzzle2, sizeof(uint8_t), 1, fp);
    File_Write(&save->num_puzzle3, sizeof(uint8_t), 1, fp);
    File_Write(&save->num_puzzle4, sizeof(uint8_t), 1, fp);
    File_Write(&save->num_key1, sizeof(uint8_t), 1, fp);
    File_Write(&save->num_key2, sizeof(uint8_t), 1, fp);
    File_Write(&save->num_key3, sizeof(uint8_t), 1, fp);
    File_Write(&save->num_key4, sizeof(uint8_t), 1, fp);
    File_Write(&save->num_leadbar, sizeof(uint8_t), 1, fp);
    File_Write(&save->challenge_failed, sizeof(uint8_t), 1, fp);
    File_Write(&save->buffer[0], sizeof(char), MAX_SAVEGAME_BUFFER, fp);
    File_Close(fp);

    REQUEST_INFO *req = &g_LoadSaveGameRequester;
    req->item_flags[slot] &= ~RIF_BLOCKED;
    sprintf(
        &req->item_texts[req->item_text_len * slot], "%s %d", filename,
        g_SaveCounter);
    g_SavedGamesCount++;
    g_SaveCounter++;
    return 1;
}
