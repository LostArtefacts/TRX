#include <trx/game/shell/mod.h>

#include <trx/core/filesystem.h>
#include <trx/core/strings.h>
#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/game_flow/reader.h>
#include <trx/game/shell/common.h>
#include <trx/game/shell/paths.h>
#include <trx/game/shell/state.h>
#include <trx/version.h>

#include <string.h>

static SHELL_MOD m_KnownMods[] = {
    { .name = "tr1",
      .mod_type = MOD_BASE_GAME,
      .engine_version = 1,
      .base_mod = nullptr },
    { .name = "tr1-ub",
      .mod_type = MOD_EXPANSION_PACK,
      .engine_version = 1,
      .base_mod = "tr1" },
    { .name = "tr1-demo-pc",
      .mod_type = MOD_MISC,
      .engine_version = 1,
      .base_mod = "tr1" },
    { .name = "tr1-level",
      .mod_type = MOD_DIRECT_LEVEL,
      .engine_version = 1,
      .base_mod = "tr1" },
    { .name = "tr2",
      .mod_type = MOD_BASE_GAME,
      .engine_version = 2,
      .base_mod = nullptr },
    { .name = "tr2-gm",
      .mod_type = MOD_EXPANSION_PACK,
      .engine_version = 2,
      .base_mod = "tr2" },
    { .name = "tr2-level",
      .mod_type = MOD_DIRECT_LEVEL,
      .engine_version = 2,
      .base_mod = "tr2" },
    { .name = "tr3",
      .mod_type = MOD_BASE_GAME,
      .engine_version = 3,
      .base_mod = nullptr },
    { .name = "tr3-la",
      .mod_type = MOD_EXPANSION_PACK,
      .engine_version = 3,
      .base_mod = "tr3" },
    { .name = "tr3-level",
      .mod_type = MOD_DIRECT_LEVEL,
      .engine_version = 3,
      .base_mod = "tr3" },
    { .name = nullptr }, // sentinel
};

static void M_ValidateNoMixedModLayouts(void)
{
    const char *const games_dir = TRXPath_Get(TRX_PATH_GAMES_DIR);
    const char *const config_dir = TRXPath_Get(TRX_PATH_CONFIG_DIR);
    if (games_dir == nullptr || config_dir == nullptr
        || strcmp(games_dir, config_dir) == 0) {
        return;
    }

    for (int32_t i = 0; m_KnownMods[i].name != nullptr; i++) {
        const SHELL_MOD *const mod = &m_KnownMods[i];
        const char *const legacy_gameflow =
            String_FormatStatic("%s/%s/gameflow.json5", config_dir, mod->name);
        if (File_Exists(legacy_gameflow)) {
            Shell_ExitSystemFmt(
                "Mixed mod layout detected: found legacy mod data at '%s' "
                "while '%s' is used for mods. Move '%s' to '%s/%s/'.",
                legacy_gameflow, games_dir, mod->name, games_dir, mod->name);
        }
    }
}

static const char *M_GetModStringsPath(const char *const mod_id)
{
    ASSERT(mod_id != nullptr);
    return TRXPath_Join(
        TRX_PATH_GAMES_DIR, String_FormatStatic("%s/strings.json5", mod_id));
}

void Shell_ScanAvailableMods(void)
{
    M_ValidateNoMixedModLayouts();

    for (int32_t i = 0; m_KnownMods[i].name != nullptr; i++) {
        SHELL_MOD *const mod = &m_KnownMods[i];
        mod->is_available =
            TRXPath_Exists(TRX_DYNAMIC_PATH_GAMEFLOW_FILE, mod->name);
        mod->is_valid = mod->is_available;
    }
}

void Shell_ValidateMods(void)
{
    const int32_t original_tr_version = g_TRVersion;

    for (int32_t i = 0; m_KnownMods[i].name != nullptr; i++) {
        SHELL_MOD *const mod = &m_KnownMods[i];
        if (!mod->is_available) {
            mod->is_valid = false;
            continue;
        }

        const SHELL_ARGS args = {
            .engine_version = mod->engine_version,
            .mod = mod,
            .level_to_select = -1,
            .save_to_load = -1,
        };
        g_TRVersion = mod->engine_version;
        TRXPath_Init(&args);

        mod->is_valid = GF_ValidateMod(mod->name, Shell_GetGameFlowPath(mod));
    }

    g_TRVersion = original_tr_version;
}

