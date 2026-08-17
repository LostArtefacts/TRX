#include <trx/game/shell/mod.h>

#include <trx/core/filesystem.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/game_flow/reader.h>
#include <trx/game/paths.h>
#include <trx/game/shell/common.h>
#include <trx/game/shell/state.h>
#include <trx/version.h>

#include <string.h>

typedef struct {
    GF_MOD_META meta;
    SHELL_MOD_TYPE mod_type;
} M_KNOWN_MOD;

typedef struct {
    char *mod_name;
    char *text;
} M_REJECTION;

static const M_KNOWN_MOD m_KnownModSeeds[] = {
    { .meta = { .name = "tr1", .engine = 1 }, .mod_type = MOD_BASE_GAME },
    { .meta = { .name = "tr1-ub", .engine = 1, .extends = "tr1" },
      .mod_type = MOD_EXPANSION_PACK },
    { .meta = { .name = "tr1-demo-pc", .engine = 1, .extends = "tr1" },
      .mod_type = MOD_MISC },
    { .meta = { .name = "tr1-level", .engine = 1, .extends = "tr1" },
      .mod_type = MOD_DIRECT_LEVEL },
    { .meta = { .name = "tr2", .engine = 2 }, .mod_type = MOD_BASE_GAME },
    { .meta = { .name = "tr2-gm", .engine = 2, .extends = "tr2" },
      .mod_type = MOD_EXPANSION_PACK },
    { .meta = { .name = "tr2-level", .engine = 2, .extends = "tr2" },
      .mod_type = MOD_DIRECT_LEVEL },
    { .meta = { .name = "tr3", .engine = 3 }, .mod_type = MOD_BASE_GAME },
    { .meta = { .name = "tr3-la", .engine = 3, .extends = "tr3" },
      .mod_type = MOD_EXPANSION_PACK },
    { .meta = { .name = "tr3-level", .engine = 3, .extends = "tr3" },
      .mod_type = MOD_DIRECT_LEVEL },
    { .meta = { .name = "tr4", .engine = 4 }, .mod_type = MOD_BASE_GAME },
    { .meta = { .name = "tr4-level", .engine = 4, .extends = "tr4" },
      .mod_type = MOD_DIRECT_LEVEL },
};

static VECTOR *m_Mods = nullptr;

// Why a game was passed over, kept so that a startup can say what went wrong
// with the game the player asked for, or with every one of them where there is
// nothing left to play.
static VECTOR *m_Rejections = nullptr;
static char *m_RejectionSummary = nullptr;

static void M_Reject(const char *const mod_name, const char *const reason)
{
    if (m_Rejections == nullptr) {
        m_Rejections = Vector_Create(sizeof(M_REJECTION));
    }
    const M_REJECTION rejection = {
        .mod_name = Memory_DupStr(mod_name),
        .text = Memory_DupStr(reason),
    };
    Vector_Add(m_Rejections, &rejection);
}

static SHELL_MOD *M_FindMod(const char *const name)
{
    if (m_Mods == nullptr || name == nullptr) {
        return nullptr;
    }
    for (int32_t i = 0; i < m_Mods->count; i++) {
        SHELL_MOD *const mod = Vector_Get(m_Mods, i);
        if (strcmp(mod->name, name) == 0) {
            return mod;
        }
    }
    return nullptr;
}

static void M_AddMod(
    const char *const name, const char *const title,
    const SHELL_MOD_TYPE mod_type, const int32_t engine_version,
    const char *const base_mod)
{
    const SHELL_MOD mod = {
        .name = Memory_DupStr(name),
        .title = title != nullptr ? Memory_DupStr(title) : nullptr,
        .mod_type = mod_type,
        .engine_version = engine_version,
        .base_mod = base_mod != nullptr ? Memory_DupStr(base_mod) : nullptr,
        .is_available = false,
        .is_valid = false,
    };
    Vector_Add(m_Mods, &mod);
}

static void M_SeedKnownMods(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(m_KnownModSeeds); i++) {
        const M_KNOWN_MOD *const seed = &m_KnownModSeeds[i];
        M_AddMod(
            seed->meta.name, nullptr, seed->mod_type, seed->meta.engine,
            seed->meta.extends);
    }
}

