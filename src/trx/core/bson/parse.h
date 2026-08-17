#pragma once

#include <trx/core/bson/types.h>
#include <trx/core/result.h>

// Reads BSON into a JSON structure, reporting what it could not make sense of
// and where. Caller frees the value with JSON_ValueFree().
RESULT BSON_Parse(const char *src, size_t src_size, JSON_VALUE **out_value);

JSON_VALUE *BSON_ParseEx(
    const char *src, size_t src_size, BSON_PARSE_RESULT *result);

const char *BSON_GetErrorDescription(BSON_PARSE_ERROR error);
