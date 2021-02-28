#ifndef T1M_JSON_UTILS_H
#define T1M_JSON_UTILS_H

#include "json-parser/json.h"

struct json_value_s* JSONGetField(struct json_value_s* root, const char* name);
int8_t JSONGetBooleanValue(struct json_value_s* root, const char* name);
int32_t JSONGetIntegerValue(struct json_value_s* root, const char* name);
const char* JSONGetStringValue(struct json_value_s* root, const char* name);

#endif