static void M_ScanForCustomMods(void)
{
    const char *const games_dir = GamePath_Get(GAME_PATH_GAMES_DIR);
    if (games_dir == nullptr) {
        return;
    }

    void *const dir = FS_OpenDirectory(games_dir);
    if (dir == nullptr) {
        return;
    }

    const char *entry;
    while ((entry = FS_ReadDirectory(dir)) != nullptr) {
        if (strcmp(entry, ".") == 0 || strcmp(entry, "..") == 0) {
            continue;
        }

        if (M_FindMod(entry) != nullptr) {
            continue;
        }

        const char *const gameflow_path =
            String_FormatStatic("%s/%s/gameflow.json5", games_dir, entry);

        GF_MOD_META meta = {};
        char *error = nullptr;
        if (!GF_ReadModMeta(gameflow_path, &meta, &error)) {
            LOG_WARNING("Failed to read mod metadata from '%s'", gameflow_path);
            M_Reject(
                entry,
                error != nullptr ? error
                                 : String_FormatStatic(
                                       "%s could not be read", gameflow_path));
            Memory_FreePointer(&error);
            continue;
        }
        Memory_FreePointer(&error);

        if (meta.engine <= 0) {
            LOG_WARNING(
                "Custom mod '%s' has no 'engine' field in gameflow; skipping",
                entry);
            M_Reject(
                entry,
                String_FormatStatic("%s has no 'engine' field", gameflow_path));
            Memory_FreePointer(&meta.name);
            Memory_FreePointer(&meta.extends);
            continue;
        }

        M_AddMod(entry, meta.name, MOD_CUSTOM, meta.engine, meta.extends);
        Memory_FreePointer(&meta.name);
        Memory_FreePointer(&meta.extends);
    }

    FS_CloseDirectory(dir);
}

static void M_ReadModMetaForKnownMods(void)
{
    for (int32_t i = 0; i < m_Mods->count; i++) {
        SHELL_MOD *const mod = Vector_Get(m_Mods, i);
        if (!mod->is_available || mod->mod_type == MOD_CUSTOM) {
            continue;
        }

        const char *const gameflow_path =
            GamePath_PeekResolve(GAME_DYNAMIC_PATH_GAMEFLOW_FILE, mod->name);
        if (gameflow_path == nullptr) {
            continue;
        }

        GF_MOD_META meta = {};
        char *error = nullptr;
        if (!GF_ReadModMeta(gameflow_path, &meta, &error)) {
            M_Reject(
                mod->name,
                error != nullptr ? error
                                 : String_FormatStatic(
                                       "%s could not be read", gameflow_path));
            Memory_FreePointer(&error);
            continue;
        }
        Memory_FreePointer(&error);

        if (meta.name != nullptr) {
            Memory_FreePointer(&mod->title);
            mod->title = meta.name;
            meta.name = nullptr;
        }

        if (meta.engine > 0) {
            mod->engine_version = meta.engine;
        }

        if (meta.extends != nullptr) {
            Memory_FreePointer(&mod->base_mod);
            mod->base_mod = meta.extends;
            meta.extends = nullptr;
        }

        Memory_FreePointer(&meta.name);
        Memory_FreePointer(&meta.extends);
    }
}

static void M_ValidateEngineVersions(void)
{
    for (int32_t i = 0; i < m_Mods->count; i++) {
        SHELL_MOD *const mod = Vector_Get(m_Mods, i);
        if (mod->engine_version <= 0 && mod->is_available) {
            LOG_WARNING(
                "Mod '%s' has no valid engine version; disabling", mod->name);
            mod->is_available = false;
            mod->is_valid = false;
        }
    }
}

