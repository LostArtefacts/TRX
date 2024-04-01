#include "config.h"

#include "config_map.h"
#include "filesystem.h"
#include "game/input.h"
#include "game/music.h"
#include "game/sound.h"
#include "gfx/context.h"
#include "global/const.h"
#include "global/types.h"
#include "json/json_base.h"
#include "json/json_parse.h"
#include "json/json_write.h"
#include "log.h"
#include "memory.h"
#include "util.h"

#include <stdio.h>
#include <string.h>

#define Q(x) #x
#define QUOTE(x) Q(x)

CONFIG g_Config = { 0 };

static const char *m_TR1XGlobalSettingsPath = "cfg/TR1X.json5";

static int Config_ReadEnum(
    struct json_object_s *obj, const char *name, int default_value,
    const CONFIG_OPTION_ENUM_MAP *enum_map);
static void Config_WriteEnum(
    struct json_object_s *obj, const char *name, int value,
    const CONFIG_OPTION_ENUM_MAP *enum_map);

static int Config_ReadEnum(
    struct json_object_s *obj, const char *name, int default_value,
    const CONFIG_OPTION_ENUM_MAP *enum_map)
{
    const char *value_str = json_object_get_string(obj, name, NULL);
    if (value_str) {
        while (enum_map->text) {
            if (!strcmp(value_str, enum_map->text)) {
                return enum_map->value;
            }
            enum_map++;
        }
    }
    return default_value;
}

static void Config_WriteEnum(
    struct json_object_s *obj, const char *name, int value,
    const CONFIG_OPTION_ENUM_MAP *enum_map)
{
    while (enum_map->text) {
        if (enum_map->value == value) {
            json_object_append_string(obj, name, enum_map->text);
            break;
        }
        enum_map++;
    }
}

bool Config_ReadFromJSON(const char *cfg_data)
{
    bool result = false;
    struct json_value_s *root;
    struct json_parse_result_s parse_result;

    root = json_parse_ex(
        cfg_data, strlen(cfg_data), json_parse_flags_allow_json5, NULL, NULL,
        &parse_result);
    if (root) {
        result = true;
    } else {
        LOG_ERROR(
            "failed to parse config file: %s in line %d, char %d",
            json_get_error_description(parse_result.error),
            parse_result.error_line_no, parse_result.error_row_no);
        // continue to supply the default values
    }

    struct json_object_s *root_obj = json_value_as_object(root);

    const CONFIG_OPTION *opt = g_ConfigOptionMap;
    while (opt->target) {
        if (opt->type == COT_BOOL) {
            *(bool *)opt->target = json_object_get_bool(
                root_obj, opt->name, *(bool *)opt->default_value);
        } else if (opt->type == COT_INT32) {
            *(int32_t *)opt->target = json_object_get_int(
                root_obj, opt->name, *(int32_t *)opt->default_value);
        } else if (opt->type == COT_FLOAT) {
            *(float *)opt->target = json_object_get_double(
                root_obj, opt->name, *(float *)opt->default_value);
        } else if (opt->type == COT_DOUBLE) {
            *(double *)opt->target = json_object_get_double(
                root_obj, opt->name, *(double *)opt->default_value);
        } else if (opt->type == COT_ENUM) {
            *(int *)opt->target = Config_ReadEnum(
                root_obj, opt->name, *(int *)opt->default_value,
                (const CONFIG_OPTION_ENUM_MAP *)opt->param);
        }
        opt++;
    }

    CLAMP(g_Config.start_lara_hitpoints, 1, LARA_HITPOINTS);
    CLAMP(g_Config.fov_value, 30, 255);
    CLAMP(g_Config.camera_speed, 1, 10);
    CLAMP(g_Config.music_volume, 0, 10);
    CLAMP(g_Config.sound_volume, 0, 10);
    CLAMP(g_Config.input.layout, 0, INPUT_LAYOUT_NUMBER_OF - 1);
    CLAMP(g_Config.input.cntlr_layout, 0, INPUT_LAYOUT_NUMBER_OF - 1);
    CLAMP(g_Config.brightness, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
    CLAMP(g_Config.ui.text_scale, MIN_TEXT_SCALE, MAX_TEXT_SCALE);
    CLAMP(g_Config.ui.bar_scale, MIN_BAR_SCALE, MAX_BAR_SCALE);

    if (g_Config.rendering.fps != 30 && g_Config.rendering.fps != 60) {
        g_Config.rendering.fps = 30;
    }

    char layout_name[50];
    for (INPUT_LAYOUT layout = INPUT_LAYOUT_CUSTOM_1;
         layout < INPUT_LAYOUT_NUMBER_OF; layout++) {
        sprintf(layout_name, "layout_%d", layout);
        struct json_array_s *layout_arr =
            json_object_get_array(root_obj, layout_name);
        for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
            INPUT_SCANCODE scancode = Input_GetAssignedScancode(layout, role);
            scancode = json_array_get_int(layout_arr, role, scancode);
            Input_AssignScancode(layout, role, scancode);
        }
    }

    for (INPUT_LAYOUT layout = INPUT_LAYOUT_CUSTOM_1;
         layout < INPUT_LAYOUT_NUMBER_OF; layout++) {
        sprintf(layout_name, "cntlr_layout_%d", layout);
        struct json_array_s *cntlr_arr =
            json_object_get_array(root_obj, layout_name);
        INPUT_ROLE role = 0;
        int i = 0;
        for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
            int16_t type = Input_GetAssignedButtonType(layout, role);
            type = json_array_get_int(cntlr_arr, i, type);
            i++;

            int16_t bind = Input_GetAssignedBind(layout, role);
            bind = json_array_get_int(cntlr_arr, i, bind);
            i++;

            int16_t axis_dir = Input_GetAssignedAxisDir(layout, role);
            axis_dir = json_array_get_int(cntlr_arr, i, axis_dir);
            i++;

            if (type == BT_BUTTON) {
                Input_AssignButton(layout, role, bind);
            } else {
                Input_AssignAxis(layout, role, bind, axis_dir);
            }
        }
    }

    if (root) {
        json_value_free(root);
    }

    g_Config.loaded = true;
    return result;
}

