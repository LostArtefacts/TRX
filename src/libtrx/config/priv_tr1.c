#include "config/common.h"
#include "config/file.h"
#include "config/priv.h"
#include "config/vars.h"
#include "game/clock.h"
#include "game/input.h"
#include "game/lara/const.h"
#include "log.h"
#include "utils.h"

#include <stdio.h>

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
    if (arr == nullptr) {
        return;
    }

    for (size_t i = 0; i < arr->length; i++) {
        JSON_OBJECT *const bind_obj = JSON_ArrayGetObject(arr, i);
        if (bind_obj == nullptr) {
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
    if (arr == nullptr) {
        return;
    }

    for (size_t i = 0; i < arr->length; i++) {
        JSON_OBJECT *const bind_obj = JSON_ArrayGetObject(arr, i);
        if (bind_obj == nullptr) {
            // this can happen on TR1X <= 3.1.1, which is no longer supported
            LOG_WARNING("unsupported controller layout config");
            continue;
        }

        Input_AssignFromJSONObject(INPUT_BACKEND_CONTROLLER, layout, bind_obj);
    }
}

static void M_LoadLegacyOptions(JSON_OBJECT *const parent_obj)
{
#define READ_FALLBACK_BOOL(target, key)                                        \
    target = JSON_ObjectGetBool(parent_obj, key, target)
#define READ_FALLBACK_INT(target, key)                                         \
    target = JSON_ObjectGetInt(parent_obj, key, target)

    // 0.10..4.0.3: enable_enemy_healthbar
    {
        const JSON_VALUE *const value =
            JSON_ObjectGetValue(parent_obj, "enable_enemy_healthbar");
        if (JSON_ValueIsTrue(value)) {
            g_Config.ui.enemy_healthbar_show_mode = BSM_ALWAYS;
        } else if (JSON_ValueIsFalse(value)) {
            g_Config.ui.enemy_healthbar_show_mode = BSM_NEVER;
        }
    }

    // ..4.1.2: healthbar_show_mode, airbar_show_mode, enemy_healthbar_show_mode
    {
        g_Config.ui.healthbar_show_mode = ConfigFile_ReadEnum(
            parent_obj, "healthbar_showing_mode",
            g_Config.ui.healthbar_show_mode, ENUM_MAP_NAME(BAR_SHOW_MODE));
        g_Config.ui.airbar_show_mode = ConfigFile_ReadEnum(
            parent_obj, "airbar_showing_mode", g_Config.ui.airbar_show_mode,
            ENUM_MAP_NAME(BAR_SHOW_MODE));
        g_Config.ui.enemy_healthbar_show_mode = ConfigFile_ReadEnum(
            parent_obj, "enemy_healthbar_showing_mode",
            g_Config.ui.enemy_healthbar_show_mode,
            ENUM_MAP_NAME(BAR_SHOW_MODE));
    }

    // 2.16..4.5.1 load_current_music
    {
        const JSON_VALUE *const value =
            JSON_ObjectGetValue(parent_obj, "load_current_music");
        if (JSON_ValueIsTrue(value)) {
            g_Config.audio.music_load_condition = MUSIC_LOAD_NON_AMBIENT;
        } else if (JSON_ValueIsFalse(value)) {
            g_Config.audio.music_load_condition = MUSIC_LOAD_NEVER;
        }
    }

    // ..4.7
    READ_FALLBACK_BOOL(g_Config.window.is_fullscreen, "enable_fullscreen");
    READ_FALLBACK_BOOL(g_Config.window.is_maximized, "enable_maximized");
    READ_FALLBACK_BOOL(g_Config.gameplay.enable_walk_to_items, "walk_to_items");
    READ_FALLBACK_BOOL(
        g_Config.gameplay.enable_inverted_look, "enabled_inverted_look");
    READ_FALLBACK_INT(g_Config.window.x, "window_x");
    READ_FALLBACK_INT(g_Config.window.y, "window_y");
    READ_FALLBACK_INT(g_Config.window.width, "window_width");
    READ_FALLBACK_INT(g_Config.window.height, "window_height");
    READ_FALLBACK_INT(g_Config.input.keyboard_layout, "layout");
    READ_FALLBACK_INT(g_Config.input.controller_layout, "cntlr_layout");

    // ..4.9
    READ_FALLBACK_BOOL(g_Config.gameplay.enable_legal, "enable_eidos_logo");
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

void Config_LoadFromJSON(JSON_OBJECT *root_obj)
{
    ConfigFile_LoadOptions(root_obj, Config_GetOptionMap());

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
    ConfigFile_DumpOptions(root_obj, Config_GetOptionMap());

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
    CLAMP(g_Config.gameplay.start_lara_hitpoints, 1, LARA_MAX_HITPOINTS);
    CLAMP(g_Config.visuals.fog_start, 1, 100);
    CLAMP(g_Config.visuals.fog_end, 1, 100);
    CLAMP(g_Config.visuals.fov_value, 30, 150);
    CLAMP(g_Config.gameplay.camera_speed, 1, 10);
    CLAMP(g_Config.audio.music_volume, 0, 10);
    CLAMP(g_Config.audio.sound_volume, 0, 10);
    CLAMP(g_Config.input.keyboard_layout, 0, INPUT_LAYOUT_NUMBER_OF - 1);
    CLAMP(g_Config.input.controller_layout, 0, INPUT_LAYOUT_NUMBER_OF - 1);
    CLAMP(
        g_Config.visuals.brightness, CONFIG_MIN_BRIGHTNESS,
        CONFIG_MAX_BRIGHTNESS);
    CLAMP(g_Config.ui.text_scale, CONFIG_MIN_TEXT_SCALE, CONFIG_MAX_TEXT_SCALE);
    CLAMP(g_Config.ui.bar_scale, CONFIG_MIN_BAR_SCALE, CONFIG_MAX_BAR_SCALE);
    CLAMP(
        g_Config.gameplay.turbo_speed, CLOCK_TURBO_SPEED_MIN,
        CLOCK_TURBO_SPEED_MAX);
    CLAMPL(g_Config.gameplay.maximum_save_slots, 0);
    CLAMPL(g_Config.rendering.anisotropy_filter, 1.0);
    CLAMP(g_Config.rendering.wireframe_width, 1.0, 100.0);

    if (g_Config.rendering.fps != 30 && g_Config.rendering.fps != 60) {
        g_Config.rendering.fps = 30;
    }
}
