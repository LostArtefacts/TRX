#pragma once

#include <stdbool.h>

void Shell_Main();
void Shell_ExitSystem(const char *message);
void Shell_ExitSystemFmt(const char *fmt, ...);
void Shell_Wait(int nticks);
bool Shell_MakeScreenshot();
