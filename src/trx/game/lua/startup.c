#include <trx/game/lua/startup.h>

#include <trx/core/filesystem.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/vector.h>
#include <trx/game/console/common.h>
#include <trx/game/lua/common.h>
#include <trx/game/paths.h>

#include <stdlib.h>
#include <string.h>

static int M_CompareScriptPaths(const void *const a, const void *const b)
{
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

// Lists startup scripts in name order so recordings run the same way on every
// machine.
static VECTOR *M_ListStartupScripts(void)
{
    VECTOR *const paths = Vector_Create(sizeof(char *));
    const char *const dir_path =
        GamePath_PeekResolve(GAME_DYNAMIC_PATH_STARTUP_SCRIPT_DIR, "");
    if (dir_path == nullptr) {
        return paths;
    }
    FS_DIR *const dir = FS_OpenDirectory(dir_path);
    if (dir == nullptr) {
        return paths;
    }

    const char *entry;
    while ((entry = FS_ReadDirectory(dir)) != nullptr) {
        if (!String_EndsWith(entry, ".lua")) {
            continue;
        }
        char *const path = String_Format("%s/%s", dir_path, entry);
        Vector_Add(paths, &path);
    }
    FS_CloseDirectory(dir);
    qsort(
        Vector_GetData(paths), paths->count, sizeof(char *),
        M_CompareScriptPaths);
    return paths;
}

void LUA_RunStartupScripts(void)
{
    VECTOR *const paths = M_ListStartupScripts();
    for (int32_t i = 0; i < paths->count; i++) {
        char *const path = *(char **)Vector_Get(paths, i);
        LOG_INFO("Running Lua startup script: %s", path);
        LUA_RESULT res = LUA_EvalFile(path);
        if (res.code != LUA_OK) {
            Console_ShowError("Lua startup script error: %s", res.message);
        }
        LUA_FreeResult(&res);
        Memory_Free(path);
    }
    Vector_Free(paths);
}
