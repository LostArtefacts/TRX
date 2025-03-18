#include "config/common.h"
#include "config/file.h"
#include "config/priv.h"
#include "config/vars.h"
#include "debug.h"
#include "game/clock.h"
#include "game/input.h"
#include "log.h"
#include "utils.h"

#include <stdio.h>

static void M_LoadInputConfig(JSON_OBJECT *root_obj);
static void M_LoadInputLayout(
    JSON_OBJECT *parent_obj, INPUT_BACKEND backend, INPUT_LAYOUT layout);
static void M_DumpInputConfig(JSON_OBJECT *root_obj);
static void M_DumpInputLayout(
    JSON_OBJECT *parent_obj, INPUT_BACKEND backend, INPUT_LAYOUT layout);

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

void Config_LoadFromJSON(JSON_OBJECT *root_obj)
{
    ConfigFile_LoadOptions(root_obj, Config_GetOptionMap());
    M_LoadInputConfig(root_obj);
    g_Config.loaded = true;
    g_SavedConfig = g_Config;
}

void Config_DumpToJSON(JSON_OBJECT *root_obj)
{
    ConfigFile_DumpOptions(root_obj, Config_GetOptionMap());
    M_DumpInputConfig(root_obj);
}

void Config_Sanitize(void)
{
    CLAMP(
        g_Config.gameplay.turbo_speed, CLOCK_TURBO_SPEED_MIN,
        CLOCK_TURBO_SPEED_MAX);
    CLAMP(g_Config.rendering.scaler, 1, 4);

    if (g_Config.rendering.render_mode != RM_HARDWARE
        && g_Config.rendering.render_mode != RM_SOFTWARE) {
        g_Config.rendering.render_mode = RM_SOFTWARE;
    }
    if (g_Config.rendering.aspect_mode != AM_ANY
        && g_Config.rendering.aspect_mode != AM_16_9) {
        g_Config.rendering.aspect_mode = AM_4_3;
    }
    if (g_Config.rendering.texel_adjust_mode != TAM_DISABLED
        && g_Config.rendering.texel_adjust_mode != TAM_BILINEAR_ONLY) {
        g_Config.rendering.texel_adjust_mode = TAM_ALWAYS;
    }
    CLAMP(g_Config.rendering.nearest_adjustment, 0, 256);
    CLAMP(g_Config.rendering.linear_adjustment, 0, 256);
    if (g_Config.rendering.fps != 30 && g_Config.rendering.fps != 60) {
        g_Config.rendering.fps = 30;
    }

    CLAMP(g_Config.visuals.fov, 30, 150);
    CLAMP(g_Config.ui.bar_scale, 0.5, 2.0);
    CLAMP(g_Config.ui.text_scale, 0.5, 2.0);
}
