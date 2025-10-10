#include "config/common.h"
#include "config/file.h"
#include "config/priv.h"
#include "config/vars.h"
#include "debug.h"
#include "game/clock.h"
#include "game/gym.h"
#include "game/input.h"
#include "log.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>

static void M_LoadInputLayout(
    JSON_OBJECT *const parent_obj, const INPUT_BACKEND backend,
    const INPUT_LAYOUT layout)
{
    char layout_name[20];
    sprintf(layout_name, "layout_%d", layout);
    JSON_ARRAY *const arr = JSON_ObjectGetArray(parent_obj, layout_name);
    if (arr == nullptr) {
        return;
    }

    for (size_t i = 0; i < arr->length; i++) {
        JSON_OBJECT *const bind_obj = JSON_ArrayGetObject(arr, i);
        ASSERT(bind_obj != nullptr);
        Input_AssignFromJSONObject(backend, layout, bind_obj);
    }
}

static void M_LoadInputConfig(JSON_OBJECT *const root_obj)
{
    JSON_OBJECT *const input_obj = JSON_ObjectGetObject(root_obj, "input");
    if (input_obj == nullptr) {
        return;
    }

    JSON_OBJECT *const keyboard_obj =
        JSON_ObjectGetObject(input_obj, "keyboard");
    JSON_OBJECT *const controller_obj =
        JSON_ObjectGetObject(input_obj, "controller");
    for (INPUT_LAYOUT layout = INPUT_LAYOUT_CUSTOM_1;
         layout < INPUT_LAYOUT_NUMBER_OF; layout++) {
        if (keyboard_obj != nullptr) {
            M_LoadInputLayout(keyboard_obj, INPUT_BACKEND_KEYBOARD, layout);
        }
        if (controller_obj != nullptr) {
            M_LoadInputLayout(controller_obj, INPUT_BACKEND_CONTROLLER, layout);
        }
    }
}

static void M_DumpInputLayout(
    JSON_OBJECT *const parent_obj, const INPUT_BACKEND backend,
    const INPUT_LAYOUT layout)
{
    JSON_ARRAY *const arr = JSON_ArrayNew();

    bool has_elements = false;
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        JSON_OBJECT *const bind_obj = JSON_ObjectNew();
        if (Input_AssignToJSONObject(backend, layout, bind_obj, role)) {
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

static void M_DumpInputConfig(JSON_OBJECT *const root_obj)
{
    JSON_OBJECT *const input_obj = JSON_ObjectNew();
    JSON_OBJECT *const keyboard_obj = JSON_ObjectNew();
    JSON_OBJECT *const controller_obj = JSON_ObjectNew();
    JSON_ObjectAppendObject(root_obj, "input", input_obj);
    JSON_ObjectAppendObject(input_obj, "keyboard", keyboard_obj);
    JSON_ObjectAppendObject(input_obj, "controller", controller_obj);
    for (INPUT_LAYOUT layout = INPUT_LAYOUT_CUSTOM_1;
         layout < INPUT_LAYOUT_NUMBER_OF; layout++) {
        M_DumpInputLayout(keyboard_obj, INPUT_BACKEND_KEYBOARD, layout);
        M_DumpInputLayout(controller_obj, INPUT_BACKEND_CONTROLLER, layout);
    }
}

static void M_LoadLegacyOptions(JSON_OBJECT *const parent_obj)
{
#define READ_FALLBACK_BOOL(target, key)                                        \
    target = JSON_ObjectGetBool(parent_obj, key, target)

    // ..0.10
    READ_FALLBACK_BOOL(g_Config.visuals.use_ps1_fov, "use_pcx_fov");
    // ..1.5
    READ_FALLBACK_BOOL(g_Config.visuals.use_ps1_fov, "use_psx_fov");

    // ..1.1
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

    // 1.5
    {
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

    // ..1.5.1
    {
        const JSON_VALUE *const value =
            JSON_ObjectGetValue(parent_obj, "fix_pickup_drift_glitch");
        if (JSON_ValueIsFalse(value)) {
            g_Config.gameplay.fix_lara_pickup_embed = false;
        }
    }
}

void Config_LoadFromJSON(JSON_OBJECT *root_obj)
{
    ConfigFile_LoadOptions(root_obj, Config_GetOptionMap());
    if (Gym_HasAssaultStats()) {
        ConfigFile_LoadAssaultStats(root_obj, &g_Config.profile.assault_stats);
    }
    M_LoadInputConfig(root_obj);
    M_LoadLegacyOptions(root_obj);
}

void Config_DumpToJSON(JSON_OBJECT *root_obj)
{
    ConfigFile_DumpOptions(root_obj, Config_GetOptionMap());
    if (Gym_HasAssaultStats()) {
        ConfigFile_DumpAssaultStats(root_obj, &g_Config.profile.assault_stats);
    }
    M_DumpInputConfig(root_obj);
}

void Config_Sanitize(void)
{
    Config_SanitizeCommon();
    if (g_Config.rendering.aspect_mode != AM_ANY
        && g_Config.rendering.aspect_mode != AM_16_9
        && g_Config.rendering.aspect_mode != AM_16_10) {
        g_Config.rendering.aspect_mode = AM_4_3;
    }
    CLAMP(g_Config.visuals.fov, 30, 150);
}
