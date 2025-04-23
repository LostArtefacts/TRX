#pragma once

#include "config/option.h"
#include "enum_map.h"
#include "game/gym.h"
#include "json.h"

#include <stdint.h>

typedef struct {
    const char *default_path;
    const char *enforced_path;
    void (*action)(JSON_OBJECT *root_obj);
} CONFIG_IO_ARGS;

bool ConfigFile_Read(const CONFIG_IO_ARGS *control);
bool ConfigFile_Write(const CONFIG_IO_ARGS *control);

void ConfigFile_LoadOptions(
    JSON_OBJECT *root_obj, const CONFIG_OPTION *options);
void ConfigFile_DumpOptions(
    JSON_OBJECT *root_obj, const CONFIG_OPTION *options);

int ConfigFile_ReadEnum(
    JSON_OBJECT *obj, const char *name, int default_value,
    const char *enum_name);
void ConfigFile_WriteEnum(
    JSON_OBJECT *obj, const char *name, int value, const char *enum_name);

bool ConfigFile_LoadAssaultStats(
    JSON_OBJECT *root_obj, ASSAULT_STATS *assault_stats);
bool ConfigFile_DumpAssaultStats(
    JSON_OBJECT *root_obj, const ASSAULT_STATS *assault_stats);
