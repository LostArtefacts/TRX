#include "bson.h"

#include "log.h"
#include "memory.h"

#include <assert.h>
#include <float.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool M_GetMarkerSize(size_t *size, const char *key);
static bool M_GetNullWrappedSize(size_t *size, const char *key);
static bool M_GetBoolWrappedSize(size_t *size, const char *key);
static bool M_GetInt32Size(size_t *size);
static bool M_GetInt32WrappedSize(size_t *size, const char *key);
static bool M_GetDoubleSize(size_t *size);
static bool M_GetDoubleWrappedSize(size_t *size, const char *key);
static bool M_GetNumberWrappedSize(
    size_t *size, const char *key, const JSON_NUMBER *number);
static bool M_GetStringSize(size_t *size, const JSON_STRING *string);
static bool M_GetStringWrappedSize(
    size_t *size, const char *key, const JSON_STRING *string);
static bool M_GetArraySize(size_t *size, const JSON_ARRAY *array);
static bool M_GetArrayWrappedSize(
    size_t *size, const char *key, const JSON_ARRAY *array);
static bool M_GetObjectSize(size_t *size, const JSON_OBJECT *object);
static bool M_GetObjectWrappedSize(
    size_t *size, const char *key, const JSON_OBJECT *object);
static bool M_GetValueSize(size_t *size, const JSON_VALUE *value);
static bool M_GetValueWrappedSize(
    size_t *size, const char *key, const JSON_VALUE *value);

static char *M_WriteMarker(char *data, const char *key, const uint8_t marker);
static char *M_WriteNullWrapped(char *data, const char *key);
static char *M_WriteBoolWrapped(char *data, const char *key, bool value);
static char *M_WriteInt32(char *data, const int32_t value);
static char *M_WriteInt32Wrapped(
    char *data, const char *key, const int32_t value);
static char *M_WriteDouble(char *data, const double value);
static char *M_WriteDoubleWrapped(
    char *data, const char *key, const double value);
static char *M_WriteNumberWrapped(
    char *data, const char *key, const JSON_NUMBER *number);
static char *M_WriteString(char *data, const JSON_STRING *string);
static char *M_WriteStringWrapped(
    char *data, const char *key, const JSON_STRING *string);
static char *M_WriteArray(char *data, const JSON_ARRAY *array);
static char *M_WriteArrayWrapped(
    char *data, const char *key, const JSON_ARRAY *array);
static char *M_WriteObject(char *data, const JSON_OBJECT *object);
static char *M_WriteObjectWrapped(
    char *data, const char *key, const JSON_OBJECT *object);
static char *M_WriteValue(char *data, const JSON_VALUE *value);
static char *M_WriteValueWrapped(
    char *data, const char *key, const JSON_VALUE *value);

static bool M_GetMarkerSize(size_t *size, const char *key)
{
    assert(size);
    assert(key);
    *size += 1; // marker
    *size += strlen(key); // key
    *size += 1; // NULL terminator
    return true;
}

static bool M_GetNullWrappedSize(size_t *size, const char *key)
{
    assert(size);
    assert(key);
    return M_GetMarkerSize(size, key);
}

static bool M_GetBoolWrappedSize(size_t *size, const char *key)
{
    assert(size);
    assert(key);
    if (!M_GetMarkerSize(size, key)) {
        return false;
    }
    *size += 1;
    return true;
}

static bool M_GetInt32Size(size_t *size)
{
    assert(size);
    *size += sizeof(int32_t);
    return true;
}

static bool M_GetInt32WrappedSize(size_t *size, const char *key)
{
    assert(size);
    assert(key);
    if (!M_GetMarkerSize(size, key)) {
        return false;
    }
    if (!M_GetInt32Size(size)) {
        return false;
    }
    return true;
}

static bool M_GetDoubleSize(size_t *size)
{
    assert(size);
    *size += sizeof(double);
    return true;
}

static bool M_GetDoubleWrappedSize(size_t *size, const char *key)
{
    assert(size);
    assert(key);
    if (!M_GetMarkerSize(size, key)) {
        return false;
    }
    if (!M_GetDoubleSize(size)) {
        return false;
    }
    return true;
}

static bool M_GetNumberWrappedSize(
    size_t *size, const char *key, const JSON_NUMBER *number)
{
    assert(size);
    assert(key);

    char *str = number->number;
    assert(str);

    // hexadecimal numbers
    if (number->number_size >= 2 && (str[1] == 'x' || str[1] == 'X')) {
        return M_GetInt32WrappedSize(size, key);
    }

    // skip leading sign
    if (str[0] == '+' || str[0] == '-') {
        str += 1;
    }
    assert(str[0]);

    if (!strcmp(str, "Infinity")) {
        // BSON does not support Infinity.
        return M_GetDoubleWrappedSize(size, key);
    } else if (!strcmp(str, "NaN")) {
        // BSON does not support NaN.
        return M_GetInt32WrappedSize(size, key);
    } else if (strchr(str, '.')) {
        return M_GetDoubleWrappedSize(size, key);
    } else {
        return M_GetInt32WrappedSize(size, key);
    }

    return false;
}

