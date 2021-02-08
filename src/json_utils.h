#ifndef TR1M_JSON_UTILS_H
#define TR1M_JSON_UTILS_H

#include "json-parser/json.h"

json_value *get_json_field(json_value *root, json_type fieldType, const char *name, int *pIndex);
int get_json_boolean_field_value(json_value *root, const char *name);

#endif
