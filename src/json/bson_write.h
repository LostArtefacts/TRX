#pragma once

#include "json/json_base.h"

/* Write out a BSON binary string. Return 0 if an error occurred (malformed
 * JSON input, or malloc failed). The out_size parameter is optional. */
void *bson_write(const struct json_value_s *value, size_t *out_size);
