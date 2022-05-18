#include "json/bson_write.h"

#include "json/json_base.h"
#include "log.h"
#include "memory.h"

#include <assert.h>
#include <float.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool bson_write_get_marker_size(size_t *size, const char *key);
static bool bson_write_get_null_wrapped(size_t *size, const char *key);
static bool bson_write_get_boolean_wrapped_size(size_t *size, const char *key);
static bool bson_write_get_int32_size(size_t *size);
static bool bson_write_get_int32_wrapped_size(size_t *size, const char *key);
static bool bson_write_get_double_size(size_t *size);
static bool bson_write_get_double_wrapped_size(size_t *size, const char *key);
static bool bson_write_get_number_wrapped_size(
    size_t *size, const char *key, const struct json_number_s *number);
static bool bson_write_get_string_size(
    size_t *size, const struct json_string_s *string);
static bool bson_write_get_string_wrapped_size(
    size_t *size, const char *key, const struct json_string_s *string);
static bool bson_write_get_array_size(
    size_t *size, const struct json_array_s *array);
static bool bson_write_get_array_wrapped_size(
    size_t *size, const char *key, const struct json_array_s *array);
static bool bson_write_get_object_size(
    size_t *size, const struct json_object_s *object);
static bool bson_write_get_object_wrapped_size(
    size_t *size, const char *key, const struct json_object_s *object);
static bool bson_write_get_value_size(
    size_t *size, const struct json_value_s *value);
static bool bson_write_get_value_wrapped_size(
    size_t *size, const char *key, const struct json_value_s *value);

static char *bson_write_marker(
    char *data, const char *key, const uint8_t marker);
static char *bson_write_null_wrapped(char *data, const char *key);
static char *bson_write_boolean_wrapped(
    char *data, const char *key, bool value);
static char *bson_write_int32(char *data, const int32_t value);
static char *bson_write_int32_wrapped(
    char *data, const char *key, const int32_t value);
static char *bson_write_double(char *data, const double value);
static char *bson_write_double_wrapped(
    char *data, const char *key, const double value);
static char *bson_write_number_wrapped(
    char *data, const char *key, const struct json_number_s *number);
static char *bson_write_string(char *data, const struct json_string_s *string);
static char *bson_write_string_wrapped(
    char *data, const char *key, const struct json_string_s *string);
static char *bson_write_array(char *data, const struct json_array_s *array);
static char *bson_write_array_wrapped(
    char *data, const char *key, const struct json_array_s *array);
static char *bson_write_object(char *data, const struct json_object_s *object);
static char *bson_write_object_wrapped(
    char *data, const char *key, const struct json_object_s *object);
static char *bson_write_value(char *data, const struct json_value_s *value);
static char *bson_write_value_wrapped(
    char *data, const char *key, const struct json_value_s *value);

static bool bson_write_get_marker_size(size_t *size, const char *key)
{
    assert(size);
    assert(key);
    *size += 1; // marker
    *size += strlen(key); // key
    *size += 1; // NULL terminator
    return true;
}

static bool bson_write_get_null_wrapped_size(size_t *size, const char *key)
{
    assert(size);
    assert(key);
    return bson_write_get_marker_size(size, key);
}

static bool bson_write_get_boolean_wrapped_size(size_t *size, const char *key)
{
    assert(size);
    assert(key);
    if (!bson_write_get_marker_size(size, key)) {
        return false;
    }
    *size += 1;
    return true;
}

static bool bson_write_get_int32_size(size_t *size)
{
    assert(size);
    *size += sizeof(int32_t);
    return true;
}

static bool bson_write_get_int32_wrapped_size(size_t *size, const char *key)
{
    assert(size);
    assert(key);
    if (!bson_write_get_marker_size(size, key)) {
        return false;
    }
    if (!bson_write_get_int32_size(size)) {
        return false;
    }
    return true;
}

static bool bson_write_get_double_size(size_t *size)
{
    assert(size);
    *size += sizeof(double);
    return true;
}

static bool bson_write_get_double_wrapped_size(size_t *size, const char *key)
{
    assert(size);
    assert(key);
    if (!bson_write_get_marker_size(size, key)) {
        return false;
    }
    if (!bson_write_get_double_size(size)) {
        return false;
    }
    return true;
}

static bool bson_write_get_number_wrapped_size(
    size_t *size, const char *key, const struct json_number_s *number)
{
    assert(size);
    assert(key);

    char *str = number->number;
    assert(str);

    // hexadecimal numbers
    if (number->number_size >= 2 && (str[1] == 'x' || str[1] == 'X')) {
        return bson_write_get_int32_wrapped_size(size, key);
    }

    // skip leading sign
    if (str[0] == '+' || str[0] == '-') {
        str += 1;
    }
    assert(str[0]);

    if (!strcmp(str, "Infinity")) {
        // BSON does not support Infinity.
        return bson_write_get_double_wrapped_size(size, key);
    } else if (!strcmp(str, "NaN")) {
        // BSON does not support NaN.
        return bson_write_get_int32_wrapped_size(size, key);
    } else if (strchr(str, '.')) {
        return bson_write_get_double_wrapped_size(size, key);
    } else {
        return bson_write_get_int32_wrapped_size(size, key);
    }

    return false;
}

