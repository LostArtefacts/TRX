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

void S_File_CreateDirectory(const char *path)
{
#if defined(_WIN32)
    #include <direct.h>
    _mkdir(path);
#else
    #include <sys/types.h>
    #include <sys/stat.h>
    mkdir(path, 0664);
#endif
}