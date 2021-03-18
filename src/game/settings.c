#include "game/settings.h"

#include "game/option.h"
#include "game/vars.h"
#include "specific/shed.h"
#include "specific/sndpc.h"

#include "filesystem.h"

void S_ReadUserSettings()
{
    MYFILE *fp = FileOpen(UserSettingsPath, FILE_OPEN_READ);
    if (!fp) {
        return;
    }

    FileRead(&OptionMusicVolume, sizeof(int16_t), 1, fp);
    FileRead(&OptionSoundFXVolume, sizeof(int16_t), 1, fp);
    FileRead(Layout[1], sizeof(int16_t), 13, fp);
    FileRead(&AppSettings, sizeof(int32_t), 1, fp);
    FileRead(&GameHiRes, sizeof(int32_t), 1, fp);
    FileRead(&GameSizer, sizeof(double), 1, fp);
    FileRead(&IConfig, sizeof(int32_t), 1, fp);

    DefaultConflict();

    if (OptionMusicVolume) {
        S_CDVolume(25 * OptionMusicVolume + 5);
    } else {
        S_CDVolume(0);
    }

    if (OptionSoundFXVolume) {
        adjust_master_volume(6 * OptionSoundFXVolume + 3);
    } else {
        adjust_master_volume(0);
    }

    FileClose(fp);
}

void S_WriteUserSettings()
{
    MYFILE *fp = FileOpen(UserSettingsPath, FILE_OPEN_WRITE);
    if (!fp) {
        return;
    }
    FileWrite(&OptionMusicVolume, sizeof(int16_t), 1, fp);
    FileWrite(&OptionSoundFXVolume, sizeof(int16_t), 1, fp);
    FileWrite(Layout[1], sizeof(int16_t), 13, fp);
    FileWrite(&AppSettings, sizeof(int32_t), 1, fp);
    FileWrite(&GameHiRes, sizeof(int32_t), 1, fp);
    FileWrite(&GameSizer, sizeof(double), 1, fp);
    FileWrite(&IConfig, sizeof(int32_t), 1, fp);
    FileClose(fp);
}
