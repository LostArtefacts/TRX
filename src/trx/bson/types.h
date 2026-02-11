#pragma once

#include <trx/bson/enum.h>
#include <trx/json/base.h>

typedef struct {
    BSON_PARSE_ERROR error;
    size_t error_offset;
} BSON_PARSE_RESULT;
