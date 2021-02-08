#ifndef TR1M_JSON_UTILS_H
#define TR1M_JSON_UTILS_H

#include "json-parser/json.h"

json_value *tr1m_json_get_field(
    json_value *root,
    json_type field_type,
    const char *name,
    int *pIndex
);

int tr1m_json_get_boolean_value(json_value *root, const char *name);

#endif
