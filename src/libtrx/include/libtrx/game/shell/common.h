#pragma once

#include "../../config/types.h"
#include "../../event_manager.h"
#include "./args.h"

#include <SDL2/SDL_events.h>
#include <stdint.h>

typedef struct {
    int32_t w;
    int32_t h;
} SHELL_SIZE;

extern void Shell_Shutdown(void);

extern SDL_Window *Shell_GetWindow(void);
const char *Shell_GetConfigDir(void);

extern int32_t Shell_Main(const SHELL_ARGS *args);
void Shell_Terminate(int32_t exit_code);
void Shell_ExitSystem(const char *message);
void Shell_ExitSystemFmt(const char *fmt, ...);

void Shell_ScheduleExit(void);
bool Shell_IsExiting(void);
const SHELL_ARGS *Shell_GetArgs(void);

bool Shell_IsFullscreen(void);
SHELL_SIZE Shell_GetDefaultSize(void);
SHELL_SIZE Shell_GetWindowSize(void);
SHELL_SIZE Shell_GetCurrentSize(void);
SHELL_SIZE Shell_GetCurrentDisplaySize(void);
