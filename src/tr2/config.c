#include "config.h"

#include "config_map.h"

#include <libtrx/config/file.h>

CONFIG g_Config = { 0 };

static const char *m_ConfigPath = "cfg/TR2X.json5";

const char *Config_GetPath(void)
{
    return m_ConfigPath;
}

void Config_LoadFromJSON(JSON_OBJECT *root_obj)
{
    ConfigFile_LoadOptions(root_obj, g_ConfigOptionMap);
}

void Config_DumpToJSON(JSON_OBJECT *root_obj)
{
    ConfigFile_DumpOptions(root_obj, g_ConfigOptionMap);
}

void Config_Sanitize(void)
{
}

void Config_ApplyChanges(void)
{
}

const CONFIG_OPTION *Config_GetOptionMap(void)
{
    return g_ConfigOptionMap;
}
