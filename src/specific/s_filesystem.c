#include "specific/s_filesystem.h"

#include "log.h"

#include <assert.h>
#include <errno.h>
#include <SDL2/SDL.h>
#include <string.h>

#if defined(_WIN32)
    #include <direct.h>
#else
    #include <sys/types.h>
    #include <sys/stat.h>
#endif

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
    assert(path);
    int32_t err_check = 0;
#if defined(_WIN32)
    err_check = _mkdir(path);
#else
    err_check = mkdir(path, 0664);
#endif
    if (err_check != 0) {
        LOG_ERROR("mkdir failed! %s\n", strerror(errno));
    }
}
