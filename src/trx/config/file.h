#pragma once

#include <trx/config/option.h>
#include <trx/core/enum_map.h>
#include <trx/core/json.h>
#include <trx/core/vector.h>

#include <stdint.h>

typedef struct {
    const char *default_path;
    const char *enforced_path;
    void (*action)(JSON_OBJECT *root_obj);
    VECTOR *hidden_targets;
} CONFIG_IO_ARGS;

bool ConfigFile_Read(const CONFIG_IO_ARGS *control);
bool ConfigFile_Write(const CONFIG_IO_ARGS *control);

void ConfigFile_LoadOptions(
    JSON_OBJECT *root_obj, const CONFIG_OPTION *options);
void ConfigFile_DumpOptions(
    JSON_OBJECT *root_obj, const CONFIG_OPTION *options);
