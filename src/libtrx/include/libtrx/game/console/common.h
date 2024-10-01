#pragma once

#include "../types.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    CR_SUCCESS,
    CR_FAILURE,
    CR_UNAVAILABLE,
    CR_BAD_INVOCATION,
} COMMAND_RESULT;

typedef struct __PACKING {
    const struct __PACKING CONSOLE_COMMAND *cmd;
    const char *prefix;
    const char *args;
} COMMAND_CONTEXT;

typedef struct __PACKING CONSOLE_COMMAND {
    const char *prefix;
    COMMAND_RESULT (*proc)(const COMMAND_CONTEXT *ctx);
} CONSOLE_COMMAND;

void Console_Init(void);
void Console_Shutdown(void);

void Console_Open(void);
void Console_Close(void);
bool Console_IsOpened(void);

void Console_ScrollLogs(void);
int32_t Console_GetVisibleLogCount(void);
int32_t Console_GetMaxLogCount(void);

void Console_Log(const char *fmt, ...);
COMMAND_RESULT Console_Eval(const char *cmdline);

void Console_Draw(void);
extern void Console_DrawBackdrop(void);
