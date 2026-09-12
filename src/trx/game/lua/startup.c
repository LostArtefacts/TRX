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

#define M_ENTRY_POINT "init.lua"

typedef struct {
    char *path;
    // Directory of the entry point, or null for a loose script.
    char *dir;
} M_SCRIPT;

static const char *m_ScriptDir = nullptr;

static int M_CompareScripts(const void *const a, const void *const b)
{
    return strcmp(((const M_SCRIPT *)a)->path, ((const M_SCRIPT *)b)->path);
}

// Add loose Lua files in name order.
static void M_AddScriptsFrom(VECTOR *const paths, const char *const dir_path)
{
    if (dir_path == nullptr) {
        return;
    }
    FS_DIR *const dir = FS_OpenDirectory(dir_path);
    if (dir == nullptr) {
        return;
    }

    const char *entry;
    while ((entry = FS_ReadDirectory(dir)) != nullptr) {
        if (!String_EndsWith(entry, ".lua")) {
            continue;
        }
        char *path = String_Format("%s/%s", dir_path, entry);
        if (FS_DirExists(path)) {
            Memory_FreePointer(&path);
            continue;
        }
        const M_SCRIPT script = { .path = path, .dir = nullptr };
        Vector_Add(paths, (void *)&script);
    }
    FS_CloseDirectory(dir);
    qsort(
        Vector_GetData(paths), paths->count, sizeof(M_SCRIPT),
        M_CompareScripts);
}

static int M_CompareNames(const void *const a, const void *const b)
{
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

// List subdirectories in name order.
static VECTOR *M_ListSubDirs(const char *const root)
{
    VECTOR *const names = Vector_Create(sizeof(char *));
    FS_DIR *const dir = root == nullptr ? nullptr : FS_OpenDirectory(root);
    if (dir == nullptr) {
        return names;
    }
    const char *entry;
    while ((entry = FS_ReadDirectory(dir)) != nullptr) {
        if (strcmp(entry, ".") == 0 || strcmp(entry, "..") == 0) {
            continue;
        }
        char *name = String_Format("%s/%s", root, entry);
        if (FS_DirExists(name)) {
            Vector_Add(names, &name);
        } else {
            Memory_FreePointer(&name);
        }
    }
    FS_CloseDirectory(dir);
    qsort(Vector_GetData(names), names->count, sizeof(char *), M_CompareNames);
    return names;
}

// Add each subdirectory's entry point. Ignore directories without one.
static void M_AddDirScripts(VECTOR *const paths, const char *const root)
{
    VECTOR *const dirs = M_ListSubDirs(root);
    for (int32_t i = 0; i < dirs->count; i++) {
        char *dir = *(char **)Vector_Get(dirs, i);
        char *entry_point = String_Format("%s/" M_ENTRY_POINT, dir);
        char *path = GamePath_ResolveCase(entry_point);
        Memory_FreePointer(&entry_point);
        if (path == nullptr) {
            Memory_FreePointer(&dir);
            continue;
        }
        const M_SCRIPT script = { .path = path, .dir = dir };
        Vector_Add(paths, (void *)&script);
    }
    Vector_Free(dirs);
}

// List loose scripts before directory scripts.
static VECTOR *M_ListStartupScripts(void)
{
    VECTOR *const paths = Vector_Create(sizeof(M_SCRIPT));
    char *root = Memory_DupStr(
        GamePath_PeekResolve(GAME_DYNAMIC_PATH_STARTUP_SCRIPT_DIR, ""));
    M_AddScriptsFrom(paths, root);
    M_AddDirScripts(paths, root);
    Memory_FreePointer(&root);
    return paths;
}

void LUA_RunStartupScripts(void)
{
    VECTOR *const paths = M_ListStartupScripts();
    for (int32_t i = 0; i < paths->count; i++) {
        M_SCRIPT *const script = Vector_Get(paths, i);
        LOG_INFO("Running Lua startup script: %s", script->path);
        m_ScriptDir = script->dir;
        LUA_RESULT res = LUA_EvalFile(script->path);
        m_ScriptDir = nullptr;
        if (res.code != LUA_OK) {
            Console_ShowError("Lua startup script error: %s", res.message);
        }
        LUA_FreeResult(&res);
        Memory_FreePointer(&script->path);
        Memory_FreePointer(&script->dir);
    }
    Vector_Free(paths);
}

const char *LUA_GetStartupScriptDir(void)
{
    return m_ScriptDir;
}
