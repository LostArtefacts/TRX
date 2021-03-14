#include "game/game.h"

#include "game/camera.h"
#include "game/const.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/savegame.h"
#include "game/setup.h"
#include "game/text.h"
#include "game/vars.h"
#include "specific/display.h"
#include "specific/frontend.h"
#include "specific/input.h"
#include "specific/output.h"
#include "specific/shed.h"
#include "specific/sndpc.h"

#include "config.h"
#include "util.h"

#include <stdio.h>

int32_t StartGame(int32_t level_num, GAMEFLOW_LEVEL_TYPE level_type)
{
    CurrentLevel = level_num;
    TitleLoaded = 0;
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
    if (!InventoryChosen) {
        return GF_EXIT_TO_TITLE;
    }

    if (InventoryExtraData[0] == 0) {
        return GF_START_SAVED_GAME | InventoryExtraData[1];
    } else if (InventoryExtraData[0] == 1) {
        return GF_START_GAME | GF.first_level_num;
    } else {
        return GF_EXIT_TO_TITLE;
    }
}

int32_t GameLoop(int32_t demo_mode)
{
    TRACE("");
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

    S_SoundStopAllSamples();
    S_CDStop();
    if (OptionMusicVolume) {
        S_CDVolume(OptionMusicVolume * 25 + 5);
    }

    return ret;
}

int32_t LevelCompleteSequence(int32_t level_num)
{
    return GF_EXIT_TO_TITLE;
}

void SeedRandomControl(int32_t seed)
{
    TRACE("%d", seed);
    Rand1 = seed;
}

int32_t GetRandomControl()
{
    Rand1 = 0x41C64E6D * Rand1 + 0x3039;
    return (Rand1 >> 10) & 0x7FFF;
}

void SeedRandomDraw(int32_t seed)
{
    Rand2 = seed;
}

int32_t GetRandomDraw()
{
    Rand2 = 0x41C64E6D * Rand2 + 0x3039;
    return (Rand2 >> 10) & 0x7FFF;
}

void LevelStats(int32_t level_num)
{
    static char string[100];
    static char time_str[100];
    TEXTSTRING *txt;

    TempVideoAdjust(HiRes, 1.0);
    T_InitPrint();

    // heading
    sprintf(string, "%s", GF.levels[level_num].level_title);
    txt = T_Print(0, -50, 0, string);
    T_CentreH(txt, 1);
    T_CentreV(txt, 1);

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
    txt = T_Print(0, 70, 0, string);
    T_CentreH(txt, 1);
    T_CentreV(txt, 1);

    // secrets
    int32_t secrets_taken = 0;
    int32_t secrets_total = MAX_SECRETS;
    do {
        if (SaveGame.secrets & 1) {
            ++secrets_taken;
        }
        SaveGame.secrets >>= 1;
        --secrets_total;
    } while (secrets_total);
    sprintf(
        string, GF.strings[GS_STATS_SECRETS_FMT], secrets_taken,
        GF.levels[level_num].secrets);
    txt = T_Print(0, 40, 0, string);
    T_CentreH(txt, 1);
    T_CentreV(txt, 1);

    // pickups
    sprintf(string, GF.strings[GS_STATS_PICKUPS_FMT], SaveGame.pickups);
    txt = T_Print(0, 10, 0, string);
    T_CentreH(txt, 1);
    T_CentreV(txt, 1);

    // kills
    sprintf(string, GF.strings[GS_STATS_KILLS_FMT], SaveGame.kills);
    txt = T_Print(0, -20, 0, string);
    T_CentreH(txt, 1);
    T_CentreV(txt, 1);

    // wait till action key release
    if (T1MConfig.fix_end_of_level_freeze) {
        while (Input & IN_SELECT) {
            S_UpdateInput();
            S_InitialisePolyList();
            S_CopyBufferToScreen();
            S_UpdateInput();
            T_DrawText();
            S_OutputPolyList();
            S_DumpScreen();
        }
    } else {
        while (Input & IN_SELECT) {
            S_UpdateInput();
        }
        S_InitialisePolyList();
        S_CopyBufferToScreen();
        S_UpdateInput();
        T_DrawText();
        S_OutputPolyList();
        S_DumpScreen();
    }

    // wait till action key press
    while (!(Input & IN_SELECT)) {
        if (ResetFlag) {
            break;
        }
        S_InitialisePolyList();
        S_CopyBufferToScreen();
        S_UpdateInput();
        T_DrawText();
        S_OutputPolyList();
        S_DumpScreen();
    }

    if (level_num == GF.last_level_num) {
        SaveGame.bonus_flag = 1;
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
    TRACE("%s", filename);
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        return 0;
    }
    fread(filename, sizeof(char), 75, fp);
    int32_t counter;

    fread(&counter, sizeof(int32_t), 1, fp);

    if (!save->start) {
        S_ExitSystem("null save->start");
        return 0;
    }
    TRACE("%d", GF.level_count);
    fread(&save->start[0], sizeof(START_INFO), GF.level_count, fp);
    fread(&save->timer, sizeof(uint32_t), 1, fp);
    fread(&save->kills, sizeof(uint32_t), 1, fp);
    fread(&save->secrets, sizeof(uint16_t), 1, fp);
    fread(&save->current_level, sizeof(uint16_t), 1, fp);
    fread(&save->pickups, sizeof(uint8_t), 1, fp);
    fread(&save->bonus_flag, sizeof(uint8_t), 1, fp);
    fread(&save->num_pickup1, sizeof(uint8_t), 1, fp);
    fread(&save->num_pickup2, sizeof(uint8_t), 1, fp);
    fread(&save->num_puzzle1, sizeof(uint8_t), 1, fp);
    fread(&save->num_puzzle2, sizeof(uint8_t), 1, fp);
    fread(&save->num_puzzle3, sizeof(uint8_t), 1, fp);
    fread(&save->num_puzzle4, sizeof(uint8_t), 1, fp);
    fread(&save->num_key1, sizeof(uint8_t), 1, fp);
    fread(&save->num_key2, sizeof(uint8_t), 1, fp);
    fread(&save->num_key3, sizeof(uint8_t), 1, fp);
    fread(&save->num_key4, sizeof(uint8_t), 1, fp);
    fread(&save->num_leadbar, sizeof(uint8_t), 1, fp);
    fread(&save->challenge_failed, sizeof(uint8_t), 1, fp);
    fread(&save->buffer[0], sizeof(char), MAX_SAVEGAME_BUFFER, fp);
    fclose(fp);

    for (int i = 0; i < GF.level_count; i++) {
        if (GF.levels[i].level_type == GFL_CURRENT) {
            save->start[save->current_level] = save->start[i];
        }
    }

    return 1;
}

