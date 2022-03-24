#pragma once

#include <stdbool.h>
#include <stdint.h>

void S_Shell_ShowFatalError(const char *message);

void S_Shell_Shutdown(void);
void S_Shell_SeedRandom(void);
void S_Shell_SpinMessageLoop(void);
bool S_Shell_GetCommandLine(int *arg_count, char ***args);
void *S_Shell_GetWindowHandle(void);
void S_Shell_TerminateGame(int exit_code);
void S_Shell_ToggleFullscreen(void);
int S_Shell_GetCurrentDisplayWidth(void);
int S_Shell_GetCurrentDisplayHeight(void);
