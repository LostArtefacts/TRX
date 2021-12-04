#include "specific/s_filesystem.h"

#include "log.h"

#include <SDL2/SDL.h>
#include <shlwapi.h>

const char *m_GameDir = NULL;

size_t S_File_GetMaxPath()
{
    return MAX_PATH;
}

bool S_File_IsRelative(const char *path)
{
    return PathIsRelativeA(path);
}

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
