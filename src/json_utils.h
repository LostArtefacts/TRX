#ifndef TR1MAIN_JSON_UTILS_H
#define TR1MAIN_JSON_UTILS_H

#include "json-parser/json.h"

json_value* tr1m_json_get_field(
    json_value* root, json_type field_type, const char* name, int* pIndex);

int tr1m_json_get_boolean_value(json_value* root, const char* name);
const char* tr1m_json_get_string_value(json_value* root, const char* name);

#endif
