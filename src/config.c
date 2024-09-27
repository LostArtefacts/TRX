#include "config.h"

#include "config_map.h"
#include "game/clock.h"
#include "game/input.h"
#include "game/music.h"
#include "game/output.h"
#include "game/requester.h"
#include "game/sound.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/config/file.h>
#include <libtrx/enum_map.h>
#include <libtrx/filesystem.h>
#include <libtrx/game/console/common.h>
#include <libtrx/game/ui/events.h>
#include <libtrx/json.h>
#include <libtrx/memory.h>
#include <libtrx/utils.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

CONFIG g_Config = { 0 };

static const char *m_ConfigPath = "cfg/TR1X.json5";

static void M_LoadKeyboardLayout(JSON_OBJECT *parent_obj, INPUT_LAYOUT layout);
static void M_LoadControllerLayout(
    JSON_OBJECT *parent_obj, INPUT_LAYOUT layout);
static void M_LoadLegacyOptions(JSON_OBJECT *const parent_obj);
static void M_DumpKeyboardLayout(JSON_OBJECT *parent_obj, INPUT_LAYOUT layout);
static void M_DumpControllerLayout(
    JSON_OBJECT *parent_obj, INPUT_LAYOUT layout);

static void M_LoadKeyboardLayout(
    JSON_OBJECT *const parent_obj, const INPUT_LAYOUT layout)
{
    char layout_name[20];
    sprintf(layout_name, "layout_%d", layout);
    JSON_ARRAY *const arr = JSON_ObjectGetArray(parent_obj, layout_name);
    if (!arr) {
        return;
    }

    const JSON_VALUE *const first_value = JSON_ArrayGetValue(arr, 0);
    if (first_value != NULL && first_value->type == JSON_TYPE_NUMBER) {
        // legacy config for versions <= 3.1.1
        for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
            INPUT_SCANCODE scancode = Input_GetAssignedScancode(layout, role);
            scancode = JSON_ArrayGetInt(arr, role, scancode);
            Input_AssignScancode(layout, role, scancode);
        }
    } else {
        // version 4.0+
        for (size_t i = 0; i < arr->length; i++) {
            JSON_OBJECT *const bind_obj = JSON_ArrayGetObject(arr, i);
            if (!bind_obj) {
                continue;
            }

            const INPUT_ROLE role = JSON_ObjectGetInt(bind_obj, "role", -1);
            if (role == (INPUT_ROLE)-1) {
                continue;
            }

            INPUT_SCANCODE scancode = Input_GetAssignedScancode(layout, role);
            scancode = JSON_ObjectGetInt(bind_obj, "scancode", scancode);

            Input_AssignScancode(layout, role, scancode);
        }
    }
}

static void M_LoadControllerLayout(
    JSON_OBJECT *const parent_obj, const INPUT_LAYOUT layout)
{
    char layout_name[20];
    sprintf(layout_name, "cntlr_layout_%d", layout);
    JSON_ARRAY *const arr = JSON_ObjectGetArray(parent_obj, layout_name);
    if (!arr) {
        return;
    }

    const JSON_VALUE *const first_value = JSON_ArrayGetValue(arr, 0);
    if (first_value != NULL && first_value->type == JSON_TYPE_NUMBER) {
        // legacy config for versions <= 3.1.1
        int i = 0;
        for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
            int16_t type = Input_GetAssignedButtonType(layout, role);
            type = JSON_ArrayGetInt(arr, i, type);
            i++;

            int16_t bind = Input_GetAssignedBind(layout, role);
            bind = JSON_ArrayGetInt(arr, i, bind);
            i++;

            int16_t axis_dir = Input_GetAssignedAxisDir(layout, role);
            axis_dir = JSON_ArrayGetInt(arr, i, axis_dir);
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
            JSON_OBJECT *const bind_obj = JSON_ArrayGetObject(arr, i);
            if (!bind_obj) {
                continue;
            }

            const INPUT_ROLE role = JSON_ObjectGetInt(bind_obj, "role", -1);
            if (role == (INPUT_ROLE)-1) {
                continue;
            }

            int16_t type = Input_GetAssignedButtonType(layout, role);
            type = JSON_ObjectGetInt(bind_obj, "button_type", type);

            int16_t bind = Input_GetAssignedBind(layout, role);
            bind = JSON_ObjectGetInt(bind_obj, "bind", bind);

            int16_t axis_dir = Input_GetAssignedAxisDir(layout, role);
            axis_dir = JSON_ObjectGetInt(bind_obj, "axis_dir", axis_dir);

            if (type == BT_BUTTON) {
                Input_AssignButton(layout, role, bind);
            } else {
                Input_AssignAxis(layout, role, bind, axis_dir);
            }
        }
    }
}

