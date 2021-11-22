#include "game/settings.h"

#include "config.h"
#include "filesystem.h"
#include "game/music.h"
#include "game/option.h"
#include "game/screen.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "json.h"
#include "log.h"
#include "memory.h"
#include "specific/s_input.h"

#include <stdint.h>
#include <string.h>

static const char *ATIUserSettingsPath = "atiset.dat";
static const char *T1MUserSettingsPath = "cfg/Tomb1Main_runtime.json5";

static int32_t S_ReadUserSettingsATI();
static int32_t S_ReadUserSettingsT1M();
static int32_t S_ReadUserSettingsT1MFromJson(const char *cfg_data);
static int32_t S_WriteUserSettingsT1M();

static int32_t S_ReadUserSettingsATI()
{
    MYFILE *fp = File_Open(ATIUserSettingsPath, FILE_OPEN_READ);
    if (!fp) {
        return 0;
    }

    LOG_INFO("Loading user settings (T1M)");

    File_Read(&T1MConfig.music_volume, sizeof(int16_t), 1, fp);
    File_Read(&T1MConfig.sound_volume, sizeof(int16_t), 1, fp);

    {
        int16_t layout[13];
        File_Read(layout, sizeof(int16_t), 13, fp);

        S_Input_AssignKeyCode(INPUT_LAYOUT_USER, INPUT_KEY_UP, layout[0]);
        S_Input_AssignKeyCode(INPUT_LAYOUT_USER, INPUT_KEY_DOWN, layout[1]);
        S_Input_AssignKeyCode(INPUT_LAYOUT_USER, INPUT_KEY_LEFT, layout[2]);
        S_Input_AssignKeyCode(INPUT_LAYOUT_USER, INPUT_KEY_RIGHT, layout[3]);
        S_Input_AssignKeyCode(INPUT_LAYOUT_USER, INPUT_KEY_STEP_L, layout[4]);
        S_Input_AssignKeyCode(INPUT_LAYOUT_USER, INPUT_KEY_STEP_R, layout[5]);
        S_Input_AssignKeyCode(INPUT_LAYOUT_USER, INPUT_KEY_SLOW, layout[6]);
        S_Input_AssignKeyCode(INPUT_LAYOUT_USER, INPUT_KEY_JUMP, layout[7]);
        S_Input_AssignKeyCode(INPUT_LAYOUT_USER, INPUT_KEY_ACTION, layout[8]);
        S_Input_AssignKeyCode(INPUT_LAYOUT_USER, INPUT_KEY_DRAW, layout[9]);
        S_Input_AssignKeyCode(INPUT_LAYOUT_USER, INPUT_KEY_LOOK, layout[10]);
        S_Input_AssignKeyCode(INPUT_LAYOUT_USER, INPUT_KEY_ROLL, layout[11]);
        S_Input_AssignKeyCode(INPUT_LAYOUT_USER, INPUT_KEY_OPTION, layout[12]);
    }

    {
        uint32_t render_flags;
        File_Read(&render_flags, sizeof(int32_t), 1, fp);
        T1MConfig.render_flags.perspective = (bool)(render_flags & 1);
        T1MConfig.render_flags.bilinear = (bool)(render_flags & 2);
        T1MConfig.render_flags.fps_counter = (bool)(render_flags & 4);
    }

    {
        int32_t resolution_idx;
        File_Read(&resolution_idx, sizeof(int32_t), 1, fp);
        CLAMP(resolution_idx, 0, RESOLUTIONS_SIZE - 1);
        Screen_SetGameResIdx(resolution_idx);
    }

    // Skip GameSizer from TombATI, which is no longer used in T1M.
    // In the original game, it's expected to be 1.0 everywhere and changing it
    // to any other value results in uninteresting window clipping anomalies.
    File_Seek(fp, sizeof(double), FILE_SEEK_CUR);

    File_Read(&T1MConfig.input.layout, sizeof(int32_t), 1, fp);

    T1MConfig.brightness = DEFAULT_BRIGHTNESS;
    T1MConfig.ui.text_scale = DEFAULT_UI_SCALE;
    T1MConfig.ui.bar_scale = DEFAULT_UI_SCALE;

    File_Close(fp);
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
    T1MConfig.render_flags.bilinear =
        json_object_get_bool(root_obj, "bilinear", true);
    T1MConfig.render_flags.perspective =
        json_object_get_bool(root_obj, "perspective", true);

    {
        int32_t resolution_idx = json_object_get_number_int(
            root_obj, "hi_res", RESOLUTIONS_SIZE - 1);
        CLAMP(resolution_idx, 0, RESOLUTIONS_SIZE - 1);
        Screen_SetGameResIdx(resolution_idx);
    }

    T1MConfig.music_volume =
        json_object_get_number_int(root_obj, "music_volume", 8);
    CLAMP(T1MConfig.music_volume, 0, 10);

    T1MConfig.sound_volume =
        json_object_get_number_int(root_obj, "sound_volume", 8);
    CLAMP(T1MConfig.sound_volume, 0, 10);

    T1MConfig.input.layout =
        json_object_get_number_int(root_obj, "layout_num", 0);
    CLAMP(T1MConfig.input.layout, 0, 1);

    T1MConfig.brightness = json_object_get_number_double(
        root_obj, "brightness", DEFAULT_BRIGHTNESS);
    CLAMP(T1MConfig.brightness, MIN_BRIGHTNESS, MAX_BRIGHTNESS);

    T1MConfig.ui.text_scale = json_object_get_number_double(
        root_obj, "ui_text_scale", DEFAULT_UI_SCALE);
    CLAMP(T1MConfig.ui.text_scale, MIN_UI_SCALE, MAX_UI_SCALE);

    T1MConfig.ui.bar_scale = json_object_get_number_double(
        root_obj, "ui_bar_scale", DEFAULT_UI_SCALE);
    CLAMP(T1MConfig.ui.bar_scale, MIN_UI_SCALE, MAX_UI_SCALE);

    struct json_array_s *layout_arr = json_object_get_array(root_obj, "layout");
    for (int i = 0; i < INPUT_KEY_NUMBER_OF; i++) {
        S_INPUT_KEYCODE key_code =
            S_Input_GetAssignedKeyCode(INPUT_LAYOUT_USER, i);
        key_code = json_array_get_number_int(layout_arr, i, key_code);
        S_Input_AssignKeyCode(INPUT_LAYOUT_USER, i, key_code);
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

    fp = File_Open(T1MUserSettingsPath, FILE_OPEN_READ);
    if (!fp) {
        LOG_ERROR("Failed to open file '%s'", T1MUserSettingsPath);
        result = S_ReadUserSettingsT1MFromJson("");
        goto cleanup;
    }

    cfg_data_size = File_Size(fp);
    cfg_data = Memory_Alloc(cfg_data_size + 1);
    File_Read(cfg_data, 1, cfg_data_size, fp);
    cfg_data[cfg_data_size] = '\0';
    File_Close(fp);
    fp = NULL;

    result = S_ReadUserSettingsT1MFromJson(cfg_data);

cleanup:
    if (fp) {
        File_Close(fp);
    }
    if (cfg_data) {
        Memory_Free(cfg_data);
    }
    return result;
}

static int32_t S_WriteUserSettingsT1M()
{
    LOG_INFO("Saving user settings (T1M)");

    MYFILE *fp = File_Open(T1MUserSettingsPath, FILE_OPEN_WRITE);
    if (!fp) {
        return 0;
    }

    size_t size;
    struct json_object_s *root_obj = json_object_new();
    json_object_append_bool(
        root_obj, "bilinear", T1MConfig.render_flags.bilinear);
    json_object_append_bool(
        root_obj, "perspective", T1MConfig.render_flags.perspective);
    json_object_append_number_int(root_obj, "hi_res", Screen_GetGameResIdx());
    json_object_append_number_int(
        root_obj, "music_volume", T1MConfig.music_volume);
    json_object_append_number_int(
        root_obj, "sound_volume", T1MConfig.sound_volume);
    json_object_append_number_int(
        root_obj, "layout_num", T1MConfig.input.layout);
    json_object_append_number_double(
        root_obj, "ui_text_scale", T1MConfig.ui.text_scale);
    json_object_append_number_double(
        root_obj, "ui_bar_scale", T1MConfig.ui.bar_scale);
    json_object_append_number_double(
        root_obj, "brightness", T1MConfig.brightness);

    struct json_array_s *layout_arr = json_array_new();
    for (int i = 0; i < INPUT_KEY_NUMBER_OF; i++) {
        json_array_append_number_int(
            layout_arr, S_Input_GetAssignedKeyCode(INPUT_LAYOUT_USER, i));
    }
    json_object_append_array(root_obj, "layout", layout_arr);

    struct json_value_s *root = json_value_from_object(root_obj);
    char *data = json_write_pretty(root, "  ", "\n", &size);
    json_value_free(root);

    File_Write(data, sizeof(char), size - 1, fp);
    File_Close(fp);
    Memory_Free(data);

    return 1;
}

void S_ReadUserSettings()
{
    if (S_ReadUserSettingsATI()) {
        if (!File_Delete(ATIUserSettingsPath)) {
            // only save settings if we successfully removed the file
            S_WriteUserSettingsT1M();
        }
    }
    S_ReadUserSettingsT1M();

    DefaultConflict();

    Music_SetVolume(T1MConfig.music_volume);
    Sound_SetMasterVolume(T1MConfig.sound_volume);
}

void S_WriteUserSettings()
{
    S_WriteUserSettingsT1M();
}
