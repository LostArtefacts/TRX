#pragma once

#include <trx/core/log.h>
#include <trx/core/vector.h>
#include <trx/game/console/types.h>

#include <stddef.h>
#include <stdint.h>

#define Console_Log(level, ...)                                                \
    Console_LogImpl(level, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define Console_Info(...) Console_Log(LOG_LEVEL_INFO, __VA_ARGS__)
#define Console_Warn(...) Console_Log(LOG_LEVEL_WARNING, __VA_ARGS__)
#define Console_Error(...) Console_Log(LOG_LEVEL_ERROR, __VA_ARGS__)

void Console_Open(void);
void Console_Close(void);
bool Console_IsOpened(void);

void Console_LogImpl(
    LOG_LEVEL level, const char *file, int line, const char *func,
    const char *fmt, ...);
void Console_Clear(void);
COMMAND_RESULT Console_Eval(const char *cmdline);

// Controls whether console commands emit log events to the UI console
void Console_SetVerbose(bool verbose);
bool Console_IsVerbose(void);

void Console_Control(void);
void Console_Draw(void);
