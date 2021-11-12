#include "game/game.h"

#include "config.h"
#include "filesystem.h"
#include "game/camera.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/music.h"
#include "game/savegame.h"
#include "game/setup.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/const.h"
#include "global/vars.h"
#include "log.h"
#include "specific/display.h"
#include "specific/frontend.h"
#include "specific/init.h"
#include "specific/input.h"
#include "specific/output.h"

#include <stdio.h>

static int32_t Rand1 = 0xD371F947;
static int32_t Rand2 = 0xD371F947;

int32_t StartGame(int32_t level_num, GAMEFLOW_LEVEL_TYPE level_type)
{
    CurrentLevel = level_num;
    if (level_type != GFL_SAVED) {
        InitialiseLevelFlags();
    }

    if (!InitialiseLevel(level_num, level_type)) {
        CurrentLevel = 0;
        return GF_EXIT_TO_TITLE;
    }

    return GF_NOP;
}

int32_t StopGame()
{
    if (LevelComplete) {
        S_FadeInInventory(1);
        return GF_LEVEL_COMPLETE | CurrentLevel;
    }

    S_FadeToBlack();
    if (!InvChosen) {
        return GF_EXIT_TO_TITLE;
    }

    if (InvExtraData[0] == 0) {
        return GF_START_SAVED_GAME | InvExtraData[1];
    } else if (InvExtraData[0] == 1) {
        return GF_START_GAME | GF.first_level_num;
    } else {
        return GF_EXIT_TO_TITLE;
    }
}

int32_t GameLoop(int32_t demo_mode)
{
    NoInputCount = 0;
    ResetFlag = false;
    OverlayFlag = 1;
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
    Music_SetVolume(T1MConfig.music_volume);

    if (ret == GF_NOP_BREAK) {
        return GF_NOP;
    }

    return ret;
}

int32_t LevelCompleteSequence(int32_t level_num)
{
    return GF_EXIT_TO_TITLE;
}

void SeedRandomControl(int32_t seed)
{
    LOG_DEBUG("%d", seed);
    Rand1 = seed;
}

int32_t GetRandomControl()
{
    Rand1 = 0x41C64E6D * Rand1 + 0x3039;
    return (Rand1 >> 10) & 0x7FFF;
}

void SeedRandomDraw(int32_t seed)
{
    LOG_DEBUG("%d", seed);
    Rand2 = seed;
}

int32_t GetRandomDraw()
{
    Rand2 = 0x41C64E6D * Rand2 + 0x3039;
    return (Rand2 >> 10) & 0x7FFF;
}

void LevelStats(int32_t level_num)
{
    char string[100];
    char time_str[100];
    TEXTSTRING *txt;

    TempVideoAdjust(GetScreenSizeIdx());
    Text_RemoveAll();

    // heading
    sprintf(string, "%s", GF.levels[level_num].level_title);
    txt = Text_Create(0, -50, string);
    Text_CentreH(txt, 1);
    Text_CentreV(txt, 1);

    // time taken
    int32_t seconds = SaveGame.timer / 30;
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
    sprintf(string, GF.strings[GS_STATS_TIME_TAKEN_FMT], time_str);
    txt = Text_Create(0, 70, string);
    Text_CentreH(txt, 1);
    Text_CentreV(txt, 1);

    // secrets
    int32_t secrets_taken = 0;
    int32_t secrets_total = MAX_SECRETS;
    do {
        if (SaveGame.secrets & 1) {
            secrets_taken++;
        }
        SaveGame.secrets >>= 1;
        secrets_total--;
    } while (secrets_total);
    sprintf(
        string, GF.strings[GS_STATS_SECRETS_FMT], secrets_taken,
        GF.levels[level_num].secrets);
    txt = Text_Create(0, 40, string);
    Text_CentreH(txt, 1);
    Text_CentreV(txt, 1);

    // pickups
    sprintf(string, GF.strings[GS_STATS_PICKUPS_FMT], SaveGame.pickups);
    txt = Text_Create(0, 10, string);
    Text_CentreH(txt, 1);
    Text_CentreV(txt, 1);

    // kills
    sprintf(string, GF.strings[GS_STATS_KILLS_FMT], SaveGame.kills);
    txt = Text_Create(0, -20, string);
    Text_CentreH(txt, 1);
    Text_CentreV(txt, 1);

    // wait till action key release
    while (Input.select || Input.deselect) {
        S_UpdateInput();
        S_InitialisePolyList();
        S_CopyBufferToScreen();
        S_UpdateInput();
        Text_Draw();
        S_OutputPolyList();
        S_DumpScreen();
    }

    // wait till action or escape key press
    while (!Input.select && !Input.deselect) {
        if (ResetFlag) {
            break;
        }
        S_InitialisePolyList();
        S_CopyBufferToScreen();
        S_UpdateInput();
        Text_Draw();
        S_OutputPolyList();
        S_DumpScreen();
    }

    // wait till escape key release
    while (Input.deselect) {
        S_InitialisePolyList();
        S_CopyBufferToScreen();
        S_UpdateInput();
        Text_Draw();
        S_OutputPolyList();
        S_DumpScreen();
    }

    if (level_num == GF.last_level_num) {
        SaveGame.bonus_flag = GBF_NGPLUS;
    } else {
        CreateStartInfo(level_num + 1);
        ModifyStartInfo(level_num + 1);
    }

    SaveGame.start[CurrentLevel].available = 0;
    S_FadeToBlack();
    TempVideoRemove();
}

