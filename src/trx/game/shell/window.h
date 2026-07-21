#pragma once

#include <SDL2/SDL_video.h>

// The SDL window the game renders into. Kept apart from shell/common.h so that
// header stays free of SDL, and the modules that only want the shell's state
// need not pull the windowing library in.
SDL_Window *Shell_GetWindow(void);
