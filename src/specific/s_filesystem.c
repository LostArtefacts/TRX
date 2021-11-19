#include "specific/s_filesystem.h"

#include "log.h"

#include <shlwapi.h>

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
    HMODULE module = NULL;
    DWORD flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
        | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;

    if (!GetModuleHandleEx(flags, NULL, &module)) {
        LOG_ERROR("Can't get module handle");
        return NULL;
    }

    static char path[MAX_PATH];
    GetModuleFileNameA(module, path, sizeof(path));
    PathRemoveFileSpecA(path);

    return path;
}