static bool M_GetStringSize(size_t *size, const JSON_STRING *string)
{
    assert(size);
    assert(string);
    *size += sizeof(uint32_t); // size
    *size += string->string_size; // string
    *size += 1; // NULL terminator
    return true;
}

static bool M_GetStringWrappedSize(
    size_t *size, const char *key, const JSON_STRING *string)
{
    assert(size);
    assert(key);
    assert(string);
    if (!M_GetMarkerSize(size, key)) {
        return false;
    }
    if (!M_GetStringSize(size, string)) {
        return false;
    }
    return true;
}

static bool M_GetArraySize(size_t *size, const JSON_ARRAY *array)
{
    assert(size);
    assert(array);
    char key[12];
    int idx = 0;
    *size += sizeof(int32_t); // object size
    for (JSON_ARRAY_ELEMENT *element = array->start; element != NULL;
         element = element->next) {
        sprintf(key, "%d", idx);
        idx++;
        if (!M_GetValueWrappedSize(size, key, element->value)) {
            return false;
        }
    }
    *size += 1; // NULL terminator
    return true;
}

static bool M_GetArrayWrappedSize(
    size_t *size, const char *key, const JSON_ARRAY *array)
{
    assert(size);
    assert(key);
    assert(array);
    if (!M_GetMarkerSize(size, key)) {
        return false;
    }
    if (!M_GetArraySize(size, array)) {
        return false;
    }
    return true;
}

static bool M_GetObjectSize(size_t *size, const JSON_OBJECT *object)
{
    assert(size);
    assert(object);
    *size += sizeof(int32_t); // object size
    for (JSON_OBJECT_ELEMENT *element = object->start; element != NULL;
         element = element->next) {
        if (!M_GetValueWrappedSize(
                size, element->name->string, element->value)) {
            return false;
        }
    }
    *size += 1; // NULL terminator
    return true;
}

static bool M_GetObjectWrappedSize(
    size_t *size, const char *key, const JSON_OBJECT *object)
{
    assert(size);
    assert(key);
    assert(object);
    if (!M_GetMarkerSize(size, key)) {
        return false;
    }
    if (!M_GetObjectSize(size, object)) {
        return false;
    }
    return true;
}

static bool M_GetValueSize(size_t *size, const JSON_VALUE *value)
{
    assert(size);
    assert(value);
    switch (value->type) {
    case JSON_TYPE_ARRAY:
        return M_GetArraySize(size, (JSON_ARRAY *)value->payload);
    case JSON_TYPE_OBJECT:
        return M_GetObjectSize(size, (JSON_OBJECT *)value->payload);
    default:
        LOG_ERROR("Bad BSON root element: %d", value->type);
    }
    return false;
}

static bool M_GetValueWrappedSize(
    size_t *size, const char *key, const JSON_VALUE *value)
{
    assert(size);
    assert(key);
    assert(value);
    switch (value->type) {
    case JSON_TYPE_NULL:
        return M_GetNullWrappedSize(size, key);
    case JSON_TYPE_TRUE:
        return M_GetBoolWrappedSize(size, key);
    case JSON_TYPE_FALSE:
        return M_GetBoolWrappedSize(size, key);
    case JSON_TYPE_NUMBER:
        return M_GetNumberWrappedSize(size, key, (JSON_NUMBER *)value->payload);
    case JSON_TYPE_STRING:
        return M_GetStringWrappedSize(size, key, (JSON_STRING *)value->payload);
    case JSON_TYPE_ARRAY:
        return M_GetArrayWrappedSize(size, key, (JSON_ARRAY *)value->payload);
    case JSON_TYPE_OBJECT:
        return M_GetObjectWrappedSize(size, key, (JSON_OBJECT *)value->payload);
    default:
        LOG_ERROR("Unknown JSON element: %d", value->type);
        return false;
    }
}

static char *M_WriteMarker(char *data, const char *key, const uint8_t marker)
{
    assert(data);
    assert(key);
    *data++ = marker;
    strcpy(data, key);
    data += strlen(key);
    *data++ = '\0';
    return data;
}

static char *M_WriteNullWrapped(char *data, const char *key)
{
    assert(data);
    assert(key);
    return M_WriteMarker(data, key, '\x0A');
}

static char *M_WriteBoolWrapped(char *data, const char *key, bool value)
{
    assert(data);
    assert(key);
    data = M_WriteMarker(data, key, '\x08');
    *(int8_t *)data++ = (int8_t)value;
    return data;
}

static char *M_WriteInt32(char *data, const int32_t value)
{
    assert(data);
    *(int32_t *)data = value;
    data += sizeof(int32_t);
    return data;
}

static char *M_WriteInt32Wrapped(
    char *data, const char *key, const int32_t value)
{
    assert(data);
    assert(key);
    data = M_WriteMarker(data, key, '\x10');
    return M_WriteInt32(data, value);
}

static char *M_WriteDouble(char *data, const double value)
{
    assert(data);
    *(double *)data = value;
    data += sizeof(double);
    return data;
}

