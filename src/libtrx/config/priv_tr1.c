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
#include <string.h>

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
            g_Config.ui.enemy_health_bar.show_mode = BSM_ALWAYS;
        } else if (JSON_ValueIsFalse(value)) {
            g_Config.ui.enemy_health_bar.show_mode = BSM_NEVER;
        }
    }

    // ..4.1.2: healthbar_show_mode, airbar_show_mode, enemy_healthbar_show_mode
    {
        g_Config.ui.lara_health_bar.show_mode = ConfigFile_ReadEnum(
            parent_obj, "healthbar_showing_mode",
            g_Config.ui.lara_health_bar.show_mode,
            ENUM_MAP_NAME(BAR_SHOW_MODE));
        g_Config.ui.lara_air_bar.show_mode = ConfigFile_ReadEnum(
            parent_obj, "airbar_showing_mode",
            g_Config.ui.lara_air_bar.show_mode, ENUM_MAP_NAME(BAR_SHOW_MODE));
        g_Config.ui.enemy_health_bar.show_mode = ConfigFile_ReadEnum(
            parent_obj, "enemy_healthbar_showing_mode",
            g_Config.ui.enemy_health_bar.show_mode,
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
    READ_FALLBACK_BOOL(g_Config.gameplay.enable_cutscenes, "enable_cine");
    READ_FALLBACK_BOOL(g_Config.gameplay.enable_legal, "enable_eidos_logo");

    // ..4.11: 0…10 scale volumes to 0.0f…1.0f and convert wall bug fix
    {
        const JSON_VALUE *const value =
            JSON_ObjectGetValue(parent_obj, "sound_volume");
        const JSON_NUMBER *const num =
            value != nullptr ? JSON_ValueGetNumber(value) : nullptr;
        if (num != nullptr && strchr(num->number, '.') == nullptr) {
            g_Config.audio.sound_volume = JSON_ValueGetInt(value, 0) / 10.0f;
        }
    }
    {
        const JSON_VALUE *const value =
            JSON_ObjectGetValue(parent_obj, "music_volume");
        const JSON_NUMBER *const num =
            value != nullptr ? JSON_ValueGetNumber(value) : nullptr;
        if (value != nullptr && value->type == JSON_TYPE_NUMBER
            && strchr(num->number, '.') == nullptr) {
            g_Config.audio.music_volume = JSON_ValueGetInt(value, 0) / 10.0f;
        }
    }
    {
        const JSON_VALUE *const value =
            JSON_ObjectGetValue(parent_obj, "fix_wall_jump_glitch");
        if (JSON_ValueIsTrue(value)) {
            g_Config.gameplay.wall_glitch_mode = WALL_GLITCH_FIXED;
        }
    }

    // ..4.13: convert enhanced look to enum type
    {
        const JSON_VALUE *const value =
            JSON_ObjectGetValue(parent_obj, "enable_enhanced_look");
        if (JSON_ValueIsTrue(value)) {
            g_Config.gameplay.look_mode = LOOK_MODE_UNRESTRICTED;
        } else if (JSON_ValueIsFalse(value)) {
            g_Config.gameplay.look_mode = LOOK_MODE_RESTRICTED;
        }
    }

    // ..4.15
    {
        if (JSON_ObjectGetValue(parent_obj, "master_volume") == nullptr) {
            g_Config.audio.master_volume = 1.0;
        }
        if (JSON_ObjectGetValue(parent_obj, "ambient_volume") == nullptr) {
            g_Config.audio.ambient_volume = g_Config.audio.music_volume;
        }
        if (JSON_ObjectGetValue(parent_obj, "cutscene_volume") == nullptr) {
            g_Config.audio.cutscene_volume = g_Config.audio.music_volume;
        }
        if (JSON_ObjectGetValue(parent_obj, "fmv_volume") == nullptr) {
            g_Config.audio.fmv_volume = g_Config.audio.music_volume;
        }
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

void Config_LoadFromJSON(JSON_OBJECT *root_obj)
{
    ConfigFile_LoadOptions(root_obj, Config_GetOptionMap());
    if (Gym_HasAssaultStats()) {
        ConfigFile_LoadAssaultStats(root_obj, &g_Config.profile.assault_stats);
    }

    for (INPUT_LAYOUT layout = INPUT_LAYOUT_CUSTOM_1;
         layout < INPUT_LAYOUT_NUMBER_OF; layout++) {
        M_LoadKeyboardLayout(root_obj, layout);
        M_LoadControllerLayout(root_obj, layout);
    }

    M_LoadLegacyOptions(root_obj);
}

void Config_DumpToJSON(JSON_OBJECT *root_obj)
{
    ConfigFile_DumpOptions(root_obj, Config_GetOptionMap());
    if (Gym_HasAssaultStats()) {
        ConfigFile_DumpAssaultStats(root_obj, &g_Config.profile.assault_stats);
    }

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
    Config_SanitizeCommon();
    CLAMP(g_Config.visuals.fov_value, 30, 150);
    CLAMPL(g_Config.gameplay.maximum_save_slots, 0);
}
