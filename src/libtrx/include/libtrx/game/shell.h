#pragma once

#include <SDL2/SDL.h>
#include <stdint.h>

typedef struct {
    int32_t w;
    int32_t h;
} SHELL_SIZE;

extern void Shell_Shutdown(void);
extern SDL_Window *Shell_GetWindow(void);

extern bool Shell_ParseArgs(int32_t arg_count, const char **args);
void Shell_Setup(void);
extern int32_t Shell_Main(void);
void Shell_Terminate(int32_t exit_code);
void Shell_ExitSystem(const char *message);
void Shell_ExitSystemFmt(const char *fmt, ...);

void Shell_ScheduleExit(void);
bool Shell_IsExiting(void);

bool Shell_IsFullscreen(void);
SHELL_SIZE Shell_GetWindowSize(void);
SHELL_SIZE Shell_GetCurrentSize(void);
SHELL_SIZE Shell_GetCurrentDisplaySize(void);

extern const char *Shell_GetConfigPath(void);
extern const char *Shell_GetGameFlowPath(void);
extern void Shell_ProcessInput(void);
extern void Shell_ProcessEvents(void);
