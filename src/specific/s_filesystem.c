#include "specific/s_filesystem.h"

#include "log.h"

#include <SDL2/SDL.h>

const char *m_GameDir = NULL;

const char *S_File_GetGameDirectory()
{
    if (!m_GameDir) {
        m_GameDir = SDL_GetBasePath();
        if (!m_GameDir) {
            LOG_ERROR("Can't get module handle");
            return NULL;
        }
    }
    return m_GameDir;
}
