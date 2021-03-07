#ifndef T1M_JSON_UTILS_H
#define T1M_JSON_UTILS_H

#include "json-parser/json.h"
#include <stdbool.h>

struct json_value_s* JSONGetField(struct json_value_s* root, const char* name);
int JSONGetBooleanValue(
    struct json_value_s* root, const char* name, int8_t* value);
int JSONGetIntegerValue(
    struct json_value_s* root, const char* name, int32_t* value);
int JSONGetStringValue(
    struct json_value_s* root, const char* name, const char** default_value);

#endif
