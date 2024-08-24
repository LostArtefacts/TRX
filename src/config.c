#include "config.h"

#include "config_map.h"
#include "game/input.h"
#include "game/music.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/enum_str.h"
#include "global/types.h"

#include <libtrx/config/config_file.h>
#include <libtrx/filesystem.h>
#include <libtrx/json.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>
#include <libtrx/utils.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

CONFIG g_Config = { 0 };

static const char *m_ConfigPath = "cfg/TR1X.json5";

static void Config_LoadKeyboardLayout(
    struct json_object_s *parent_obj, INPUT_LAYOUT layout);
static void Config_LoadControllerLayout(
    struct json_object_s *parent_obj, INPUT_LAYOUT layout);
static void Config_LoadLegacyOptions(struct json_object_s *const parent_obj);
static void Config_DumpKeyboardLayout(
    struct json_object_s *parent_obj, INPUT_LAYOUT layout);
static void Config_DumpControllerLayout(
    struct json_object_s *parent_obj, INPUT_LAYOUT layout);

static void Config_Load(struct json_object_s *root_obj);
static void Config_Dump(struct json_object_s *root_obj);

static void Config_LoadKeyboardLayout(
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

static void Config_LoadControllerLayout(
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

static void Config_LoadLegacyOptions(struct json_object_s *const parent_obj)
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

    // ..4.1.2: healthbar_show_mode, airbar_show_mode, enemy_healthbar_show_mode
    {
        g_Config.healthbar_show_mode = ConfigFile_ReadEnum(
            parent_obj, "healthbar_showing_mode", g_Config.healthbar_show_mode,
            ENUM_STRING_MAP(BAR_SHOW_MODE));
        g_Config.airbar_show_mode = ConfigFile_ReadEnum(
            parent_obj, "airbar_showing_mode", g_Config.airbar_show_mode,
            ENUM_STRING_MAP(BAR_SHOW_MODE));
        g_Config.enemy_healthbar_show_mode = ConfigFile_ReadEnum(
            parent_obj, "enemy_healthbar_showing_mode",
            g_Config.enemy_healthbar_show_mode, ENUM_STRING_MAP(BAR_SHOW_MODE));
    }
}

static void Config_DumpKeyboardLayout(
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

static void Config_DumpControllerLayout(
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

static void Config_Load(struct json_object_s *root_obj)
{
    ConfigFile_LoadOptions(root_obj, g_ConfigOptionMap);

    for (INPUT_LAYOUT layout = INPUT_LAYOUT_CUSTOM_1;
         layout < INPUT_LAYOUT_NUMBER_OF; layout++) {
        Config_LoadKeyboardLayout(root_obj, layout);
        Config_LoadControllerLayout(root_obj, layout);
    }

    Config_LoadLegacyOptions(root_obj);

    CLAMP(g_Config.start_lara_hitpoints, 1, LARA_MAX_HITPOINTS);
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
}

static void Config_Dump(struct json_object_s *root_obj)
{
    ConfigFile_DumpOptions(root_obj, g_ConfigOptionMap);

    for (INPUT_LAYOUT layout = INPUT_LAYOUT_CUSTOM_1;
         layout < INPUT_LAYOUT_NUMBER_OF; layout++) {
        Config_DumpKeyboardLayout(root_obj, layout);
    }

    for (INPUT_LAYOUT layout = INPUT_LAYOUT_CUSTOM_1;
         layout < INPUT_LAYOUT_NUMBER_OF; layout++) {
        Config_DumpControllerLayout(root_obj, layout);
    }
}

bool Config_Read(void)
{
    const bool result = ConfigFile_Read(m_ConfigPath, &Config_Load);
    Input_CheckConflicts(CM_KEYBOARD, g_Config.input.layout);
    Input_CheckConflicts(CM_CONTROLLER, g_Config.input.cntlr_layout);
    Music_SetVolume(g_Config.music_volume);
    Sound_SetMasterVolume(g_Config.sound_volume);
    return result;
}

bool Config_Write(void)
{
    return ConfigFile_Write(m_ConfigPath, &Config_Dump);
}
