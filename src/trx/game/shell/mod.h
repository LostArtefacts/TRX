#pragma once

#include <stdint.h>

typedef enum {
    MOD_BASE_GAME,
    MOD_EXPANSION_PACK,
    MOD_MISC,
    MOD_DIRECT_LEVEL,
} SHELL_MOD_TYPE;

typedef struct {
    const char *name;
    SHELL_MOD_TYPE mod_type;
    int32_t engine_version;
    const char *base_mod;
    bool is_available;
} SHELL_MOD;

void Shell_ScanAvailableMods(void);
int32_t Shell_GetModCount(void);
const SHELL_MOD *Shell_GetMod(int32_t index);
const SHELL_MOD *Shell_GetModByName(const char *name);
const SHELL_MOD *Shell_GetModByType(
    SHELL_MOD_TYPE mod_type, int32_t engine_version);
bool Shell_CanSwitchToMod(const SHELL_MOD *mod);
bool Shell_IsCurrentMod(const char *name);

const char *Shell_GetCommonStringsPath(void);
const char *Shell_GetBaseGameStringsPath(const SHELL_MOD *mod);
const char *Shell_GetGameStringsPath(const SHELL_MOD *mod);
const char *Shell_GetGameFlowPath(const SHELL_MOD *mod);
