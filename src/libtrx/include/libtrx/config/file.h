#pragma once

#include "../enum_map.h"
#include "../json.h"
#include "./option.h"

#include <stdbool.h>
#include <stdint.h>

bool ConfigFile_Read(const char *path, void (*load)(JSON_OBJECT *root_obj));
bool ConfigFile_Write(const char *path, void (*dump)(JSON_OBJECT *root_obj));

void ConfigFile_LoadOptions(
    JSON_OBJECT *root_obj, const CONFIG_OPTION *options);
void ConfigFile_DumpOptions(
    JSON_OBJECT *root_obj, const CONFIG_OPTION *options);

int ConfigFile_ReadEnum(
    JSON_OBJECT *obj, const char *name, int default_value,
    const char *enum_name);
void ConfigFile_WriteEnum(
    JSON_OBJECT *obj, const char *name, int value, const char *enum_name);
