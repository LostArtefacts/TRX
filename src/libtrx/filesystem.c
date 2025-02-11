#include "filesystem.h"

#include "debug.h"
#include "log.h"
#include "memory.h"
#include "strings.h"
#include "utils.h"

#include <SDL2/SDL_filesystem.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
    #include <direct.h>
    #define PATH_SEPARATOR "\\"
#else
    #include <sys/stat.h>
    #define PATH_SEPARATOR "/"
#endif

struct MYFILE {
    FILE *fp;
    const char *path;
};

const char *m_GameDir = nullptr;

static void M_PathAppendSeparator(char *path);
static void M_PathAppendPart(char *path, const char *part);
static char *M_CasePath(char const *path);
static bool M_ExistsRaw(const char *path);

static void M_PathAppendSeparator(char *path)
{
    if (!String_EndsWith(path, PATH_SEPARATOR)) {
        strcat(path, PATH_SEPARATOR);
    }
}

static void M_PathAppendPart(char *path, const char *part)
{
    M_PathAppendSeparator(path);
    strcat(path, part);
}

static char *M_CasePath(char const *path)
{
    ASSERT(path != nullptr);

    char *path_copy = Memory_DupStr(path);
    if (M_ExistsRaw(path)) {
        return path_copy;
    }

    char *path_piece = path_copy;
    char *current_path = Memory_Alloc(strlen(path) + 2);

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
            return nullptr;
        }

        struct dirent *cur_file = readdir(path_dir);
        while (cur_file) {
            if (String_Equivalent(path_piece, cur_file->d_name)) {
                M_PathAppendPart(current_path, cur_file->d_name);
                break;
            }
            cur_file = readdir(path_dir);
        }
        closedir(path_dir);

        if (!cur_file) {
            M_PathAppendPart(current_path, path_piece);
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

static bool M_ExistsRaw(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (fp) {
        fclose(fp);
        return true;
    }
    return false;
}

bool File_IsAbsolute(const char *path)
{
    return path && (path[0] == '/' || strstr(path, ":\\"));
}

bool File_IsRelative(const char *path)
{
    return path && !File_IsAbsolute(path);
}

const char *File_GetGameDirectory(void)
{
    if (m_GameDir == nullptr) {
        m_GameDir = SDL_GetBasePath();
        if (!m_GameDir) {
            LOG_ERROR("Can't get module handle");
            return nullptr;
        }
    }
    return m_GameDir;
}

bool File_DirExists(const char *path)
{
    char *full_path = File_GetFullPath(path);
    DIR *dir = opendir(path);
    Memory_FreePointer(&full_path);
    if (dir != nullptr) {
        closedir(dir);
        return true;
    }
    return false;
}

bool File_Exists(const char *path)
{
    char *full_path = File_GetFullPath(path);
    bool ret = M_ExistsRaw(full_path);
    Memory_FreePointer(&full_path);
    return ret;
}

char *File_GetFullPath(const char *path)
{
    char *full_path = nullptr;
    if (File_IsRelative(path)) {
        const char *game_dir = File_GetGameDirectory();
        if (game_dir) {
            full_path = Memory_Alloc(strlen(game_dir) + strlen(path) + 1);
            sprintf(full_path, "%s%s", game_dir, path);
        }
    }
    if (!full_path) {
        full_path = Memory_DupStr(path);
    }

    char *case_path = M_CasePath(full_path);
    if (case_path) {
        Memory_FreePointer(&full_path);
        return case_path;
    }

    return full_path;
}

char *File_GetParentDirectory(const char *path)
{
    char *full_path = File_GetFullPath(path);
    char *const last_delim =
        MAX(strrchr(full_path, '/'), strrchr(full_path, '\\'));
    if (last_delim != nullptr) {
        *last_delim = '\0';
    }
    return full_path;
}

char *File_GuessExtension(const char *path, const char **extensions)
{
    if (!File_Exists(path)) {
        const char *dot = strrchr(path, '.');
        if (dot) {
            for (const char **ext = &extensions[0]; *ext; ext++) {
                size_t out_size = dot - path + strlen(*ext) + 1;
                char *out = Memory_Alloc(out_size);
                strncpy(out, path, dot - path);
                out[dot - path] = '\0';
                strcat(out, *ext);
                if (File_Exists(out)) {
                    return out;
                }
                Memory_FreePointer(&out);
            }
        }
    }
    return Memory_DupStr(path);
}

MYFILE *File_Open(const char *path, FILE_OPEN_MODE mode)
{
    char *full_path = File_GetFullPath(path);
    MYFILE *file = Memory_Alloc(sizeof(MYFILE));
    file->path = Memory_DupStr(path);
    switch (mode) {
    case FILE_OPEN_WRITE:
        file->fp = fopen(full_path, "wb");
        break;
    case FILE_OPEN_READ:
        file->fp = fopen(full_path, "rb");
        break;
    case FILE_OPEN_READ_WRITE:
        file->fp = fopen(full_path, "r+b");
        break;
    default:
        file->fp = nullptr;
        break;
    }
    Memory_FreePointer(&full_path);
    if (!file->fp) {
        Memory_FreePointer(&file->path);
        Memory_FreePointer(&file);
    }
    return file;
}

void File_ReadData(MYFILE *const file, void *const data, const size_t size)
{
    fread(data, size, 1, file->fp);
}

void File_ReadItems(
    MYFILE *const file, void *data, const size_t count, const size_t item_size)
{
    fread(data, item_size, count, file->fp);
}

int8_t File_ReadS8(MYFILE *const file)
{
    int8_t result;
    fread(&result, sizeof(result), 1, file->fp);
    return result;
}

int16_t File_ReadS16(MYFILE *const file)
{
    int16_t result;
    fread(&result, sizeof(result), 1, file->fp);
    return result;
}

int32_t File_ReadS32(MYFILE *const file)
{
    int32_t result;
    fread(&result, sizeof(result), 1, file->fp);
    return result;
}

uint8_t File_ReadU8(MYFILE *const file)
{
    uint8_t result;
    fread(&result, sizeof(result), 1, file->fp);
    return result;
}

uint16_t File_ReadU16(MYFILE *const file)
{
    uint16_t result;
    fread(&result, sizeof(result), 1, file->fp);
    return result;
}

uint32_t File_ReadU32(MYFILE *const file)
{
    uint32_t result;
    fread(&result, sizeof(result), 1, file->fp);
    return result;
}

void File_WriteData(
    MYFILE *const file, const void *const data, const size_t size)
{
    fwrite(data, size, 1, file->fp);
}

void File_WriteItems(
    MYFILE *const file, const void *const data, const size_t count,
    const size_t item_size)
{
    fwrite(data, item_size, count, file->fp);
}

void File_WriteS8(MYFILE *const file, const int8_t value)
{
    fwrite(&value, sizeof(value), 1, file->fp);
}

void File_WriteS16(MYFILE *const file, const int16_t value)
{
    fwrite(&value, sizeof(value), 1, file->fp);
}

void File_WriteS32(MYFILE *const file, const int32_t value)
{
    fwrite(&value, sizeof(value), 1, file->fp);
}

void File_WriteU8(MYFILE *const file, const uint8_t value)
{
    fwrite(&value, sizeof(value), 1, file->fp);
}

void File_WriteU16(MYFILE *const file, const uint16_t value)
{
    fwrite(&value, sizeof(value), 1, file->fp);
}

void File_WriteU32(MYFILE *const file, const uint32_t value)
{
    fwrite(&value, sizeof(value), 1, file->fp);
}

void File_Skip(MYFILE *file, size_t bytes)
{
    File_Seek(file, bytes, FILE_SEEK_CUR);
}

void File_Seek(MYFILE *file, size_t pos, FILE_SEEK_MODE mode)
{
    switch (mode) {
    case FILE_SEEK_SET:
        fseek(file->fp, pos, SEEK_SET);
        break;
    case FILE_SEEK_CUR:
        fseek(file->fp, pos, SEEK_CUR);
        break;
    case FILE_SEEK_END:
        fseek(file->fp, pos, SEEK_END);
        break;
    }
}

size_t File_Pos(MYFILE *file)
{
    return ftell(file->fp);
}

size_t File_Size(MYFILE *file)
{
    size_t old = ftell(file->fp);
    fseek(file->fp, 0, SEEK_END);
    size_t size = ftell(file->fp);
    fseek(file->fp, old, SEEK_SET);
    return size;
}

const char *File_GetPath(MYFILE *file)
{
    return file->path;
}

void File_Close(MYFILE *file)
{
    fclose(file->fp);
    Memory_FreePointer(&file->path);
    Memory_FreePointer(&file);
}

bool File_Load(const char *path, char **output_data, size_t *output_size)
{
    ASSERT(output_data != nullptr);

    MYFILE *fp = File_Open(path, FILE_OPEN_READ);
    if (!fp) {
        LOG_ERROR("Can't open file %s", path);
        *output_data = nullptr;
        return false;
    }

    size_t data_size = File_Size(fp);
    char *data = Memory_Alloc(data_size + 1);
    File_ReadData(fp, data, data_size);
    if (File_Pos(fp) != data_size) {
        *output_data = nullptr;
        LOG_ERROR("Can't read file %s", path);
        Memory_FreePointer(&data);
        File_Close(fp);
        return false;
    }
    File_Close(fp);
    data[data_size] = '\0';

    *output_data = data;
    if (output_size != nullptr) {
        *output_size = data_size;
    }
    return true;
}

void File_CreateDirectory(const char *path)
{
    char *full_path = File_GetFullPath(path);
    ASSERT(full_path != nullptr);
#if defined(_WIN32)
    _mkdir(full_path);
#else
    mkdir(full_path, 0775);
#endif
    Memory_FreePointer(&full_path);
}
