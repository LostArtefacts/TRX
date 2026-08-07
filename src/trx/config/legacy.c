#include <trx/config/legacy.h>

#include <trx/config/common.h>
#include <trx/config/vars.h>
#include <trx/version.h>

#include <string.h>

// What last wrote the file, for a migration that has to tell one build from
// another rather than one key from another. Nothing keys on it today.
#define M_CONFIG_VERSION_CURRENT 1

// Reads one key an older release wrote. The value is what the file holds for
// it, or nullptr where the file does not have it at all.
typedef void (*M_APPLY)(const JSON_VALUE *value);

typedef struct {
    // The key that release wrote.
    const char *old_key;
    // The option that replaced it. While the file has this one, that is where
    // the player's answer went, and reading the old key over it would put back
    // what they changed away from.
    const char *new_key;
    M_APPLY apply;
} M_MIGRATION;

static void M_ApplyGameModes(const JSON_VALUE *const value)
{
    CONFIG_SET(
        g_Config.gameplay.game_modes_policy,
        JSON_ValueIsTrue(value) ? GAME_MODES_POLICY_ALWAYS
                                : GAME_MODES_POLICY_NEVER);
}

static void M_ApplyTargetChange(const JSON_VALUE *const value)
{
    CONFIG_SET(
        g_Config.gameplay.target_change_mode,
        JSON_ValueIsTrue(value) ? TARGET_CHANGE_MODE_ENHANCED
                                : TARGET_CHANGE_MODE_OFF);
}

static void M_ApplyBreeze(const JSON_VALUE *const value)
{
    if (JSON_ValueIsTrue(value)) {
        CONFIG_SET(
            g_Config.visuals.breeze_mode,
            g_TRVersion <= 2 ? BREEZE_MODE_TR2 : BREEZE_MODE_TR3);
    } else {
        CONFIG_SET(g_Config.visuals.breeze_mode, BREEZE_MODE_OFF);
    }
}

static void M_ApplySaveCrystals(const JSON_VALUE *const value)
{
    // Only an explicit opt-in carries over; the per-game default covers the
    // rest, so TR3 keeps healing.
    if (JSON_ValueIsTrue(value)) {
        CONFIG_SET(g_Config.gameplay.save_crystal_mode, SAVE_CRYSTAL_SAVE);
    }
}

static void M_ApplyTurns(const JSON_VALUE *const value)
{
    if (JSON_ValueIsFalse(value)) {
        CONFIG_SET(g_Config.gameplay.enable_alternative_turns, false);
    }
}

// Every key no build writes any more, and what became of it.
static const M_MIGRATION m_Migrations[] = {
    // TRX ..1.9: game modes changed to policy.
    { "enable_game_modes", "game_modes_policy", M_ApplyGameModes },
    // TRX ..1.10: target change on/off changed to mode.
    { "enable_target_change", "target_change_mode", M_ApplyTargetChange },
    // TRX ..1.10: breeze on/off changed to mode.
    { "enable_breeze", "breeze_mode", M_ApplyBreeze },
    // TRX ..1.10: save crystals on/off changed to mode.
    { "enable_save_crystals", "save_crystal_mode", M_ApplySaveCrystals },
    // TRX ..1.10: neutral twists on/off changed to cover multiple animations.
    { "enable_neutral_twists", "enable_alternative_turns", M_ApplyTurns },
    {}, // sentinel
};

bool ConfigLegacy_IsKey(const char *const name)
{
    if (name == nullptr) {
        return false;
    }
    for (const M_MIGRATION *m = m_Migrations; m->old_key != nullptr; m++) {
        if (strcmp(m->old_key, name) == 0) {
            return true;
        }
    }
    return false;
}

void ConfigLegacy_Load(JSON_OBJECT *const parent_obj)
{
    for (const M_MIGRATION *m = m_Migrations; m->old_key != nullptr; m++) {
        if (JSON_ObjectGetValue(parent_obj, m->new_key) == nullptr) {
            m->apply(JSON_ObjectGetValue(parent_obj, m->old_key));
        }
    }

    if (g_Config.config_version >= 0
        && g_Config.config_version < M_CONFIG_VERSION_CURRENT) {
        CONFIG_SET(g_Config.config_version, M_CONFIG_VERSION_CURRENT);
    }
}
