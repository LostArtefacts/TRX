#pragma once

#include <stdbool.h>

void Shell_Main();
void Shell_ExitSystem(const char *message);
void Shell_ExitSystemFmt(const char *fmt, ...);
void Shell_Wait(int nticks);
void Shell_ValidateLevelTitle(char *str);
void Shell_GetScreenshotName(char *str, const char *ext);
bool Shell_MakeScreenshot();
