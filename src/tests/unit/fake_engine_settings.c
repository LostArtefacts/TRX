// The engine surface the settings dialogs reach for while they build a scene:
// the file layer their presets come from, the input state they poll, and the
// gameplay predicates that decide whether a row is shown at all.
//
// The file layer is a read-only shim over the shipped data in the repository,
// so the dialogs see the presets players see. Writes go nowhere: a test that
// measures a dialog has no business leaving a config file behind.

#include "fake_engine_settings.h"

#include <trx/config/file.h>
#include <trx/config/priv.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/filesystem.h>
#include <trx/core/shell.h>
#include <trx/game/clock/timer.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/console/common.h>
#include <trx/game/creature/common.h>
#include <trx/game/game/state.h>
#include <trx/game/game_strings/manager.h>
#include <trx/game/gun/common.h>
#include <trx/game/input/common.h>
#include <trx/game/lara/common.h>
#include <trx/game/lara/vehicle.h>
#include <trx/game/objects/common.h>
#include <trx/game/output/draw.h>
#include <trx/game/music/common.h>
#include <trx/game/rooms/common.h>
#include <trx/game/sound/common.h>
#include <trx/game/shell/paths.h>
#include <trx/game/stats/init.h>

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

INPUT_STATE g_Input = {};
INPUT_STATE g_InputDB = {};

// Every path variable resolves to the shipped config directory, which is where
// the presets live.
char *TRXPath_ExpandVars(const char *const path)
{
    const char *const opening = strchr(path, '%');
    const char *const closing =
        opening != nullptr ? strchr(opening + 1, '%') : nullptr;
    if (closing == nullptr) {
        return Memory_DupStr(path);
    }
    return String_Format("%s%s", TEST_SHIP_CFG_DIR, closing + 1);
}

bool File_Load(
    const char *const path, char **const output_data, size_t *const output_size)
{
    FILE *const fp = fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    fseek(fp, 0, SEEK_END);
    const long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *const data = Memory_Alloc(size + 1);
    const size_t read_size = fread(data, 1, size, fp);
    fclose(fp);
    data[read_size] = '\0';
    *output_data = data;
    if (output_size != nullptr) {
        *output_size = read_size;
    }
    return true;
}

bool File_DirExists(const char *const path)
{
    struct stat info;
    return stat(path, &info) == 0 && S_ISDIR(info.st_mode);
}

void *File_OpenDirectory(const char *const path)
{
    return opendir(path);
}

const char *File_ReadDirectory(void *const dir)
{
    const struct dirent *const entry = readdir(dir);
    return entry != nullptr ? entry->d_name : nullptr;
}

void File_CloseDirectory(void *const dir)
{
    closedir(dir);
}

bool ConfigFile_Read(const CONFIG_IO_ARGS *const control)
{
    return false;
}

bool ConfigFile_Write(const CONFIG_IO_ARGS *const control)
{
    return true;
}

void Config_LoadFromJSON(JSON_OBJECT *const root_obj)
{
}

void Config_DumpToJSON(JSON_OBJECT *const root_obj)
{
}

void Config_Sanitize(void)
{
}

void Shell_ExitSystem(const char *const message)
{
    printf("unexpected engine exit: %s\n", message);
    abort();
}

void Shell_ExitSystemEx(
    const char *const log_message, const char *const dialog_message)
{
    Shell_ExitSystem(log_message);
}

int32_t GameStringManager_SubscribeReload(
    const EVENT_LISTENER listener, void *const user_data)
{
    return -1;
}

void GameStringManager_UnsubscribeReload(const int32_t listener_id)
{
}

void Console_LogEx(
    const LOG_LEVEL level, const char *const file, const int line,
    const char *const func, const char *const fmt, ...)
{
}

bool ClockTimer_CheckElapsedAndTake(CLOCK_TIMER *const timer, const double sec)
{
    return false;
}

bool Input_IsPressedEx(
    const INPUT_BACKEND backend, const INPUT_LAYOUT layout,
    const INPUT_ROLE role)
{
    return false;
}

void Input_EnterListenMode(void)
{
}

void Input_ExitListenMode(void)
{
}

bool Creature_IsAlly(const ITEM *const item)
{
    return false;
}

bool Game_IsBonusFlagSet(const GAME_BONUS_FLAG flag)
{
    return false;
}

int32_t Gun_GetAmmoClipCount(const LARA_GUN_TYPE gun_type)
{
    return 1;
}

ITEM *Lara_GetItem(void)
{
    return nullptr;
}

LARA_INFO *Lara_GetLaraInfo(void)
{
    return nullptr;
}

ITEM *Lara_Vehicle_GetItem(void)
{
    return nullptr;
}

bool Lara_Vehicle_IsOnType(const OBJECT_ID obj_id)
{
    return false;
}

bool Object_IsType(const OBJECT_ID obj_id, const OBJECT_ID *const test_arr)
{
    return false;
}

ROOM *Room_Get(const int32_t room_num)
{
    return nullptr;
}

// Every option that a game could offer is worth measuring, so the predicates
// that gate rows answer as permissively as they can.
bool Stats_GameHasCrystals(void)
{
    return true;
}

bool Touch_HasHardwareSupport(void)
{
    return true;
}

void Output_DrawScreenGradientQuad(
    const int32_t sx, const int32_t sy, const int32_t z, const int32_t w,
    const int32_t h, const RGBA_8888 tl, const RGBA_8888 tr, const RGBA_8888 bl,
    const RGBA_8888 br)
{
}

void Shell_ExitSystemFmt(const char *const fmt, ...)
{
    Shell_ExitSystem(fmt);
}

MYFILE *File_Open(const char *const path, const FILE_OPEN_MODE mode)
{
    return nullptr;
}

void File_WriteData(
    MYFILE *const file, const void *const data, const size_t size)
{
}

void File_Close(MYFILE *const file)
{
}

VECTOR *GameStringManager_GetAvailableLanguages(void)
{
    return nullptr;
}

const char *GameStringManager_GetLanguageName(const char *const code)
{
    return code;
}

bool GameStringManager_ReloadLanguage(const char *const lang)
{
    return true;
}

bool Catalog_NameToEnum(
    const CATALOG_CONTEXT context, const char *const name,
    CATALOG_ID *const out_id)
{
    return false;
}

void Music_SetVolume(const float volume)
{
}

void Sound_SetMasterVolume(const float volume)
{
}

int32_t Sound_Effect(
    const SAMPLE_TRX_ID sfx_num, const XYZ_32 *const pos, const uint32_t flags)
{
    return 0;
}