static bool bson_write_get_string_size(
    size_t *size, const struct json_string_s *string)
{
    assert(size);
    assert(string);
    *size += sizeof(uint32_t); // size
    *size += string->string_size; // string
    *size += 1; // NULL terminator
    return true;
}

static bool bson_write_get_string_wrapped_size(
    size_t *size, const char *key, const struct json_string_s *string)
{
    assert(size);
    assert(key);
    assert(string);
    if (!bson_write_get_marker_size(size, key)) {
        return false;
    }
    if (!bson_write_get_string_size(size, string)) {
        return false;
    }
    return true;
}

static bool bson_write_get_array_size(
    size_t *size, const struct json_array_s *array)
{
    assert(size);
    assert(array);
    char key[12];
    int idx = 0;
    *size += sizeof(int32_t); // object size
    for (struct json_array_element_s *element = array->start;
         element != json_null; element = element->next) {
        sprintf(key, "%d", idx);
        idx++;
        if (!bson_write_get_value_wrapped_size(size, key, element->value)) {
            return false;
        }
    }
    *size += 1; // NULL terminator
    return true;
}

static bool bson_write_get_array_wrapped_size(
    size_t *size, const char *key, const struct json_array_s *array)
{
    assert(size);
    assert(key);
    assert(array);
    if (!bson_write_get_marker_size(size, key)) {
        return false;
    }
    if (!bson_write_get_array_size(size, array)) {
        return false;
    }
    return true;
}

static bool bson_write_get_object_size(
    size_t *size, const struct json_object_s *object)
{
    assert(size);
    assert(object);
    *size += sizeof(int32_t); // object size
    for (struct json_object_element_s *element = object->start;
         element != json_null; element = element->next) {
        if (!bson_write_get_value_wrapped_size(
                size, element->name->string, element->value)) {
            return false;
        }
    }
    *size += 1; // NULL terminator
    return true;
}

static bool bson_write_get_object_wrapped_size(
    size_t *size, const char *key, const struct json_object_s *object)
{
    assert(size);
    assert(key);
    assert(object);
    if (!bson_write_get_marker_size(size, key)) {
        return false;
    }
    if (!bson_write_get_object_size(size, object)) {
        return false;
    }
    return true;
}

static bool bson_write_get_value_size(
    size_t *size, const struct json_value_s *value)
{
    assert(size);
    assert(value);
    switch (value->type) {
    case json_type_array:
        return bson_write_get_array_size(
            size, (struct json_array_s *)value->payload);
    case json_type_object:
        return bson_write_get_object_size(
            size, (struct json_object_s *)value->payload);
    default:
        LOG_ERROR("Bad BSON root element: %d", value->type);
    }
    return false;
}

static bool bson_write_get_value_wrapped_size(
    size_t *size, const char *key, const struct json_value_s *value)
{
    assert(size);
    assert(key);
    assert(value);
    switch (value->type) {
    case json_type_null:
        return bson_write_get_null_wrapped_size(size, key);
    case json_type_true:
        return bson_write_get_boolean_wrapped_size(size, key);
    case json_type_false:
        return bson_write_get_boolean_wrapped_size(size, key);
    case json_type_number:
        return bson_write_get_number_wrapped_size(
            size, key, (struct json_number_s *)value->payload);
    case json_type_string:
        return bson_write_get_string_wrapped_size(
            size, key, (struct json_string_s *)value->payload);
    case json_type_array:
        return bson_write_get_array_wrapped_size(
            size, key, (struct json_array_s *)value->payload);
    case json_type_object:
        return bson_write_get_object_wrapped_size(
            size, key, (struct json_object_s *)value->payload);
    default:
        LOG_ERROR("Unknown JSON element: %d", value->type);
        return false;
    }
}

static char *bson_write_marker(
    char *data, const char *key, const uint8_t marker)
{
    assert(data);
    assert(key);
    *data++ = marker;
    strcpy(data, key);
    data += strlen(key);
    *data++ = '\0';
    return data;
}

static char *bson_write_null_wrapped(char *data, const char *key)
{
    assert(data);
    assert(key);
    return bson_write_marker(data, key, '\x0A');
}

static char *bson_write_boolean_wrapped(char *data, const char *key, bool value)
{
    assert(data);
    assert(key);
    data = bson_write_marker(data, key, '\x08');
    *(int8_t *)data++ = (int8_t)value;
    return data;
}

static char *bson_write_int32(char *data, const int32_t value)
{
    assert(data);
    *(int32_t *)data = value;
    data += sizeof(int32_t);
    return data;
}

static char *bson_write_int32_wrapped(
    char *data, const char *key, const int32_t value)
{
    assert(data);
    assert(key);
    data = bson_write_marker(data, key, '\x10');
    return bson_write_int32(data, value);
}

static char *bson_write_double(char *data, const double value)
{
    assert(data);
    *(double *)data = value;
    data += sizeof(double);
    return data;
}

