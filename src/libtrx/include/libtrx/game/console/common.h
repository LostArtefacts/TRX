#pragma once

#include "./types.h"

#include <stdint.h>

void Console_Init(void);
void Console_Shutdown(void);

void Console_Open(void);
void Console_Close(void);
bool Console_IsOpened(void);

void Console_Log(const char *fmt, ...);
COMMAND_RESULT Console_Eval(const char *cmdline);

void Console_Control(void);
void Console_Draw(void);
extern void Console_DrawBackdrop(void);