int32_t S_LoadGame(SAVEGAME_INFO *save, int32_t slot)
{
    char filename[80];
    sprintf(filename, GF.save_game_fmt, slot);
    LOG_DEBUG("%s", filename);
    MYFILE *fp = FileOpen(filename, FILE_OPEN_READ);
    if (!fp) {
        return 0;
    }
    FileRead(filename, sizeof(char), 75, fp);
    int32_t counter;

    FileRead(&counter, sizeof(int32_t), 1, fp);

    if (!save->start) {
        S_ExitSystem("null save->start");
        return 0;
    }
    FileRead(&save->start[0], sizeof(START_INFO), GF.level_count, fp);
    FileRead(&save->timer, sizeof(uint32_t), 1, fp);
    FileRead(&save->kills, sizeof(uint32_t), 1, fp);
    FileRead(&save->secrets, sizeof(uint16_t), 1, fp);
    FileRead(&save->current_level, sizeof(uint16_t), 1, fp);
    FileRead(&save->pickups, sizeof(uint8_t), 1, fp);
    FileRead(&save->bonus_flag, sizeof(uint8_t), 1, fp);
    FileRead(&save->num_pickup1, sizeof(uint8_t), 1, fp);
    FileRead(&save->num_pickup2, sizeof(uint8_t), 1, fp);
    FileRead(&save->num_puzzle1, sizeof(uint8_t), 1, fp);
    FileRead(&save->num_puzzle2, sizeof(uint8_t), 1, fp);
    FileRead(&save->num_puzzle3, sizeof(uint8_t), 1, fp);
    FileRead(&save->num_puzzle4, sizeof(uint8_t), 1, fp);
    FileRead(&save->num_key1, sizeof(uint8_t), 1, fp);
    FileRead(&save->num_key2, sizeof(uint8_t), 1, fp);
    FileRead(&save->num_key3, sizeof(uint8_t), 1, fp);
    FileRead(&save->num_key4, sizeof(uint8_t), 1, fp);
    FileRead(&save->num_leadbar, sizeof(uint8_t), 1, fp);
    FileRead(&save->challenge_failed, sizeof(uint8_t), 1, fp);
    FileRead(&save->buffer[0], sizeof(char), MAX_SAVEGAME_BUFFER, fp);
    FileClose(fp);

    for (int i = 0; i < GF.level_count; i++) {
        if (GF.levels[i].level_type == GFL_CURRENT) {
            save->start[save->current_level] = save->start[i];
        }
    }

    return 1;
}

