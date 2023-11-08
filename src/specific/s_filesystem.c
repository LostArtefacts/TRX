#include "specific/s_filesystem.h"

#include "log.h"
#include "memory.h"
#include "strings.h"

#include <SDL2/SDL_filesystem.h>
#include <assert.h>
#include <dirent.h>
#include <stdbool.h>
#include <string.h>

#if defined(_WIN32)
    #include <direct.h>
    #define PATH_SEPARATOR "\\"
#else
    #include <sys/stat.h>
    #define PATH_SEPARATOR "/"
#endif

const char *m_GameDir = NULL;

static bool S_File_StringEndsWith(const char *str, const char *suffix);
static void S_File_PathAppendSeparator(char *path);
static void S_File_PathAppendPart(char *path, const char *part);

static bool S_File_StringEndsWith(const char *str, const char *suffix)
{
    int str_len = strlen(str);
    int suffix_len = strlen(suffix);

    if (suffix_len > str_len) {
        return 0;
    }

    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

static void S_File_PathAppendSeparator(char *path)
{
    if (!S_File_StringEndsWith(path, PATH_SEPARATOR)) {
        strcat(path, PATH_SEPARATOR);
    }
}

static void S_File_PathAppendPart(char *path, const char *part)
{
    S_File_PathAppendSeparator(path);
    strcat(path, part);
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
    mkdir(path, 0775);
#endif
}

char *S_File_CasePath(char const *path)
{
    assert(path);

    char *current_path = Memory_Alloc(strlen(path) + 2);

    char *path_copy = Memory_DupStr(path);
    char *path_piece = path_copy;

    if (path_copy[0] == '/') {
        strcpy(current_path, "/");
        path_piece++;
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
            if (String_Equivalent(path_piece, cur_file->d_name)) {
                S_File_PathAppendPart(current_path, cur_file->d_name);
                break;
            }
            cur_file = readdir(path_dir);
        }
        closedir(path_dir);

        if (!cur_file) {
            S_File_PathAppendPart(current_path, path_piece);
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
    if (current_path[0] == '.'
        && strcmp(current_path + 1, PATH_SEPARATOR)
            == 0) { /* strip leading ./ */
        result = Memory_DupStr(current_path + 1 + strlen(PATH_SEPARATOR));
    } else {
        result = Memory_DupStr(current_path);
    }
    Memory_FreePointer(&current_path);
    return result;
}
