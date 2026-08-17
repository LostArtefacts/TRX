#pragma once

#include <trx/core/bson/types.h>
#include <trx/core/result.h>

// Writes a JSON structure out as BSON, reporting data BSON cannot hold.
// Caller frees the bytes with Memory_Free().
RESULT BSON_Write(const JSON_VALUE *value, void **out_data, size_t *out_size);
