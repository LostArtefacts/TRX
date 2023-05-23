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

char *S_File_CasePath(char const *path)
{
    assert(path);

    char *current_path = Memory_Alloc(strlen(path) + 2);
    const char *sep = strchr(path, '/') ? "/" : "\\";

    char *path_copy = Memory_DupStr(path);
    char *path_piece = path_copy;

    if (path_copy[0] == '/') {
        strcpy(current_path, "/");
    } else if (strstr(path_copy, ":\\")) {
        strcpy(current_path, path_copy);
        strstr(current_path, ":\\")[1] = '\0';
        path_piece += 3;
    } else {
        strcpy(current_path, ".");
    }

    while (path_piece) {
        char *delim = strpbrk(path_piece, "/\\");
        char old_delim = delim ? *delim : '\0';
        if (delim) {
            *delim = '\0';
        }

        DIR *path_dir = opendir(current_path);
        if (!path_dir) {
            Memory_FreePointer(&path_copy);
            Memory_FreePointer(&current_path);
            return NULL;
        }

        struct dirent *cur_file = readdir(path_dir);
        while (cur_file) {
            if (strcasecmp(path_piece, cur_file->d_name) == 0) {
                strcat(current_path, sep);
                strcat(current_path, cur_file->d_name);
                break;
            }
            cur_file = readdir(path_dir);
        }
        closedir(path_dir);

        if (!cur_file) {
            strcat(current_path, sep);
            strcat(current_path, path_piece);
        }

        if (delim) {
            *delim = old_delim;
            path_piece = delim + 1;
        } else {
            break;
        }
    }

    Memory_FreePointer(&path_copy);

    char *result;
    if (current_path[0] == '.') { /* strip leading ./ */
        result = Memory_DupStr(current_path + 2);
    } else {
        result = Memory_DupStr(current_path);
    }
    return result;
}
