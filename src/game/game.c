#include "game/game.h"
#include "game/savegame.h"
#include "game/text.h"
#include "game/vars.h"
#include "specific/display.h"
#include "specific/frontend.h"
#include "specific/input.h"
#include "specific/output.h"
#include "config.h"
#include "util.h"

int LevelIsValid(int16_t level_number)
{
    TRACE("%d", level_number);
    int number_valid = 0;
    for (;;) {
        if (ValidLevels[number_valid] == -1) {
            break;
        }
        number_valid++;
    }
    for (int i = 0; i < number_valid; i++) {
        if (ValidLevels[i] == level_number) {
            return 1;
        }
    }
    return 0;
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

void LevelStats(int level_id)
{
    static char string[100];
    TEXTSTRING* txt;

    TempVideoAdjust(HiRes, 1.0);
    T_InitPrint();

    // heading
    sprintf(string, "%s", LevelTitles[level_id]); // TODO: translation
    txt = T_Print(0, -50, 0, string);
    T_CentreH(txt, 1);
    T_CentreV(txt, 1);

    // time taken
    int seconds = SaveGame[0].timer / 30;
    int hours = seconds / 3600;
    int minutes = (seconds / 60) % 60;
    seconds %= 60;
    if (hours) {
        sprintf(
            string, "%s %d:%d%d:%d%d",
            "TIME TAKEN", // TODO: translation
            hours, minutes / 10, minutes % 10, seconds / 10, seconds % 10);
    } else {
        sprintf(
            string, "%s %d:%d%d",
            "TIME TAKEN", // TODO: translation
            minutes, seconds / 10, seconds % 10);
    }
    txt = T_Print(0, 70, 0, string);
    T_CentreH(txt, 1);
    T_CentreV(txt, 1);

    // secrets
    int secrets_taken = 0;
    int secrets_total = MAX_SECRETS;
    do {
        if (SaveGame[0].secrets & 1) {
            ++secrets_taken;
        }
        SaveGame[0].secrets >>= 1;
        --secrets_total;
    } while (secrets_total);
    sprintf(
        string, "%s %d %s %d",
        "SECRETS", // TODO: translation
        secrets_taken,
        "OF", // TODO: translation
        SecretTotals[level_id]);
    txt = T_Print(0, 40, 0, string);
    T_CentreH(txt, 1);
    T_CentreV(txt, 1);

    // pickups
    sprintf(
        string, "%s %d", "PICKUPS",
        SaveGame[0].pickups); // TODO: translation
    txt = T_Print(0, 10, 0, string);
    T_CentreH(txt, 1);
    T_CentreV(txt, 1);

    // kills
    sprintf(string, "%s %d", "KILLS", SaveGame[0].kills); // TODO: translation
    txt = T_Print(0, -20, 0, string);
    T_CentreH(txt, 1);
    T_CentreV(txt, 1);

    // wait till action key release
#ifdef T1M_FEAT_OG_FIXES
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
#else
    {
#endif
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

    // go to next level
    if (level_id == LV_LEVEL10C) {
        SaveGame[0].bonus_flag = 1;
        for (int level = LV_LEVEL1; level <= LV_LEVEL10C; level++) {
            ModifyStartInfo(level);
        }
        SaveGame[0].current_level = 1;
    } else {
        CreateStartInfo(level_id + 1);
        ModifyStartInfo(level_id + 1);
        SaveGame[0].current_level = level_id + 1;
    }

    SaveGame[0].start[LV_CURRENT].available = 0;
    S_FadeToBlack();
    TempVideoRemove();
}

void T1MInjectSpecificGame()
{
    INJECT(0x0041D5A0, LevelStats);
    INJECT(0x0041D950, LevelIsValid);
    INJECT(0x0041D8F0, GetRandomControl);
    INJECT(0x0041D910, SeedRandomControl);
    INJECT(0x0041D920, GetRandomDraw);
    INJECT(0x0041D940, SeedRandomDraw);
}
