#include <trx/config/priv.h>

#include <trx/config/common.h>
#include <trx/config/file.h>
#include <trx/config/legacy.h>
#include <trx/config/section.h>
#include <trx/config/vars.h>
#include <trx/core/utils.h>
#include <trx/game/clock.h>
#include <trx/game/input.h>
#include <trx/game/lara/const.h>

void Config_LoadFromJSON(JSON_OBJECT *root_obj)
{
    ConfigFile_LoadOptions(root_obj, Config_GetOptionMap());
    for (const CONFIG_SECTION *const *s = Config_Section_GetAll();
         *s != nullptr; s++) {
        (*s)->load(JSON_ObjectGetObject(root_obj, (*s)->key));
    }
    ConfigLegacy_Load(root_obj);
}

void Config_DumpToJSON(JSON_OBJECT *root_obj)
{
    ConfigFile_DumpOptions(root_obj, Config_GetOptionMap());
    for (const CONFIG_SECTION *const *s = Config_Section_GetAll();
         *s != nullptr; s++) {
        JSON_OBJECT *const obj = JSON_ObjectNew();
        (*s)->save(obj);
        // A section with nothing to say leaves no key behind, so what another
        // game wrote under it is carried over rather than replaced by a husk.
        if (obj->length > 0) {
            JSON_ObjectAppendObject(root_obj, (*s)->key, obj);
        } else {
            JSON_ObjectFree(obj);
        }
    }
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