static char *M_WriteDoubleWrapped(
    char *data, const char *key, const double value)
{
    assert(data);
    assert(key);
    data = M_WriteMarker(data, key, '\x01');
    return M_WriteDouble(data, value);
}

static char *M_WriteNumberWrapped(
    char *data, const char *key, const JSON_NUMBER *number)
{
    assert(data);
    assert(key);
    assert(number);
    char *str = number->number;

    // hexadecimal numbers
    if (number->number_size >= 2 && (str[1] == 'x' || str[1] == 'X')) {
        return M_WriteInt32Wrapped(
            data, key, json_strtoumax(number->number, NULL, 0));
    }

    // skip leading sign
    if (str[0] == '+' || str[0] == '-') {
        str++;
    }
    assert(str[0]);

    if (!strcmp(str, "Infinity")) {
        // BSON does not support Infinity.
        return M_WriteDoubleWrapped(data, key, DBL_MAX);
    } else if (!strcmp(str, "NaN")) {
        // BSON does not support NaN.
        return M_WriteInt32Wrapped(data, key, 0);
    } else if (strchr(str, '.')) {
        return M_WriteDoubleWrapped(data, key, atof(number->number));
    } else {
        return M_WriteInt32Wrapped(data, key, atoi(number->number));
    }

    return data;
}

static char *M_WriteString(char *data, const JSON_STRING *string)
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

static char *M_WriteStringWrapped(
    char *data, const char *key, const JSON_STRING *string)
{
    assert(data);
    assert(key);
    assert(string);
    data = M_WriteMarker(data, key, '\x02');
    data = M_WriteString(data, string);
    return data;
}

static char *M_WriteArray(char *data, const JSON_ARRAY *array)
{
    assert(data);
    assert(array);
    char key[12];
    int idx = 0;
    char *old = data;
    data += sizeof(int32_t);
    for (JSON_ARRAY_ELEMENT *element = array->start; element != NULL;
         element = element->next) {
        sprintf(key, "%d", idx);
        idx++;
        data = M_WriteValueWrapped(data, key, element->value);
    }
    *data++ = '\0';
    *(int32_t *)old = data - old;
    return data;
}

static char *M_WriteArrayWrapped(
    char *data, const char *key, const JSON_ARRAY *array)
{
    assert(data);
    assert(key);
    assert(array);
    data = M_WriteMarker(data, key, '\x04');
    data = M_WriteArray(data, array);
    return data;
}

static char *M_WriteObject(char *data, const JSON_OBJECT *object)
{
    assert(data);
    assert(object);
    char *old = data;
    data += sizeof(int32_t);
    for (JSON_OBJECT_ELEMENT *element = object->start; element != NULL;
         element = element->next) {
        data = M_WriteValueWrapped(data, element->name->string, element->value);
    }
    *data++ = '\0';
    *(int32_t *)old = data - old;
    return data;
}

static char *M_WriteObjectWrapped(
    char *data, const char *key, const JSON_OBJECT *object)
{
    assert(data);
    assert(key);
    assert(object);
    data = M_WriteMarker(data, key, '\x03');
    data = M_WriteObject(data, object);
    return data;
}

static char *M_WriteValue(char *data, const JSON_VALUE *value)
{
    assert(data);
    assert(value);
    switch (value->type) {
    case JSON_TYPE_ARRAY:
        data = M_WriteArray(data, (JSON_ARRAY *)value->payload);
        break;
    case JSON_TYPE_OBJECT:
        data = M_WriteObject(data, (JSON_OBJECT *)value->payload);
        break;
    default:
        assert(0);
    }
    return data;
}

static char *M_WriteValueWrapped(
    char *data, const char *key, const JSON_VALUE *value)
{
    assert(data);
    assert(key);
    assert(value);
    switch (value->type) {
    case JSON_TYPE_NULL:
        return M_WriteNullWrapped(data, key);
    case JSON_TYPE_TRUE:
        return M_WriteBoolWrapped(data, key, true);
    case JSON_TYPE_FALSE:
        return M_WriteBoolWrapped(data, key, false);
    case JSON_TYPE_NUMBER:
        return M_WriteNumberWrapped(data, key, (JSON_NUMBER *)value->payload);
    case JSON_TYPE_STRING:
        return M_WriteStringWrapped(data, key, (JSON_STRING *)value->payload);
    case JSON_TYPE_ARRAY:
        return M_WriteArrayWrapped(data, key, (JSON_ARRAY *)value->payload);
    case JSON_TYPE_OBJECT:
        return M_WriteObjectWrapped(data, key, (JSON_OBJECT *)value->payload);
    default:
        return NULL;
    }
}

void *BSON_Write(const JSON_VALUE *value, size_t *out_size)
{
    assert(value);
    *out_size = -1;
    if (value == NULL) {
        return NULL;
    }

    size_t size = 0;
    if (!M_GetValueSize(&size, value)) {
        return NULL;
    }

    char *data = Memory_Alloc(size);
    char *data_end = M_WriteValue(data, value);
    assert((size_t)(data_end - data) == size);

    if (out_size != NULL) {
        *out_size = size;
    }

    return data;
}
