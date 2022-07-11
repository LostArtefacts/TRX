#pragma once

#include "json/json_base.h"

#include <stddef.h>

enum bson_parse_error_e {
    bson_parse_error_none = 0,
    bson_parse_error_invalid_value,
    bson_parse_error_premature_end_of_buffer,
    bson_parse_error_unexpected_trailing_bytes,
    bson_parse_error_unknown,
};

struct bson_parse_result_s {
    enum bson_parse_error_e error;
    size_t error_offset;
};

// Parse a BSON file, returning a pointer to the root of the JSON structure.
// Returns NULL if an error occurred (malformed BSON input, or malloc failed).
struct json_value_s *bson_parse(const char *src, size_t src_size);

struct json_value_s *bson_parse_ex(
    const char *src, size_t src_size, struct bson_parse_result_s *result);

const char *bson_get_error_description(enum bson_parse_error_e error);