int32_t Shell_GetModCount(void)
{
    return ARRAY_SIZE(m_KnownMods) - 1;
}

const SHELL_MOD *Shell_GetMod(const int32_t index)
{
    if (index < 0 || index >= Shell_GetModCount()) {
        return nullptr;
    }
    return &m_KnownMods[index];
}

const SHELL_MOD *Shell_GetModByName(const char *const name)
{
    for (int32_t i = 0; m_KnownMods[i].name != nullptr; i++) {
        const SHELL_MOD *const mod = &m_KnownMods[i];
        if (mod->is_available && strcmp(mod->name, name) == 0) {
            return &m_KnownMods[i];
        }
    }
    return nullptr;
}

static bool M_MatchesEngineVersion(
    const SHELL_MOD *const mod, const int32_t engine_version)
{
    return engine_version == 0 || mod->engine_version == engine_version;
}

static const SHELL_MOD *M_GetFirstAvailableMod(const int32_t engine_version)
{
    for (int32_t i = 0; m_KnownMods[i].name != nullptr; i++) {
        const SHELL_MOD *const mod = &m_KnownMods[i];
        if (!Shell_CanSwitchToMod(mod)
            || !M_MatchesEngineVersion(mod, engine_version)) {
            continue;
        }
        return mod;
    }
    return nullptr;
}

const SHELL_MOD *Shell_SelectStartupMod(const int32_t engine_version)
{
    const char *const last_played_mod = ShellState_GetLastPlayedMod();
    if (last_played_mod != nullptr) {
        const SHELL_MOD *const mod = Shell_GetModByName(last_played_mod);
        if (mod != nullptr && Shell_CanSwitchToMod(mod)
            && M_MatchesEngineVersion(mod, engine_version)) {
            return mod;
        }
    }

    return M_GetFirstAvailableMod(engine_version);
}

const SHELL_MOD *Shell_GetModByType(
    const SHELL_MOD_TYPE mod_type, const int32_t engine_version)
{
    const SHELL_MOD *found = nullptr;

    for (int32_t i = 0; m_KnownMods[i].name != nullptr; i++) {
        const SHELL_MOD *const mod = &m_KnownMods[i];
        if (!mod->is_available || mod->mod_type != mod_type
            || (engine_version > 0 && mod->engine_version != engine_version)) {
            continue;
        }

        // match
        if (engine_version == 0) {
            if (found) {
                // more than one mod matches this engine version, abort
                return nullptr;
            }
            found = mod;
        } else {
            // exact version match
            return mod;
        }
    }

    return found;
}

bool Shell_CanSwitchToMod(const SHELL_MOD *const mod)
{
    return mod != nullptr && mod->is_available && mod->is_valid
        && mod->mod_type != MOD_DIRECT_LEVEL;
}

bool Shell_IsCurrentMod(const char *const name)
{
    if (name == nullptr) {
        return false;
    }

    const SHELL_ARGS *const args = Shell_GetArgs();
    if (args == nullptr || args->mod == nullptr || args->mod->name == nullptr) {
        return false;
    }

    return strcmp(args->mod->name, name) == 0;
}

const char *Shell_GetCommonStringsPath(void)
{
    return TRXPath_TryResolve(
        TRX_DYNAMIC_PATH_COMMON_CONFIG, "base_strings.json5");
}

const char *Shell_GetBaseGameStringsPath(const SHELL_MOD *const mod)
{
    const char *const base_mod =
        mod->base_mod != nullptr ? mod->base_mod : mod->name;
    return M_GetModStringsPath(base_mod);
}

const char *Shell_GetGameStringsPath(const SHELL_MOD *const mod)
{
    return M_GetModStringsPath(mod->name);
}

const char *Shell_GetGameFlowPath(const SHELL_MOD *const mod)
{
    return TRXPath_Resolve(TRX_DYNAMIC_PATH_GAMEFLOW_FILE, mod->name);
}
