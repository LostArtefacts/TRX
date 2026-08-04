#include <trx/config/legacy.h>

#include <trx/config/vars.h>
#include <trx/version.h>

// What last wrote the file, for a migration that has to tell one build from
// another rather than one key from another. Nothing keys on it today.
#define M_CONFIG_VERSION_CURRENT 1

void ConfigLegacy_Load(JSON_OBJECT *const parent_obj)
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