static char *bson_write_double_wrapped(
    char *data, const char *key, const double value)
{
    assert(data);
    assert(key);
    data = bson_write_marker(data, key, '\x01');
    return bson_write_double(data, value);
}

static char *bson_write_number_wrapped(
    char *data, const char *key, const struct json_number_s *number)
{
    assert(data);
    assert(key);
    assert(number);
    char *str = number->number;

    // hexadecimal numbers
    if (number->number_size >= 2 && (str[1] == 'x' || str[1] == 'X')) {
        return bson_write_int32_wrapped(
            data, key, json_strtoumax(number->number, json_null, 0));
    }

    // skip leading sign
    if (str[0] == '+' || str[0] == '-') {
        str++;
    }
    assert(str[0]);

    if (!strcmp(str, "Infinity")) {
        // BSON does not support Infinity.
        return bson_write_double_wrapped(data, key, DBL_MAX);
    } else if (!strcmp(str, "NaN")) {
        // BSON does not support NaN.
        return bson_write_int32_wrapped(data, key, 0);
    } else if (strchr(str, '.')) {
        return bson_write_double_wrapped(data, key, atof(number->number));
    } else {
        return bson_write_int32_wrapped(data, key, atoi(number->number));
    }

    return data;
}

static char *bson_write_string(char *data, const struct json_string_s *string)
{
    assert(data);
    assert(string);
    *(uint32_t *)data = string->string_size + 1;
    data += sizeof(uint32_t);
    memcpy(data, string->string, string->string_size);
    data += string->string_size;
    *data++ = '\0';
    return data;
}

static char *bson_write_string_wrapped(
    char *data, const char *key, const struct json_string_s *string)
{
    assert(data);
    assert(key);
    assert(string);
    data = bson_write_marker(data, key, '\x02');
    data = bson_write_string(data, string);
    return data;
}

static char *bson_write_array(char *data, const struct json_array_s *array)
{
    assert(data);
    assert(array);
    char key[12];
    int idx = 0;
    char *old = data;
    data += sizeof(int32_t);
    for (struct json_array_element_s *element = array->start;
         element != json_null; element = element->next) {
        sprintf(key, "%d", idx);
        idx++;
        data = bson_write_value_wrapped(data, key, element->value);
    }
    *data++ = '\0';
    *(int32_t *)old = data - old;
    return data;
}

static char *bson_write_array_wrapped(
    char *data, const char *key, const struct json_array_s *array)
{
    assert(data);
    assert(key);
    assert(array);
    data = bson_write_marker(data, key, '\x04');
    data = bson_write_array(data, array);
    return data;
}

static char *bson_write_object(char *data, const struct json_object_s *object)
{
    assert(data);
    assert(object);
    char *old = data;
    data += sizeof(int32_t);
    for (struct json_object_element_s *element = object->start;
         element != json_null; element = element->next) {
        data = bson_write_value_wrapped(
            data, element->name->string, element->value);
    }
    *data++ = '\0';
    *(int32_t *)old = data - old;
    return data;
}

static char *bson_write_object_wrapped(
    char *data, const char *key, const struct json_object_s *object)
{
    assert(data);
    assert(key);
    assert(object);
    data = bson_write_marker(data, key, '\x03');
    data = bson_write_object(data, object);
    return data;
}

static char *bson_write_value(char *data, const struct json_value_s *value)
{
    assert(data);
    assert(value);
    switch (value->type) {
    case json_type_array:
        data = bson_write_array(data, (struct json_array_s *)value->payload);
        break;
    case json_type_object:
        data = bson_write_object(data, (struct json_object_s *)value->payload);
        break;
    default:
        assert(0);
    }
    return data;
}

static char *bson_write_value_wrapped(
    char *data, const char *key, const struct json_value_s *value)
{
    assert(data);
    assert(key);
    assert(value);
    switch (value->type) {
    case json_type_null:
        return bson_write_null_wrapped(data, key);
    case json_type_true:
        return bson_write_boolean_wrapped(data, key, true);
    case json_type_false:
        return bson_write_boolean_wrapped(data, key, false);
    case json_type_number:
        return bson_write_number_wrapped(
            data, key, (struct json_number_s *)value->payload);
    case json_type_string:
        return bson_write_string_wrapped(
            data, key, (struct json_string_s *)value->payload);
    case json_type_array:
        return bson_write_array_wrapped(
            data, key, (struct json_array_s *)value->payload);
    case json_type_object:
        return bson_write_object_wrapped(
            data, key, (struct json_object_s *)value->payload);
    default:
        return json_null;
    }
}

void *bson_write(const struct json_value_s *value, size_t *out_size)
{
    assert(value);
    *out_size = -1;
    if (value == json_null) {
        return json_null;
    }

    size_t size = 0;
    if (!bson_write_get_value_size(&size, value)) {
        return json_null;
    }

    char *data = Memory_Alloc(size);
    char *data_end = bson_write_value(data, value);
    assert((size_t)(data_end - data) == size);

    if (out_size != json_null) {
        *out_size = size;
    }

    return data;
}
