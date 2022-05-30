#pragma once

#include <stdbool.h>

void Shell_Init(const char *gameflow_path);
void Shell_Shutdown(void);
void Shell_Main(void);
void Shell_ExitSystem(const char *message);
void Shell_ExitSystemFmt(const char *fmt, ...);
void Shell_Wait(int nticks);
bool Shell_MakeScreenshot(void);
