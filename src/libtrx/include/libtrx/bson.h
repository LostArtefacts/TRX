#pragma once

#include "json.h"

typedef enum {
    BSON_PARSE_ERROR_NONE = 0,
    BSON_PARSE_ERROR_INVALID_VALUE,
    BSON_PARSE_ERROR_PREMATURE_END_OF_BUFFER,
    BSON_PARSE_ERROR_UNEXPECTED_TRAILING_BYTES,
    BSON_PARSE_ERROR_UNKNOWN,
} BSON_PARSE_ERROR;

typedef struct {
    BSON_PARSE_ERROR error;
    size_t error_offset;
} BSON_PARSE_RESULT;

// Parse a BSON file, returning a pointer to the root of the JSON structure.
// Returns NULL if an error occurred (malformed BSON input, or malloc failed).
JSON_VALUE *BSON_Parse(const char *src, size_t src_size);

JSON_VALUE *BSON_ParseEx(
    const char *src, size_t src_size, BSON_PARSE_RESULT *result);

const char *BSON_GetErrorDescription(BSON_PARSE_ERROR error);

/* Write out a BSON binary string. Return 0 if an error occurred (malformed
 * JSON input, or malloc failed). The out_size parameter is optional. */
void *BSON_Write(const JSON_VALUE *value, size_t *out_size);
