#include "specific/s_filesystem.h"

#include "log.h"
#include "memory.h"

#include <SDL2/SDL_filesystem.h>
#include <assert.h>
#include <dirent.h>
#include <string.h>

#if defined(_WIN32)
    #include <direct.h>
#else
    #include <sys/stat.h>
#endif

const char *m_GameDir = NULL;

static char *S_File_Strsep(char **stringp, const char *delim);

static char *S_File_Strsep(char **stringp, const char *delim)
{
    char *start = *stringp;
    char *p;

    p = (start != NULL) ? strpbrk(start, delim) : NULL;

    if (p == NULL) {
        *stringp = NULL;
    } else {
        *p = '\0';
        *stringp = p + 1;
    }

    return start;
}

const char *S_File_GetGameDirectory(void)
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
#if defined(_WIN32)
    _mkdir(path);
#else
    mkdir(path, 0664);
#endif
}

// r must have strlen(path) + 2 bytes
bool S_File_CasePath(char const *path, char *r)
{
    size_t length = strlen(path);
    char *p = Memory_Alloc(length + 1);
    strcpy(p, path);
    size_t rl = 0;

    DIR *d;
    if (p[0] == '/') {
        d = opendir("/");
        p = p + 1;
    } else {
        d = opendir(".");
        r[0] = '.';
        r[1] = 0;
        rl = 1;
    }
    LOG_DEBUG("r: %s", r);

    int last = 0;
    char *c = S_File_Strsep(&p, "/");
    while (c) {
        LOG_DEBUG("while c r: %s", r);
        if (!d) {
            Memory_FreePointer(&p);
            return false;
        }

        if (last) {
            closedir(d);
            Memory_FreePointer(&p);
            return false;
        }

        r[rl] = '/';
        rl += 1;
        r[rl] = 0;

        struct dirent *e = readdir(d);
        while (e) {
            if (strcasecmp(c, e->d_name) == 0) {
                strcpy(r + rl, e->d_name);
                rl += strlen(e->d_name);
                closedir(d);
                d = opendir(r);
                break;
            }
            e = readdir(d);
        }

        if (!e) {
            strcpy(r + rl, c);
            rl += strlen(c);
            last = 1;
        }

        c = S_File_Strsep(&p, "/");
    }

    if (d) {
        closedir(d);
    }

    Memory_FreePointer(&p);
    return true;
}
