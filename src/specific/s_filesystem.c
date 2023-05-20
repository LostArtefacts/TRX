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
    char *p = start ? strpbrk(start, delim) : NULL;

    if (p) {
        *p = '\0';
        *stringp = p + 1;
    } else {
        *stringp = NULL;
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

bool S_File_CasePath(char const *path, char *case_path)
{
    size_t length = strlen(path);
    char *path_copy = Memory_Alloc(length + 1);
    strcpy(path_copy, path);
    size_t case_len = 0;

    DIR *path_dir;
    if (path_copy[0] == '/') {
        path_dir = opendir("/");
        path_copy = path_copy + 1;
    } else {
        path_dir = opendir(".");
        case_path[0] = '.';
        case_path[1] = 0;
        case_len = 1;
    }

    bool last_file = false;
    char *path_piece = S_File_Strsep(&path_copy, "/");
    while (path_piece) {
        if (!path_dir) {
            Memory_FreePointer(&path_copy);
            return false;
        }

        if (last_file) {
            closedir(path_dir);
            Memory_FreePointer(&path_copy);
            return false;
        }

        case_path[case_len] = '/';
        case_len += 1;
        case_path[case_len] = 0;

        struct dirent *cur_file = readdir(path_dir);
        while (cur_file) {
            if (strcasecmp(path_piece, cur_file->d_name) == 0) {
                strcpy(case_path + case_len, cur_file->d_name);
                case_len += strlen(cur_file->d_name);
                closedir(path_dir);
                path_dir = opendir(case_path);
                break;
            }
            cur_file = readdir(path_dir);
        }

        if (!cur_file) {
            strcpy(case_path + case_len, path_piece);
            case_len += strlen(path_piece);
            last_file = true;
        }

        path_piece = S_File_Strsep(&path_copy, "/");
    }

    if (path_dir) {
        closedir(path_dir);
    }

    Memory_FreePointer(&path_copy);
    return true;
}
