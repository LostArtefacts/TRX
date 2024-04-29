#include "config.h"

#include "config_map.h"
#include "game/input.h"
#include "game/music.h"
#include "game/sound.h"
#include "gfx/context.h"
#include "global/const.h"
#include "global/types.h"
#include "shared/filesystem.h"
#include "shared/json.h"
#include "shared/log.h"
#include "shared/memory.h"
#include "util.h"

#include <stdio.h>
#include <string.h>

CONFIG g_Config = { 0 };

static const char *m_TR1XGlobalSettingsPath = "cfg/TR1X.json5";

static int Config_ReadEnum(
    struct json_object_s *obj, const char *name, int default_value,
    const CONFIG_OPTION_ENUM_MAP *enum_map);
static void Config_WriteEnum(
    struct json_object_s *obj, const char *name, int value,
    const CONFIG_OPTION_ENUM_MAP *enum_map);

static void Config_ReadKeyboardLayout(
    struct json_object_s *parent_obj, INPUT_LAYOUT layout);
static void Config_ReadControllerLayout(
    struct json_object_s *parent_obj, INPUT_LAYOUT layout);
static void Config_ReadLegacyOptions(struct json_object_s *const parent_obj);
static void Config_WriteKeyboardLayout(
    struct json_object_s *parent_obj, INPUT_LAYOUT layout);
static void Config_WriteControllerLayout(
    struct json_object_s *parent_obj, INPUT_LAYOUT layout);

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

static void Config_ReadKeyboardLayout(
    struct json_object_s *const parent_obj, const INPUT_LAYOUT layout)
{
    char layout_name[20];
    sprintf(layout_name, "layout_%d", layout);
    struct json_array_s *const arr =
        json_object_get_array(parent_obj, layout_name);
    if (!arr) {
        return;
    }

    const struct json_value_s *const first_value = json_array_get_value(arr, 0);
    if (first_value != json_null && first_value->type == json_type_number) {
        // legacy config for versions <= 3.1.1
        for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
            INPUT_SCANCODE scancode = Input_GetAssignedScancode(layout, role);
            scancode = json_array_get_int(arr, role, scancode);
            Input_AssignScancode(layout, role, scancode);
        }
    } else {
        // version 4.0+
        for (size_t i = 0; i < arr->length; i++) {
            struct json_object_s *const bind_obj =
                json_array_get_object(arr, i);
            if (!bind_obj) {
                continue;
            }

            const INPUT_ROLE role = json_object_get_int(bind_obj, "role", -1);
            if (role == (INPUT_ROLE)-1) {
                continue;
            }

            INPUT_SCANCODE scancode = Input_GetAssignedScancode(layout, role);
            scancode = json_object_get_int(bind_obj, "scancode", scancode);

            Input_AssignScancode(layout, role, scancode);
        }
    }
}

static void Config_ReadControllerLayout(
    struct json_object_s *const parent_obj, const INPUT_LAYOUT layout)
{
    char layout_name[20];
    sprintf(layout_name, "cntlr_layout_%d", layout);
    struct json_array_s *const arr =
        json_object_get_array(parent_obj, layout_name);
    if (!arr) {
        return;
    }

    const struct json_value_s *const first_value = json_array_get_value(arr, 0);
    if (first_value != json_null && first_value->type == json_type_number) {
        // legacy config for versions <= 3.1.1
        int i = 0;
        for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
            int16_t type = Input_GetAssignedButtonType(layout, role);
            type = json_array_get_int(arr, i, type);
            i++;

            int16_t bind = Input_GetAssignedBind(layout, role);
            bind = json_array_get_int(arr, i, bind);
            i++;

            int16_t axis_dir = Input_GetAssignedAxisDir(layout, role);
            axis_dir = json_array_get_int(arr, i, axis_dir);
            i++;

            if (type == BT_BUTTON) {
                Input_AssignButton(layout, role, bind);
            } else {
                Input_AssignAxis(layout, role, bind, axis_dir);
            }
        }
    } else {
        // version 4.0+
        for (size_t i = 0; i < arr->length; i++) {
            struct json_object_s *const bind_obj =
                json_array_get_object(arr, i);
            if (!bind_obj) {
                continue;
            }

            const INPUT_ROLE role = json_object_get_int(bind_obj, "role", -1);
            if (role == (INPUT_ROLE)-1) {
                continue;
            }

            int16_t type = Input_GetAssignedButtonType(layout, role);
            type = json_object_get_int(bind_obj, "button_type", type);

            int16_t bind = Input_GetAssignedBind(layout, role);
            bind = json_object_get_int(bind_obj, "bind", bind);

            int16_t axis_dir = Input_GetAssignedAxisDir(layout, role);
            axis_dir = json_object_get_int(bind_obj, "axis_dir", axis_dir);

            if (type == BT_BUTTON) {
                Input_AssignButton(layout, role, bind);
            } else {
                Input_AssignAxis(layout, role, bind, axis_dir);
            }
        }
    }
}