static void M_LoadLegacyOptions(JSON_OBJECT *const parent_obj)
{
    // 0.10..4.0.3: enable_enemy_healthbar
    {
        const JSON_VALUE *const value =
            JSON_ObjectGetValue(parent_obj, "enable_enemy_healthbar");
        if (JSON_ValueIsTrue(value)) {
            g_Config.enemy_healthbar_show_mode = BSM_ALWAYS;
        } else if (JSON_ValueIsFalse(value)) {
            g_Config.enemy_healthbar_show_mode = BSM_NEVER;
        }
    }

    // ..4.1.2: healthbar_show_mode, airbar_show_mode, enemy_healthbar_show_mode
    {
        g_Config.healthbar_show_mode = ConfigFile_ReadEnum(
            parent_obj, "healthbar_showing_mode", g_Config.healthbar_show_mode,
            ENUM_MAP_NAME(BAR_SHOW_MODE));
        g_Config.airbar_show_mode = ConfigFile_ReadEnum(
            parent_obj, "airbar_showing_mode", g_Config.airbar_show_mode,
            ENUM_MAP_NAME(BAR_SHOW_MODE));
        g_Config.enemy_healthbar_show_mode = ConfigFile_ReadEnum(
            parent_obj, "enemy_healthbar_showing_mode",
            g_Config.enemy_healthbar_show_mode, ENUM_MAP_NAME(BAR_SHOW_MODE));
    }
}

static void M_DumpKeyboardLayout(
    JSON_OBJECT *const parent_obj, const INPUT_LAYOUT layout)
{
    JSON_ARRAY *const arr = JSON_ArrayNew();

    bool has_elements = false;
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        const INPUT_SCANCODE default_scancode =
            Input_GetAssignedScancode(INPUT_LAYOUT_DEFAULT, role);
        const INPUT_SCANCODE user_scancode =
            Input_GetAssignedScancode(layout, role);

        if (user_scancode == default_scancode) {
            continue;
        }

        JSON_OBJECT *const bind_obj = JSON_ObjectNew();
        JSON_ObjectAppendInt(bind_obj, "role", role);
        JSON_ObjectAppendInt(bind_obj, "scancode", user_scancode);
        JSON_ArrayAppendObject(arr, bind_obj);
        has_elements = true;
    }

    if (has_elements) {
        char layout_name[20];
        sprintf(layout_name, "layout_%d", layout);
        JSON_ObjectAppendArray(parent_obj, layout_name, arr);
    } else {
        JSON_ArrayFree(arr);
    }
}

