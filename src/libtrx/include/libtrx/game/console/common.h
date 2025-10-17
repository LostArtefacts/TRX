#pragma once

#include "../../log.h"
#include "./types.h"

#include <stdint.h>

#define Console_Log(...)                                                       \
    Console_LogEx(__VA_ARGS__);                                                \
    LOG_INFO(__VA_ARGS__)
#define Console_LogWarning(...)                                                \
    Console_LogEx(__VA_ARGS__);                                                \
    LOG_WARNING(__VA_ARGS__)
#define Console_LogError(...)                                                  \
    Console_LogEx(__VA_ARGS__);                                                \
    LOG_ERROR(__VA_ARGS__)

void Console_Init(void);
void Console_Shutdown(void);

void Console_Open(void);
void Console_Close(void);
bool Console_IsOpened(void);

void Console_LogEx(const char *fmt, ...);
void Console_Clear(void);
COMMAND_RESULT Console_Eval(const char *cmdline);

// Controls whether console commands emit log events to the UI console
void Console_SetVerbose(bool verbose);
bool Console_IsVerbose(void);

void Console_Control(void);
void Console_Draw(void);
