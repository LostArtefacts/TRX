#include <trx/config/priv.h>

#include <trx/config/common.h>
#include <trx/config/file.h>
#include <trx/config/vars.h>
#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/clock.h>
#include <trx/game/input.h>
#include <trx/game/lara/const.h>
#include <trx/version.h>

#include <stdio.h>

#define M_CONFIG_VERSION_CURRENT 1

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

    JSON_OBJECT *const touch_obj = JSON_ObjectGetObject(input_obj, "touch");
    if (touch_obj != nullptr) {
        for (INPUT_LAYOUT layout = INPUT_LAYOUT_CUSTOM_1;
             layout < INPUT_LAYOUT_NUMBER_OF; layout++) {
            M_LoadInputLayout(touch_obj, INPUT_BACKEND_TOUCH, layout);
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
        for (int32_t slot = 0; slot < INPUT_BINDING_SLOTS; slot++) {
            JSON_OBJECT *const bind_obj = JSON_ObjectNew();
            if (Input_AssignToJSONObject(
                    backend, layout, bind_obj, role, slot)) {
                has_elements = true;
                JSON_ArrayAppendObject(arr, bind_obj);
            } else {
                JSON_ObjectFree(bind_obj);
            }
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

    JSON_OBJECT *const touch_obj = JSON_ObjectNew();
    JSON_ObjectAppendObject(input_obj, "touch", touch_obj);
    for (INPUT_LAYOUT layout = INPUT_LAYOUT_CUSTOM_1;
         layout < INPUT_LAYOUT_NUMBER_OF; layout++) {
        M_DumpInputLayout(touch_obj, INPUT_BACKEND_TOUCH, layout);
    }
}

static void M_LoadLegacyOptions(JSON_OBJECT *const parent_obj)
{
    // TRX ..1.9: game modes changed to policy.
    if (JSON_ObjectGetValue(parent_obj, "game_modes_policy") == nullptr) {
        const JSON_VALUE *const value =
            JSON_ObjectGetValue(parent_obj, "enable_game_modes");
        g_Config.gameplay.game_modes_policy = JSON_ValueIsTrue(value)
            ? GAME_MODES_POLICY_ALWAYS
            : GAME_MODES_POLICY_NEVER;
    }

    // TRX ..1.10: target change on/off changed to mode
    if (JSON_ObjectGetValue(parent_obj, "target_change_mode") == nullptr) {
        const JSON_VALUE *const value =
            JSON_ObjectGetValue(parent_obj, "enable_target_change");
        g_Config.gameplay.target_change_mode = JSON_ValueIsTrue(value)
            ? TARGET_CHANGE_MODE_ENHANCED
            : TARGET_CHANGE_MODE_OFF;
    }

    // TRX ..1.10: breeze on/off changed to mode
    if (JSON_ObjectGetValue(parent_obj, "breeze_mode") == nullptr) {
        const JSON_VALUE *const value =
            JSON_ObjectGetValue(parent_obj, "enable_breeze");
        if (JSON_ValueIsTrue(value)) {
            g_Config.visuals.breeze_mode =
                g_TRVersion <= 2 ? BREEZE_MODE_TR2 : BREEZE_MODE_TR3;
        } else {
            g_Config.visuals.breeze_mode = BREEZE_MODE_OFF;
        }
    }

    // TRX ..1.10: save crystals on/off changed to mode. Only an explicit opt-in
    // carries over; the per-game default covers the rest, so TR3 keeps healing.
    if (JSON_ObjectGetValue(parent_obj, "save_crystal_mode") == nullptr) {
        const JSON_VALUE *const value =
            JSON_ObjectGetValue(parent_obj, "enable_save_crystals");
        if (JSON_ValueIsTrue(value)) {
            g_Config.gameplay.save_crystal_mode = SAVE_CRYSTAL_SAVE;
        }
    }

    if (g_Config.config_version >= 0
        && g_Config.config_version < M_CONFIG_VERSION_CURRENT) {
        g_Config.config_version = M_CONFIG_VERSION_CURRENT;
    }
}

void Config_LoadFromJSON(JSON_OBJECT *root_obj)
{
    ConfigFile_LoadOptions(root_obj, Config_GetOptionMap());
    if (Gym_TrackManager_HasStats(GYM_TRACK_ASSAULT)) {
        ConfigFile_LoadGymTrackStats(
            root_obj, "assault_stats", &g_Config.profile.assault_stats);
    }
    if (Gym_TrackManager_HasStats(GYM_TRACK_QUAD)) {
        ConfigFile_LoadGymTrackStats(
            root_obj, "racetrack_stats", &g_Config.profile.racetrack_stats);
    }
    M_LoadInputConfig(root_obj);
    M_LoadLegacyOptions(root_obj);
}

void Config_DumpToJSON(JSON_OBJECT *root_obj)
{
    ConfigFile_DumpOptions(root_obj, Config_GetOptionMap());
    if (Gym_TrackManager_HasStats(GYM_TRACK_ASSAULT)) {
        ConfigFile_DumpGymTrackStats(
            root_obj, "assault_stats", &g_Config.profile.assault_stats);
    }
    if (Gym_TrackManager_HasStats(GYM_TRACK_QUAD)) {
        ConfigFile_DumpGymTrackStats(
            root_obj, "racetrack_stats", &g_Config.profile.racetrack_stats);
    }
    M_DumpInputConfig(root_obj);
}

void Config_Sanitize(void)
{
    if (g_Config.rendering.aspect_mode != ASPECT_MODE_ANY
        && g_Config.rendering.aspect_mode != ASPECT_MODE_16_9
        && g_Config.rendering.aspect_mode != ASPECT_MODE_16_10) {
        g_Config.rendering.aspect_mode = ASPECT_MODE_4_3;
    }
    CLAMP(g_Config.audio.master_volume, 0.0f, 1.0f);
    CLAMP(g_Config.audio.sound_volume, 0.0f, 1.0f);
    CLAMP(g_Config.audio.music_volume, 0.0f, 1.0f);
    CLAMP(g_Config.audio.inventory_music_volume, 0.0f, 1.0f);
    CLAMP(g_Config.audio.underwater_music_volume, 0.0f, 1.0f);
    CLAMP(g_Config.audio.ambient_volume, 0.0f, 1.0f);
    CLAMP(g_Config.audio.inventory_ambient_volume, 0.0f, 1.0f);
    CLAMP(g_Config.audio.underwater_ambient_volume, 0.0f, 1.0f);
    CLAMP(g_Config.audio.cutscene_volume, 0.0f, 1.0f);
    CLAMP(g_Config.audio.fmv_volume, 0.0f, 1.0f);
    CLAMP(g_Config.input.keyboard_layout, 0, INPUT_LAYOUT_NUMBER_OF - 1);
    CLAMP(g_Config.input.controller_layout, 0, INPUT_LAYOUT_NUMBER_OF - 1);
    CLAMP(g_Config.input.touch_layout, 0, INPUT_LAYOUT_NUMBER_OF - 1);
    CLAMP(
        g_Config.gameplay.turbo_speed, CLOCK_TURBO_SPEED_MIN,
        CLOCK_TURBO_SPEED_MAX);
    CLAMP(g_Config.gameplay.start_lara_hitpoints, 1, LARA_MAX_HITPOINTS);
    CLAMP(g_Config.gameplay.camera_speed, 1, 10);
    CLAMP(g_Config.gameplay.idle_pose_timeout, 0, 1200);
    CLAMP(g_Config.rendering.wireframe_width, 1.0, 100.0);
    CLAMP(g_Config.rendering.upscaling_factor, 1, 10);
    CLAMP(g_Config.rendering.borders, 0.0, 0.45);
    CLAMP(g_Config.ui.bar_scale, 0.5, 2.0);
    CLAMP(g_Config.ui.text_scale, 0.5, 2.0);
    CLAMP(g_Config.ui.pickup_scale, 0.5, 2.0);
    CLAMP(g_Config.visuals.fog_start, 1, 100);
    CLAMP(g_Config.visuals.fog_end, 1, 100);
    CLAMP(g_Config.visuals.fov, 30, 150);
    CLAMPL(g_Config.gameplay.maximum_save_slots, 0);
    CLAMPL(g_Config.gameplay.maximum_quick_save_slots, 0);
    CLAMP(g_Config.visuals.shadow_type, 0, SHADOW_TYPE_NUMBER_OF - 1);
    CLAMP(g_Config.visuals.blood_effects, 0, BLOOD_EFFECTS_NUMBER_OF - 1);
    CLAMP(g_Config.gameplay.loading_screens, 0, LOADING_SCREENS_NEW_GAMES);

    if (g_Config.rendering.fps != 30 && g_Config.rendering.fps != 60) {
        g_Config.rendering.fps = 30;
    }

    CLAMP(
        g_Config.visuals.game_brightness, CONFIG_MIN_BRIGHTNESS,
        CONFIG_MAX_BRIGHTNESS);
    CLAMP(
        g_Config.visuals.ui_brightness, CONFIG_MIN_BRIGHTNESS,
        CONFIG_MAX_BRIGHTNESS);
    CLAMP(g_Config.visuals.gamma, CONFIG_MIN_GAMMA, CONFIG_MAX_GAMMA);
    CLAMPL(g_Config.rendering.anisotropy_filter, 1.0);
}