static RESULT M_ValidateNoMixedModLayouts(void)
{
    const char *const games_dir = GamePath_Get(GAME_PATH_GAMES_DIR);
    const char *const config_dir = GamePath_Get(GAME_PATH_CONFIG_DIR);
    if (games_dir == nullptr || config_dir == nullptr
        || strcmp(games_dir, config_dir) == 0) {
        return OK;
    }

    for (int32_t i = 0; i < m_Mods->count; i++) {
        const SHELL_MOD *const mod = Vector_Get(m_Mods, i);
        if (mod->mod_type == MOD_CUSTOM) {
            continue;
        }
        const char *const legacy_gameflow =
            String_FormatStatic("%s/%s/gameflow.json5", config_dir, mod->name);
        if (FS_Exists(legacy_gameflow)) {
            // The paths are long enough that the message box cannot fit them
            // on one line, and it does not wrap.
            return FAIL(
                "Mixed mod layout detected.\n"
                "\n"
                "Legacy mod data found at:\n"
                "    %s\n"
                "\n"
                "Mods are read from:\n"
                "    %s\n"
                "\n"
                "Move the '%s' directory to:\n"
                "    %s/%s/",
                legacy_gameflow, games_dir, mod->name, games_dir, mod->name);
        }
    }
    return OK;
}

static char *M_GetModStringsPath(const char *const mod_id)
{
    ASSERT(mod_id != nullptr);
    return GamePath_Join(
        GAME_PATH_GAMES_DIR, String_FormatStatic("%s/strings.json5", mod_id));
}

static void M_ClearRejections(void)
{
    Memory_FreePointer(&m_RejectionSummary);
    if (m_Rejections == nullptr) {
        return;
    }
    for (int32_t i = 0; i < m_Rejections->count; i++) {
        M_REJECTION *const rejection = Vector_Get(m_Rejections, i);
        Memory_FreePointer(&rejection->mod_name);
        Memory_FreePointer(&rejection->text);
    }
    Vector_Free(m_Rejections);
    m_Rejections = nullptr;
}

__attribute__((destructor)) static void M_Shutdown(void)
{
    M_ClearRejections();
    if (m_Mods == nullptr) {
        return;
    }
    for (int32_t i = 0; i < m_Mods->count; i++) {
        SHELL_MOD *const mod = Vector_Get(m_Mods, i);
        Memory_FreePointer(&mod->name);
        Memory_FreePointer(&mod->title);
        Memory_FreePointer(&mod->base_mod);
    }
    Vector_Free(m_Mods);
    m_Mods = nullptr;
}

static bool M_MatchesEngineVersion(
    const SHELL_MOD *const mod, const int32_t engine_version)
{
    return engine_version == 0 || mod->engine_version == engine_version;
}

static const SHELL_MOD *M_GetFirstAvailableMod(const int32_t engine_version)
{
    for (int32_t i = 0; i < m_Mods->count; i++) {
        const SHELL_MOD *const mod = Vector_Get(m_Mods, i);
        if (!Shell_CanSwitchToMod(mod)
            || !M_MatchesEngineVersion(mod, engine_version)) {
            continue;
        }
        return mod;
    }
    return nullptr;
}

const char *Shell_GetModRejection(const char *const mod_name)
{
    if (m_Rejections == nullptr || mod_name == nullptr) {
        return nullptr;
    }
    for (int32_t i = 0; i < m_Rejections->count; i++) {
        const M_REJECTION *const rejection = Vector_Get(m_Rejections, i);
        if (strcmp(rejection->mod_name, mod_name) == 0) {
            return rejection->text;
        }
    }
    return nullptr;
}

const char *Shell_GetModRejections(void)
{
    if (m_Rejections == nullptr || m_Rejections->count == 0) {
        return nullptr;
    }

    Memory_FreePointer(&m_RejectionSummary);
    m_RejectionSummary = Memory_DupStr("");
    for (int32_t i = 0; i < m_Rejections->count; i++) {
        const M_REJECTION *const rejection = Vector_Get(m_Rejections, i);
        char *const merged = String_Format(
            "%s%s%s: %s", m_RejectionSummary, i > 0 ? "\n" : "",
            rejection->mod_name, rejection->text);
        Memory_FreePointer(&m_RejectionSummary);
        m_RejectionSummary = merged;
    }
    return m_RejectionSummary;
}

