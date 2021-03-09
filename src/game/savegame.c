#include "game/savegame.h"
#include "game/vars.h"
#include "util.h"

void InitialiseStartInfo()
{
    if (!SaveGame[0].bonus_flag) {
        for (int i = 0; i < LV_NUMBER_OF; i++) {
            ModifyStartInfo(i);
            SaveGame[0].start[i].available = 0;
        }
        SaveGame[0].start[LV_GYM].available = 1;
        SaveGame[0].start[LV_FIRSTLEVEL].available = 1;
    }
}

void T1MInjectGameSaveGame()
{
    INJECT(0x004344D0, InitialiseStartInfo);
}
