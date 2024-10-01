#pragma once

#include "global/types.h"

BOOL __cdecl Shell_Main(void);
void __cdecl Shell_Cleanup(void);
void __cdecl Shell_ExitSystem(const char *message);
void __cdecl Shell_ExitSystemFmt(const char *fmt, ...);