bool Config_Read(void)
{
    bool result = false;
    char *cfg_data = NULL;

    if (!File_Load(m_TR1XGlobalSettingsPath, &cfg_data, NULL)) {
        LOG_WARNING(
            "'%s' not loaded - default settings will apply",
            m_TR1XGlobalSettingsPath);
        result = Config_ReadFromJSON("{}");
    } else {
        result = Config_ReadFromJSON(cfg_data);
    }

    Memory_FreePointer(&cfg_data);
    return result;
}

void Config_Init(void)
{
    Input_CheckConflicts(CM_KEYBOARD, g_Config.input.layout);
    Input_CheckConflicts(CM_CONTROLLER, g_Config.input.cntlr_layout);
    GFX_Context_SetVSync(g_Config.rendering.enable_vsync);
    Music_SetVolume(g_Config.music_volume);
    Sound_SetMasterVolume(g_Config.sound_volume);
}

bool Config_Write(void)
{
    LOG_INFO("Saving user settings");

    MYFILE *fp = File_Open(m_TR1XGlobalSettingsPath, FILE_OPEN_WRITE);
    if (!fp) {
        return false;
    }

    size_t size;
    struct json_object_s *root_obj = json_object_new();

    const CONFIG_OPTION *opt = g_ConfigOptionMap;
    while (opt->target) {
        if (opt->type == COT_BOOL) {
            json_object_append_bool(root_obj, opt->name, *(bool *)opt->target);
        } else if (opt->type == COT_INT32) {
            json_object_append_int(
                root_obj, opt->name, *(int32_t *)opt->target);
        } else if (opt->type == COT_FLOAT) {
            json_object_append_double(
                root_obj, opt->name, *(float *)opt->target);
        } else if (opt->type == COT_DOUBLE) {
            json_object_append_double(
                root_obj, opt->name, *(double *)opt->target);
        } else if (opt->type == COT_ENUM) {
            Config_WriteEnum(
                root_obj, opt->name, *(int *)opt->target,
                (const CONFIG_OPTION_ENUM_MAP *)opt->param);
        }
        opt++;
    }

    char layout_name[20];
    for (INPUT_LAYOUT layout = INPUT_LAYOUT_CUSTOM_1;
         layout < INPUT_LAYOUT_NUMBER_OF; layout++) {
        struct json_array_s *layout_arr = json_array_new();
        sprintf(layout_name, "layout_%d", layout);
        for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
            json_array_append_int(
                layout_arr, Input_GetAssignedScancode(layout, role));
        }
        json_object_append_array(root_obj, layout_name, layout_arr);
    }

    for (INPUT_LAYOUT layout = INPUT_LAYOUT_CUSTOM_1;
         layout < INPUT_LAYOUT_NUMBER_OF; layout++) {
        struct json_array_s *cntlr_arr = json_array_new();
        sprintf(layout_name, "cntlr_layout_%d", layout);
        for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
            json_array_append_int(
                cntlr_arr, Input_GetAssignedButtonType(layout, role));
            json_array_append_int(
                cntlr_arr, Input_GetAssignedBind(layout, role));
            json_array_append_int(
                cntlr_arr, Input_GetAssignedAxisDir(layout, role));
        }
        json_object_append_array(root_obj, layout_name, cntlr_arr);
    }

    struct json_value_s *root = json_value_from_object(root_obj);
    char *data = json_write_pretty(root, "  ", "\n", &size);
    json_value_free(root);

    File_Write(data, sizeof(char), size - 1, fp);
    File_Close(fp);
    Memory_FreePointer(&data);

    return true;
}