RESULT Shell_ScanAvailableMods(void)
{
    if (m_Mods != nullptr) {
        M_Shutdown();
    }
    M_ClearRejections();
    m_Mods = Vector_Create(sizeof(SHELL_MOD));

    M_SeedKnownMods();

    // Mark availability for all seeded mods.
    for (int32_t i = 0; i < m_Mods->count; i++) {
        SHELL_MOD *const mod = Vector_Get(m_Mods, i);
        mod->is_available =
            GamePath_Exists(GAME_DYNAMIC_PATH_GAMEFLOW_FILE, mod->name);
        mod->is_valid = mod->is_available;
    }

    MUST(M_ValidateNoMixedModLayouts());
    M_ScanForCustomMods();

    // Mark availability for newly added custom mods.
    for (int32_t i = 0; i < m_Mods->count; i++) {
        SHELL_MOD *const mod = Vector_Get(m_Mods, i);
        if (mod->mod_type == MOD_CUSTOM) {
            mod->is_available =
                GamePath_Exists(GAME_DYNAMIC_PATH_GAMEFLOW_FILE, mod->name);
            mod->is_valid = mod->is_available;
        }
    }

    M_ReadModMetaForKnownMods();
    M_ValidateEngineVersions();
    return OK;
}

void Shell_ValidateMods(void)
{
    const int32_t original_tr_version = g_TRVersion;

    for (int32_t i = 0; i < m_Mods->count; i++) {
        SHELL_MOD *const mod = Vector_Get(m_Mods, i);
        if (!mod->is_available) {
            mod->is_valid = false;
            continue;
        }

        const SHELL_ARGS args = {
                .startup = {
                    .engine_version = mod->engine_version,
                    .mod = mod,
                    .level_request = { .num = -1 },
                    .save_to_load = -1,
                },
        };
        g_TRVersion = mod->engine_version;
        GamePath_Init(&args);

        const char *const gameflow_path = Shell_GetGameFlowPath(mod);
        char *error = nullptr;
        mod->is_valid = GF_ValidateMod(mod->name, gameflow_path, &error);
        if (!mod->is_valid && Shell_GetModRejection(mod->name) == nullptr) {
            M_Reject(
                mod->name,
                error != nullptr ? error
                                 : String_FormatStatic(
                                       "%s could not be read", gameflow_path));
        }
        Memory_FreePointer(&error);
    }

    g_TRVersion = original_tr_version;
}

int32_t Shell_GetModCount(void)
{
    return m_Mods != nullptr ? m_Mods->count : 0;
}

const SHELL_MOD *Shell_GetMod(const int32_t index)
{
    if (index < 0 || index >= Shell_GetModCount()) {
        return nullptr;
    }
    return Vector_Get(m_Mods, index);
}

const SHELL_MOD *Shell_GetModByName(const char *const name)
{
    if (m_Mods == nullptr || name == nullptr) {
        return nullptr;
    }
    for (int32_t i = 0; i < m_Mods->count; i++) {
        const SHELL_MOD *const mod = Vector_Get(m_Mods, i);
        if (mod->is_available && strcmp(mod->name, name) == 0) {
            return mod;
        }
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

    for (int32_t i = 0; i < Shell_GetModCount(); i++) {
        const SHELL_MOD *const mod = Vector_Get(m_Mods, i);
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
    if (args == nullptr || args->startup.mod == nullptr
        || args->startup.mod->name == nullptr) {
        return false;
    }

    return strcmp(args->startup.mod->name, name) == 0;
}

const char *Shell_GetCommonStringsPath(void)
{
    return GamePath_TryResolve(
        GAME_DYNAMIC_PATH_COMMON_CONFIG, "base_strings.json5");
}

char *Shell_GetBaseGameStringsPath(const SHELL_MOD *const mod)
{
    const char *const base_mod =
        mod->base_mod != nullptr ? mod->base_mod : mod->name;
    return M_GetModStringsPath(base_mod);
}

char *Shell_GetGameStringsPath(const SHELL_MOD *const mod)
{
    return M_GetModStringsPath(mod->name);
}

const char *Shell_GetGameFlowPath(const SHELL_MOD *const mod)
{
    return GamePath_PeekResolve(GAME_DYNAMIC_PATH_GAMEFLOW_FILE, mod->name);
}
