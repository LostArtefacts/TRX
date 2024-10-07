#include "config.h"

#include "config_map.h"
#include "game/clock.h"
#include "game/input.h"
#include "game/music.h"
#include "game/output.h"
#include "game/requester.h"
#include "game/sound.h"
#include "global/vars.h"

#include <libtrx/config/file.h>
#include <libtrx/log.h>

#include <stdio.h>

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
    if (arr == NULL) {
        return;
    }

    for (size_t i = 0; i < arr->length; i++) {
        JSON_OBJECT *const bind_obj = JSON_ArrayGetObject(arr, i);
        if (bind_obj == NULL) {
            // this can happen on TR1X <= 3.1.1, which is no longer supported
            LOG_WARNING("unsupported keyboard layout config");
            continue;
        }

        Input_AssignFromJSONObject(INPUT_BACKEND_KEYBOARD, layout, bind_obj);
    }
}

static void M_LoadControllerLayout(
    JSON_OBJECT *const parent_obj, const INPUT_LAYOUT layout)
{
    char layout_name[20];
    sprintf(layout_name, "cntlr_layout_%d", layout);
    JSON_ARRAY *const arr = JSON_ObjectGetArray(parent_obj, layout_name);
    if (arr == NULL) {
        return;
    }

    for (size_t i = 0; i < arr->length; i++) {
        JSON_OBJECT *const bind_obj = JSON_ArrayGetObject(arr, i);
        if (bind_obj == NULL) {
            // this can happen on TR1X <= 3.1.1, which is no longer supported
            LOG_WARNING("unsupported controller layout config");
            continue;
        }

        Input_AssignFromJSONObject(INPUT_BACKEND_CONTROLLER, layout, bind_obj);
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
        JSON_OBJECT *const bind_obj = JSON_ObjectNew();
        if (Input_AssignToJSONObject(
                INPUT_BACKEND_KEYBOARD, layout, bind_obj, role)) {
            has_elements = true;
            JSON_ArrayAppendObject(arr, bind_obj);
        } else {
            JSON_ObjectFree(bind_obj);
        }
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
        JSON_OBJECT *const bind_obj = JSON_ObjectNew();
        if (Input_AssignToJSONObject(
                INPUT_BACKEND_CONTROLLER, layout, bind_obj, role)) {
            has_elements = true;
            JSON_ArrayAppendObject(arr, bind_obj);
        } else {
            JSON_ObjectFree(bind_obj);
        }
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
    Music_SetVolume(g_Config.music_volume);
    Sound_SetMasterVolume(g_Config.sound_volume);
    Requester_Shutdown(&g_SavegameRequester);
    Requester_Init(&g_SavegameRequester, g_Config.maximum_save_slots);
    Output_ApplyRenderSettings();
}

const CONFIG_OPTION *Config_GetOptionMap(void)
{
    return g_ConfigOptionMap;
}