static void M_DumpControllerLayout(
    JSON_OBJECT *const parent_obj, const INPUT_LAYOUT layout)
{
    JSON_ARRAY *const arr = JSON_ArrayNew();

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

        JSON_OBJECT *const bind_obj = JSON_ObjectNew();
        JSON_ObjectAppendInt(bind_obj, "role", role);
        JSON_ObjectAppendInt(bind_obj, "button_type", user_button_type);
        JSON_ObjectAppendInt(bind_obj, "bind", user_bind);
        JSON_ObjectAppendInt(bind_obj, "axis_dir", user_axis_dir);
        JSON_ArrayAppendObject(arr, bind_obj);
        has_elements = true;
    }

    if (has_elements) {
        char layout_name[20];
        sprintf(layout_name, "cntlr_layout_%d", layout);
        JSON_ObjectAppendArray(parent_obj, layout_name, arr);
    } else {
        JSON_ArrayFree(arr);
    }
}

const char *Config_GetPath(void)
{
    return m_ConfigPath;
}

void Config_LoadFromJSON(JSON_OBJECT *root_obj)
{
    ConfigFile_LoadOptions(root_obj, g_ConfigOptionMap);

    for (INPUT_LAYOUT layout = INPUT_LAYOUT_CUSTOM_1;
         layout < INPUT_LAYOUT_NUMBER_OF; layout++) {
        M_LoadKeyboardLayout(root_obj, layout);
        M_LoadControllerLayout(root_obj, layout);
    }

    M_LoadLegacyOptions(root_obj);

    g_Config.loaded = true;
}

void Config_DumpToJSON(JSON_OBJECT *root_obj)
{
    ConfigFile_DumpOptions(root_obj, g_ConfigOptionMap);

    for (INPUT_LAYOUT layout = INPUT_LAYOUT_CUSTOM_1;
         layout < INPUT_LAYOUT_NUMBER_OF; layout++) {
        M_DumpKeyboardLayout(root_obj, layout);
    }

    for (INPUT_LAYOUT layout = INPUT_LAYOUT_CUSTOM_1;
         layout < INPUT_LAYOUT_NUMBER_OF; layout++) {
        M_DumpControllerLayout(root_obj, layout);
    }
}

void Config_Sanitize(void)
{
    CLAMP(g_Config.start_lara_hitpoints, 1, LARA_MAX_HITPOINTS);
    CLAMP(g_Config.fov_value, 30, 150);
    CLAMP(g_Config.camera_speed, 1, 10);
    CLAMP(g_Config.music_volume, 0, 10);
    CLAMP(g_Config.sound_volume, 0, 10);
    CLAMP(g_Config.input.layout, 0, INPUT_LAYOUT_NUMBER_OF - 1);
    CLAMP(g_Config.input.cntlr_layout, 0, INPUT_LAYOUT_NUMBER_OF - 1);
    CLAMP(g_Config.brightness, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
    CLAMP(g_Config.ui.text_scale, MIN_TEXT_SCALE, MAX_TEXT_SCALE);
    CLAMP(g_Config.ui.bar_scale, MIN_BAR_SCALE, MAX_BAR_SCALE);
    CLAMP(
        g_Config.rendering.turbo_speed, CLOCK_TURBO_SPEED_MIN,
        CLOCK_TURBO_SPEED_MAX);
    CLAMPL(g_Config.rendering.anisotropy_filter, 1.0);
    CLAMP(g_Config.rendering.wireframe_width, 1.0, 100.0);

    if (g_Config.rendering.fps != 30 && g_Config.rendering.fps != 60) {
        g_Config.rendering.fps = 30;
    }
}

void Config_ApplyChanges(void)
{
    Input_CheckConflicts(CM_KEYBOARD, g_Config.input.layout);
    Input_CheckConflicts(CM_CONTROLLER, g_Config.input.cntlr_layout);
    Music_SetVolume(g_Config.music_volume);
    Sound_SetMasterVolume(g_Config.sound_volume);
    Requester_Shutdown(&g_SavegameRequester);
    Requester_Init(&g_SavegameRequester, g_Config.maximum_save_slots);
    Output_ApplyRenderSettings();
    Viewport_SetFOV(Viewport_GetUserFOV());
}

const CONFIG_OPTION *Config_GetOptionMap(void)
{
    return g_ConfigOptionMap;
}
