#include <trx/game/shell/mod.h>

#include <trx/filesystem.h>
#include <trx/game/shell/common.h>
#include <trx/strings.h>

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
    { .name = "tr3-level",
      .mod_type = MOD_DIRECT_LEVEL,
      .engine_version = 3,
      .base_mod = "tr3" },
    { .name = nullptr }, // sentinel
};

void Shell_ScanAvailableMods(void)
{
    for (int32_t i = 0; m_KnownMods[i].name != nullptr; i++) {
        SHELL_MOD *const mod = &m_KnownMods[i];
        mod->is_available = File_Exists(Shell_GetGameFlowPath(&m_KnownMods[i]));
    }
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

const SHELL_MOD *Shell_GetModByType(
    const SHELL_MOD_TYPE mod_type, const int32_t engine_version)
{
    for (int32_t i = 0; m_KnownMods[i].name != nullptr; i++) {
        const SHELL_MOD *const mod = &m_KnownMods[i];
        if (mod->is_available && mod->mod_type == mod_type
            && (engine_version <= 0 || mod->engine_version == engine_version)) {
            return &m_KnownMods[i];
        }
    }
    return nullptr;
}

const char *Shell_GetCommonStringsPath(void)
{
    return String_FormatStatic("%s/base_strings.json5", Shell_GetConfigDir());
}

const char *Shell_GetBaseGameStringsPath(const SHELL_MOD *const mod)
{
    return String_FormatStatic(
        "%s/%s/strings.json5", Shell_GetConfigDir(), mod->base_mod);
}

const char *Shell_GetGameStringsPath(const SHELL_MOD *const mod)
{
    return String_FormatStatic(
        "%s/%s/strings.json5", Shell_GetConfigDir(), mod->name);
}

const char *Shell_GetGameFlowPath(const SHELL_MOD *const mod)
{
    return String_FormatStatic(
        "%s/%s/gameflow.json5", Shell_GetConfigDir(), mod->name);
}