static void Config_ReadLegacyOptions(struct json_object_s *const parent_obj)
{
    // 0.10..4.0.3: enable_enemy_healthbar
    {
        const struct json_value_s *const value =
            json_object_get_value(parent_obj, "enable_enemy_healthbar");
        if (json_value_is_true(value)) {
            g_Config.enemy_healthbar_show_mode = BSM_ALWAYS;
        } else if (json_value_is_false(value)) {
            g_Config.enemy_healthbar_show_mode = BSM_NEVER;
        }
    }
}

static void Config_WriteKeyboardLayout(
    struct json_object_s *const parent_obj, const INPUT_LAYOUT layout)
{
    struct json_array_s *const arr = json_array_new();

    bool has_elements = false;
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        const INPUT_SCANCODE default_scancode =
            Input_GetAssignedScancode(INPUT_LAYOUT_DEFAULT, role);
        const INPUT_SCANCODE user_scancode =
            Input_GetAssignedScancode(layout, role);

        if (user_scancode == default_scancode) {
            continue;
        }

        struct json_object_s *const bind_obj = json_object_new();
        json_object_append_int(bind_obj, "role", role);
        json_object_append_int(bind_obj, "scancode", user_scancode);
        json_array_append_object(arr, bind_obj);
        has_elements = true;
    }

    if (has_elements) {
        char layout_name[20];
        sprintf(layout_name, "layout_%d", layout);
        json_object_append_array(parent_obj, layout_name, arr);
    } else {
        json_array_free(arr);
    }
}

static void Config_WriteControllerLayout(
    struct json_object_s *const parent_obj, const INPUT_LAYOUT layout)
{
    struct json_array_s *const arr = json_array_new();

    bool has_elements = false;
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        const int16_t default_button_type =
            Input_GetAssignedButtonType(INPUT_LAYOUT_DEFAULT, role);
        const int16_t default_axis_dir =
            Input_GetAssignedAxisDir(INPUT_LAYOUT_DEFAULT, role);
        const int16_t default_bind =
            Input_GetAssignedBind(INPUT_LAYOUT_DEFAULT, role);
        const int16_t user_button_type =
            Input_GetAssignedButtonType(layout, role);
        const int16_t user_axis_dir = Input_GetAssignedAxisDir(layout, role);
        const int16_t user_bind = Input_GetAssignedBind(layout, role);

        if (user_button_type == default_button_type
            && user_axis_dir == default_axis_dir && user_bind == default_bind) {
            continue;
        }

        struct json_object_s *const bind_obj = json_object_new();
        json_object_append_int(bind_obj, "role", role);
        json_object_append_int(bind_obj, "button_type", user_button_type);
        json_object_append_int(bind_obj, "bind", user_bind);
        json_object_append_int(bind_obj, "axis_dir", user_axis_dir);
        json_array_append_object(arr, bind_obj);
        has_elements = true;
    }

    if (has_elements) {
        char layout_name[20];
        sprintf(layout_name, "cntlr_layout_%d", layout);
        json_object_append_array(parent_obj, layout_name, arr);
    } else {
        json_array_free(arr);
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

    for (INPUT_LAYOUT layout = INPUT_LAYOUT_CUSTOM_1;
         layout < INPUT_LAYOUT_NUMBER_OF; layout++) {
        Config_ReadKeyboardLayout(root_obj, layout);
        Config_ReadControllerLayout(root_obj, layout);
    }

    Config_ReadLegacyOptions(root_obj);

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

    for (INPUT_LAYOUT layout = INPUT_LAYOUT_CUSTOM_1;
         layout < INPUT_LAYOUT_NUMBER_OF; layout++) {
        Config_WriteKeyboardLayout(root_obj, layout);
    }

    for (INPUT_LAYOUT layout = INPUT_LAYOUT_CUSTOM_1;
         layout < INPUT_LAYOUT_NUMBER_OF; layout++) {
        Config_WriteControllerLayout(root_obj, layout);
    }

    struct json_value_s *root = json_value_from_object(root_obj);
    char *data = json_write_pretty(root, "  ", "\n", &size);
    json_value_free(root);

    File_Write(data, sizeof(char), size - 1, fp);
    File_Close(fp);
    Memory_FreePointer(&data);

    return true;
}
