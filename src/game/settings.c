#include "game/settings.h"

#include "game/option.h"
#include "game/types.h"
#include "game/vars.h"
#include "specific/input.h"
#include "specific/shed.h"
#include "specific/sndpc.h"

#include "filesystem.h"
#include "json.h"
#include "util.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int32_t S_ReadUserSettingsATI();
static int32_t S_ReadUserSettingsT1M();
static int32_t S_WriteUserSettingsT1M();

static int32_t S_ReadUserSettingsATI()
{
    MYFILE *fp = FileOpen(ATIUserSettingsPath, FILE_OPEN_READ);
    if (!fp) {
        return 0;
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
    return 1;
}

static int32_t S_ReadUserSettingsT1M()
{
    int32_t result = 0;
    size_t config_data_size;
    char *config_data = NULL;
    MYFILE *fp = NULL;
    struct json_value_s *root = NULL;
    struct json_parse_result_s parse_result;

    fp = FileOpen(T1MUserSettingsPath, FILE_OPEN_READ);
    if (!fp) {
        goto cleanup;
    }

    config_data_size = FileSize(fp);
    config_data = malloc(config_data_size + 1);
    if (!config_data) {
        goto cleanup;
    }
    FileRead(config_data, 1, config_data_size, fp);
    config_data[config_data_size] = '\0';
    FileClose(fp);
    fp = NULL;

    root = json_parse_ex(
        config_data, strlen(config_data), json_parse_flags_allow_json5, NULL,
        NULL, &parse_result);
    if (!root) {
        TRACE(
            "failed to parse script file: %s in line %d, char %d",
            json_get_error_description(parse_result.error),
            parse_result.error_line_no, parse_result.error_row_no, config_data);
        goto cleanup;
    }

    result = 1;

    struct json_object_s *root_obj = json_value_as_object(root);
    if (json_object_get_bool(root_obj, "bilinear", 1)) {
        AppSettings |= ASF_BILINEAR;
    } else {
        AppSettings &= ~ASF_BILINEAR;
    }

    if (json_object_get_bool(root_obj, "perspective", 1)) {
        AppSettings |= ASF_PERSPECTIVE;
    } else {
        AppSettings &= ~ASF_PERSPECTIVE;
    }

    GameHiRes = json_object_get_number_int(root_obj, "hi_res", 3);
    CLAMP(GameHiRes, 0, 3);

    GameSizer = json_object_get_number_double(root_obj, "game_sizer", 1.0);

    OptionMusicVolume = json_object_get_number_int(root_obj, "music_volume", 8);
    CLAMP(OptionMusicVolume, 0, 10);

    OptionSoundFXVolume =
        json_object_get_number_int(root_obj, "sound_volume", 8);
    CLAMP(OptionSoundFXVolume, 0, 10);

    IConfig = json_object_get_number_int(root_obj, "layout_num", 0);
    CLAMP(IConfig, 0, 1);

    struct json_array_s *layout_arr = json_object_get_array(root_obj, "layout");
    for (int i = 0; i < 13; i++) {
        Layout[1][i] = json_array_get_number_int(layout_arr, i, Layout[1][i]);
    }

cleanup:
    if (fp) {
        FileClose(fp);
    }
    if (config_data) {
        free(config_data);
    }
    if (root) {
        json_value_free(root);
    }
    return result;
}

static int32_t S_WriteUserSettingsT1M()
{
    MYFILE *fp = FileOpen(T1MUserSettingsPath, FILE_OPEN_WRITE);
    if (!fp) {
        return 0;
    }

    size_t size;
    struct json_object_s *root_obj = json_object_new();
    json_object_append_bool(root_obj, "bilinear", AppSettings & ASF_BILINEAR);
    json_object_append_bool(
        root_obj, "perspective", AppSettings & ASF_PERSPECTIVE);
    json_object_append_number_int(root_obj, "hi_res", GameHiRes);
    json_object_append_number_double(root_obj, "game_sizer", GameSizer);
    json_object_append_number_int(root_obj, "music_volume", OptionMusicVolume);
    json_object_append_number_int(
        root_obj, "sound_volume", OptionSoundFXVolume);
    json_object_append_number_int(root_obj, "layout_num", IConfig);

    struct json_array_s *layout_arr = json_array_new();
    for (int i = 0; i < 13; i++) {
        json_array_append_number_int(layout_arr, Layout[1][i]);
    }
    json_object_append_array(root_obj, "layout", layout_arr);

    struct json_value_s *root = json_value_from_object(root_obj);
    char *data = json_write_pretty(root, "  ", "\n", &size);
    json_value_free(root);

    FileWrite(data, sizeof(char), size - 1, fp);
    FileClose(fp);
    free(data);

    return 1;
}

void S_ReadUserSettings()
{
    if (S_ReadUserSettingsATI()) {
        S_WriteUserSettingsT1M();
        FileDelete(ATIUserSettingsPath);
    }
    S_ReadUserSettingsT1M();
}

void S_WriteUserSettings()
{
    S_WriteUserSettingsT1M();
}
