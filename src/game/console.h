#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <stdbool.h>

void Console_Init(void);
void Console_Shutdown(void);
void Console_Draw(void);

void Console_Open(void);
void Console_Close(void);
bool Console_IsOpened(void);

void Console_Confirm(void);

void Console_HandleKeyDown(const SDL_Event event);
void Console_HandleTextEdit(const SDL_Event event);
void Console_HandleTextInput(const SDL_Event event);

void Console_Log(const char *fmt, ...);
void Console_ScrollLogs(void);