void GetSavedGamesList(REQUEST_INFO *req)
{
    int32_t height = GetScreenHeight();

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
    REQUEST_INFO *req = &LoadSaveGameRequester;

    req->items = 0;
    SaveCounter = 0;
    SavedGamesCount = 0;
    for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
        char filename[80];
        sprintf(filename, GF.save_game_fmt, i);

        MYFILE *fp = FileOpen(filename, FILE_OPEN_READ);
        if (fp) {
            FileRead(filename, sizeof(char), 75, fp);
            int32_t counter;
            FileRead(&counter, sizeof(int32_t), 1, fp);
            FileClose(fp);

            req->item_flags[req->items] &= ~RIF_BLOCKED;

            sprintf(
                &req->item_texts[req->items * req->item_text_len], "%s %d",
                filename, counter);

            if (counter > SaveCounter) {
                SaveCounter = counter;
                req->requested = i;
            }

            SavedGamesCount++;
        } else {
            req->item_flags[req->items] |= RIF_BLOCKED;

            sprintf(
                &req->item_texts[req->items * req->item_text_len],
                GF.strings[GS_MISC_EMPTY_SLOT_FMT], i + 1);
        }

        req->items++;
    }

    SaveCounter++;
    return 1;
}

int32_t S_SaveGame(SAVEGAME_INFO *save, int32_t slot)
{
    char filename[80];
    sprintf(filename, GF.save_game_fmt, slot);
    LOG_DEBUG("%s", filename);

    MYFILE *fp = FileOpen(filename, FILE_OPEN_WRITE);
    if (!fp) {
        return 0;
    }

    for (int i = 0; i < GF.level_count; i++) {
        if (GF.levels[i].level_type == GFL_CURRENT) {
            save->start[i] = save->start[save->current_level];
        }
    }

    sprintf(filename, "%s", GF.levels[SaveGame.current_level].level_title);
    FileWrite(filename, sizeof(char), 75, fp);
    FileWrite(&SaveCounter, sizeof(int32_t), 1, fp);

    if (!save->start) {
        S_ExitSystem("null save->start");
        return 0;
    }
    FileWrite(&save->start[0], sizeof(START_INFO), GF.level_count, fp);
    FileWrite(&save->timer, sizeof(uint32_t), 1, fp);
    FileWrite(&save->kills, sizeof(uint32_t), 1, fp);
    FileWrite(&save->secrets, sizeof(uint16_t), 1, fp);
    FileWrite(&save->current_level, sizeof(uint16_t), 1, fp);
    FileWrite(&save->pickups, sizeof(uint8_t), 1, fp);
    FileWrite(&save->bonus_flag, sizeof(uint8_t), 1, fp);
    FileWrite(&save->num_pickup1, sizeof(uint8_t), 1, fp);
    FileWrite(&save->num_pickup2, sizeof(uint8_t), 1, fp);
    FileWrite(&save->num_puzzle1, sizeof(uint8_t), 1, fp);
    FileWrite(&save->num_puzzle2, sizeof(uint8_t), 1, fp);
    FileWrite(&save->num_puzzle3, sizeof(uint8_t), 1, fp);
    FileWrite(&save->num_puzzle4, sizeof(uint8_t), 1, fp);
    FileWrite(&save->num_key1, sizeof(uint8_t), 1, fp);
    FileWrite(&save->num_key2, sizeof(uint8_t), 1, fp);
    FileWrite(&save->num_key3, sizeof(uint8_t), 1, fp);
    FileWrite(&save->num_key4, sizeof(uint8_t), 1, fp);
    FileWrite(&save->num_leadbar, sizeof(uint8_t), 1, fp);
    FileWrite(&save->challenge_failed, sizeof(uint8_t), 1, fp);
    FileWrite(&save->buffer[0], sizeof(char), MAX_SAVEGAME_BUFFER, fp);
    FileClose(fp);

    REQUEST_INFO *req = &LoadSaveGameRequester;
    req->item_flags[slot] &= ~RIF_BLOCKED;
    sprintf(
        &req->item_texts[req->item_text_len * slot], "%s %d", filename,
        SaveCounter);
    SavedGamesCount++;
    SaveCounter++;
    return 1;
}
