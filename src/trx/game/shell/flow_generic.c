#include <trx/game/shell.h>

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdint.h>

void Shell_InitIDBFS(void)
{
}

void Shell_WaitForUserInput(void)
{
}

void Shell_PersistConfigToIDBFS(void)
{
}

void Shell_PersistSavesToIDBFS(void)
{
}

bool Shell_HasTouchSupport(void)
{
    return false;
}

void Shell_SetTouchControlsVisible(const bool visible)
{
}

uint32_t Shell_GetWindowExtraFlags(void)
{
    return 0;
}

void Shell_SetupGLContextVersion(void)
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
}
