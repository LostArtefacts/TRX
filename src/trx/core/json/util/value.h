#pragma once

#include <trx/core/json.h>
#include <trx/core/result.h>
#include <trx/core/value.h>

// Bridges a TRX_VALUE and its JSON form, so the object property and config
// layers serialize a value the same way rather than each hand-rolling a
// per-type switch. Numbers, booleans and vectors are native JSON; colours and
// strings are text; a static enum is written and read as its EnumMap name,
// which `param` supplies (unused by the other types).

void JSONValue_Write(
    JSON_OBJECT *obj, const char *key, TRX_VALUE_TYPE type, const void *param,
    const TRX_VALUE *value);

// Reads `value` into `out` as `type`. Reports a value that is not there, holds
// the wrong shape, or names an enum the map does not know; the caller decides
// whether to fall back on its own default. A read string is borrowed from
// `value`.
RESULT JSONValue_ReadFrom(
    const JSON_VALUE *value, TRX_VALUE_TYPE type, const void *param,
    TRX_VALUE *out);

// JSONValue_ReadFrom on the value at `key` in `obj`.
RESULT JSONValue_Read(
    const JSON_OBJECT *obj, const char *key, TRX_VALUE_TYPE type,
    const void *param, TRX_VALUE *out);