void GetSavedGamesList(REQUEST_INFO *req)
{
    switch (HiRes) {
    case 0:
        req->y = -32;
        req->vis_lines = 5;
        break;

    case 1:
        req->y = -62;
        req->vis_lines = 8;
        break;

    case 2:
        req->y = -90;
        req->vis_lines = 10;
        break;

    case 3:
        req->y = -100;
        req->vis_lines = 12;
        break;
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

        FILE *fp = fopen(filename, "rb");
        if (fp) {
            fread(filename, sizeof(char), 75, fp);
            int32_t counter;
            fread(&counter, sizeof(int32_t), 1, fp);
            fclose(fp);

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
    TRACE("%s", filename);

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        return 0;
    }

    for (int i = 0; i < GF.level_count; i++) {
        if (GF.levels[i].level_type == GFL_CURRENT) {
            save->start[i] = save->start[save->current_level];
        }
    }

    sprintf(filename, "%s", GF.levels[SaveGame.current_level].level_title);
    fwrite(filename, sizeof(char), 75, fp);
    fwrite(&SaveCounter, sizeof(int32_t), 1, fp);

    if (!save->start) {
        S_ExitSystem("null save->start");
        return 0;
    }
    fwrite(&save->start[0], sizeof(START_INFO), GF.level_count, fp);
    fwrite(&save->timer, sizeof(uint32_t), 1, fp);
    fwrite(&save->kills, sizeof(uint32_t), 1, fp);
    fwrite(&save->secrets, sizeof(uint16_t), 1, fp);
    fwrite(&save->current_level, sizeof(uint16_t), 1, fp);
    fwrite(&save->pickups, sizeof(uint8_t), 1, fp);
    fwrite(&save->bonus_flag, sizeof(uint8_t), 1, fp);
    fwrite(&save->num_pickup1, sizeof(uint8_t), 1, fp);
    fwrite(&save->num_pickup2, sizeof(uint8_t), 1, fp);
    fwrite(&save->num_puzzle1, sizeof(uint8_t), 1, fp);
    fwrite(&save->num_puzzle2, sizeof(uint8_t), 1, fp);
    fwrite(&save->num_puzzle3, sizeof(uint8_t), 1, fp);
    fwrite(&save->num_puzzle4, sizeof(uint8_t), 1, fp);
    fwrite(&save->num_key1, sizeof(uint8_t), 1, fp);
    fwrite(&save->num_key2, sizeof(uint8_t), 1, fp);
    fwrite(&save->num_key3, sizeof(uint8_t), 1, fp);
    fwrite(&save->num_key4, sizeof(uint8_t), 1, fp);
    fwrite(&save->num_leadbar, sizeof(uint8_t), 1, fp);
    fwrite(&save->challenge_failed, sizeof(uint8_t), 1, fp);
    fwrite(&save->buffer[0], sizeof(char), MAX_SAVEGAME_BUFFER, fp);
    fclose(fp);

    REQUEST_INFO *req = &LoadSaveGameRequester;
    req->item_flags[slot] &= ~RIF_BLOCKED;
    sprintf(
        &req->item_texts[req->item_text_len * slot], "%s %d", filename,
        SaveCounter);
    SavedGamesCount++;
    SaveCounter++;
    return 1;
}

void T1MInjectGameGame()
{
    INJECT(0x0041D0C0, StartGame);
    INJECT(0x0041D2C0, GameLoop);
    INJECT(0x0041D330, LevelCompleteSequence);
    INJECT(0x0041D5A0, LevelStats);
    INJECT(0x0041D8F0, GetRandomControl);
    INJECT(0x0041D910, SeedRandomControl);
    INJECT(0x0041D920, GetRandomDraw);
    INJECT(0x0041D940, SeedRandomDraw);
    INJECT(0x0041D9B0, GetSavedGamesList);
    INJECT(0x0041DA20, S_FrontEndCheck);
    INJECT(0x0041DB70, S_SaveGame);
    INJECT(0x0041DC70, S_LoadGame);
}
