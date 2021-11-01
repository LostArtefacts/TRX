#include "game/settings.h"

#include "filesystem.h"
#include "game/mnsound.h"
#include "game/option.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "json.h"
#include "log.h"
#include "specific/display.h"
#include "specific/input.h"
#include "specific/sndpc.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int32_t S_ReadUserSettingsATI();
static int32_t S_ReadUserSettingsT1M();
static int32_t S_ReadUserSettingsT1MFromJson(const char *cfg_data);
static int32_t S_WriteUserSettingsT1M();

static int32_t S_ReadUserSettingsATI()
{
    MYFILE *fp = FileOpen(ATIUserSettingsPath, FILE_OPEN_READ);
    if (!fp) {
        return 0;
    }

    LOG_INFO("Loading user settings (T1M)");

    FileRead(&OptionMusicVolume, sizeof(int16_t), 1, fp);
    FileRead(&OptionSoundFXVolume, sizeof(int16_t), 1, fp);
    FileRead(Layout[1], sizeof(int16_t), 13, fp);
    FileRead(&RenderSettings, sizeof(int32_t), 1, fp);

    {
        int32_t resolution_idx;
        FileRead(&resolution_idx, sizeof(int32_t), 1, fp);
        CLAMP(resolution_idx, 0, RESOLUTIONS_SIZE - 1);
        SetGameScreenSizeIdx(resolution_idx);
    }

    // Skip GameSizer from TombATI, which is no longer used in T1M.
    // In the original game, it's expected to be 1.0 everywhere and changing it
    // to any other value results in uninteresting window clipping anomalies.
    FileSeek(fp, sizeof(double), FILE_SEEK_CUR);

    FileRead(&IConfig, sizeof(int32_t), 1, fp);

    UITextScale = DEFAULT_UI_SCALE;
    UIBarScale = DEFAULT_UI_SCALE;

    FileClose(fp);
    return 1;
}

static int32_t S_ReadUserSettingsT1MFromJson(const char *cfg_data)
{
    int32_t result = 0;
    struct json_value_s *root = NULL;
    struct json_parse_result_s parse_result;

    LOG_INFO("Loading user settings (T1M)");

    root = json_parse_ex(
        cfg_data, strlen(cfg_data), json_parse_flags_allow_json5, NULL, NULL,
        &parse_result);
    if (!root) {
        LOG_ERROR(
            "failed to parse script file: %s in line %d, char %d",
            json_get_error_description(parse_result.error),
            parse_result.error_line_no, parse_result.error_row_no, cfg_data);
        // continue to supply the default values
    }

    result = 1;

    struct json_object_s *root_obj = json_value_as_object(root);
    if (json_object_get_bool(root_obj, "bilinear", 1)) {
        RenderSettings |= RSF_BILINEAR;
    } else {
        RenderSettings &= ~RSF_BILINEAR;
    }

    if (json_object_get_bool(root_obj, "perspective", 1)) {
        RenderSettings |= RSF_PERSPECTIVE;
    } else {
        RenderSettings &= ~RSF_PERSPECTIVE;
    }

    {
        int32_t resolution_idx = json_object_get_number_int(
            root_obj, "hi_res", RESOLUTIONS_SIZE - 1);
        CLAMP(resolution_idx, 0, RESOLUTIONS_SIZE - 1);
        SetGameScreenSizeIdx(resolution_idx);
    }

    OptionMusicVolume = json_object_get_number_int(root_obj, "music_volume", 8);
    CLAMP(OptionMusicVolume, 0, 10);

    OptionSoundFXVolume =
        json_object_get_number_int(root_obj, "sound_volume", 8);
    CLAMP(OptionSoundFXVolume, 0, 10);

    IConfig = json_object_get_number_int(root_obj, "layout_num", 0);
    CLAMP(IConfig, 0, 1);

    UITextScale = json_object_get_number_double(
        root_obj, "ui_text_scale", DEFAULT_UI_SCALE);
    CLAMP(UITextScale, MIN_UI_SCALE, MAX_UI_SCALE);

    UIBarScale = json_object_get_number_double(
        root_obj, "ui_bar_scale", DEFAULT_UI_SCALE);
    CLAMP(UIBarScale, MIN_UI_SCALE, MAX_UI_SCALE);

    struct json_array_s *layout_arr = json_object_get_array(root_obj, "layout");
    for (int i = 0; i < KEY_NUMBER_OF; i++) {
        Layout[1][i] = json_array_get_number_int(layout_arr, i, Layout[1][i]);
    }

    if (root) {
        json_value_free(root);
    }
    return result;
}

static int32_t S_ReadUserSettingsT1M()
{
    int32_t result = 0;
    size_t cfg_data_size;
    char *cfg_data = NULL;
    MYFILE *fp = NULL;

    fp = FileOpen(T1MUserSettingsPath, FILE_OPEN_READ);
    if (!fp) {
        LOG_ERROR("Failed to open file '%s'", T1MUserSettingsPath);
        result = S_ReadUserSettingsT1MFromJson("");
        goto cleanup;
    }

    cfg_data_size = FileSize(fp);
    cfg_data = malloc(cfg_data_size + 1);
    if (!cfg_data) {
        LOG_ERROR("Failed to allocate memory");
        result = S_ReadUserSettingsT1MFromJson("");
        goto cleanup;
    }
    FileRead(cfg_data, 1, cfg_data_size, fp);
    cfg_data[cfg_data_size] = '\0';
    FileClose(fp);
    fp = NULL;

    result = S_ReadUserSettingsT1MFromJson(cfg_data);

cleanup:
    if (fp) {
        FileClose(fp);
    }
    if (cfg_data) {
        free(cfg_data);
    }
    return result;
}

static int32_t S_WriteUserSettingsT1M()
{
    LOG_INFO("Saving user settings (T1M)");

    MYFILE *fp = FileOpen(T1MUserSettingsPath, FILE_OPEN_WRITE);
    if (!fp) {
        return 0;
    }

    size_t size;
    struct json_object_s *root_obj = json_object_new();
    json_object_append_bool(
        root_obj, "bilinear", RenderSettings & RSF_BILINEAR);
    json_object_append_bool(
        root_obj, "perspective", RenderSettings & RSF_PERSPECTIVE);
    json_object_append_number_int(root_obj, "hi_res", GetGameScreenSizeIdx());
    json_object_append_number_int(root_obj, "music_volume", OptionMusicVolume);
    json_object_append_number_int(
        root_obj, "sound_volume", OptionSoundFXVolume);
    json_object_append_number_int(root_obj, "layout_num", IConfig);
    json_object_append_number_double(root_obj, "ui_text_scale", UITextScale);
    json_object_append_number_double(root_obj, "ui_bar_scale", UIBarScale);

    struct json_array_s *layout_arr = json_array_new();
    for (int i = 0; i < KEY_NUMBER_OF; i++) {
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
        if (!FileDelete(ATIUserSettingsPath)) {
            // only save settings if we successfully removed the file
            S_WriteUserSettingsT1M();
        }
    }
    S_ReadUserSettingsT1M();

    DefaultConflict();

    if (OptionMusicVolume) {
        S_MusicVolume(25 * OptionMusicVolume + 5);
    } else {
        S_MusicVolume(0);
    }

    if (OptionSoundFXVolume) {
        mn_adjust_master_volume(6 * OptionSoundFXVolume + 3);
    } else {
        mn_adjust_master_volume(0);
    }
}

void S_WriteUserSettings()
{
    S_WriteUserSettingsT1M();
}
