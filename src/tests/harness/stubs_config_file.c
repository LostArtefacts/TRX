// common.c reaches the settings document only to read it and to write it back.
// Both live in config/file.c, which drags in the JSON parser and the
// filesystem, and neither is what setting a value is being tested for: an
// option that the file says nothing about keeps its default, which is exactly
// what a test wants of it.
//
// Standing them up here keeps the unit tests engine-free, which is the whole
// reason they are a separate project (see meson.build).

#include <trx/config/file.h>

RESULT ConfigFile_Read(
    const char *const default_path, const char *const enforced_path)
{
    return OK;
}

bool ConfigFile_WasFound(void)
{
    return false;
}

JSON_OBJECT *ConfigFile_GetRoot(void)
{
    return nullptr;
}

void ConfigFile_Forget(void)
{
}

void ConfigFile_ApplyFileValueTo(CONFIG_OPTION *const option)
{
}

void ConfigFile_ApplyEnforcedTo(CONFIG_OPTION *const option)
{
}

RESULT ConfigFile_Write(
    const char *const default_path, void (*const action)(JSON_OBJECT *))
{
    return OK;
}

void ConfigFile_DumpOptions(JSON_OBJECT *const root_obj)
{
}
