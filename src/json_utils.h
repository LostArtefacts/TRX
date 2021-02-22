#ifndef T1M_JSON_UTILS_H
#define T1M_JSON_UTILS_H

#include "json-parser/json.h"

json_value* JSONGetField(
    json_value* root, json_type field_type, const char* name, int* pIndex);

int JSONGetBooleanValue(json_value* root, const char* name);
const char* JSONGetStringValue(json_value* root, const char* name);

#endif
