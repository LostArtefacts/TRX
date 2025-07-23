#pragma once

#include "./args.h"

// TODO: inline these once we get common shell code
void Shell_InstallConfig(void);
void Shell_CommonInit(const SHELL_ARGS *args);
void Shell_ShutdownCommonModules(void);
