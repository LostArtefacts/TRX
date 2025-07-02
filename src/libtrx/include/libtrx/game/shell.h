#pragma once

#include "../event_manager.h"

#include <SDL2/SDL.h>
#include <stdint.h>

typedef struct {
    int32_t w;
    int32_t h;
} SHELL_SIZE;

extern void Shell_Shutdown(void);
extern SDL_Window *Shell_GetWindow(void);

extern bool Shell_ParseArgs(int32_t arg_count, const char **args);
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

// TODO: inline these once we get common shell code
void Shell_LoadConfig(void);
void Shell_InitCommonModules(void);
void Shell_ShutdownCommonModules(void);
bool Shell_ProcessCommonEvent(const SDL_Event *event);

extern void Shell_HandleConfigChange(const EVENT *event, void *data);

extern const char *Shell_GetConfigPath(void);
extern const char *Shell_GetGameFlowPath(void);
extern void Shell_ProcessInput(void);
extern void Shell_ProcessEvents(void);
