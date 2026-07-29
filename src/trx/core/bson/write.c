#include <trx/core/bson/write.h>

#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/debug.h>

#include <float.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool M_GetValueWrappedSize(
    size_t *size, const char *key, const JSON_VALUE *value);
static char *M_WriteValueWrapped(
    char *data, const char *key, const JSON_VALUE *value);

static bool M_GetMarkerSize(size_t *size, const char *key)
{
    ASSERT(size != nullptr);
    ASSERT(key != nullptr);
    *size += 1; // marker
    *size += strlen(key); // key
    *size += 1; // nullptr terminator
    return true;
}

static bool M_GetNullWrappedSize(size_t *size, const char *key)
{
    ASSERT(size != nullptr);
    ASSERT(key != nullptr);
    return M_GetMarkerSize(size, key);
}

static bool M_GetBoolWrappedSize(size_t *size, const char *key)
{
    ASSERT(size != nullptr);
    ASSERT(key != nullptr);
    if (!M_GetMarkerSize(size, key)) {
        return false;
    }
    *size += 1;
    return true;
}

static bool M_GetInt32Size(size_t *size)
{
    ASSERT(size != nullptr);
    *size += sizeof(int32_t);
    return true;
}

static bool M_GetInt32WrappedSize(size_t *size, const char *key)
{
    ASSERT(size != nullptr);
    ASSERT(key != nullptr);
    if (!M_GetMarkerSize(size, key)) {
        return false;
    }
    if (!M_GetInt32Size(size)) {
        return false;
    }
    return true;
}

static bool M_GetInt64Size(size_t *size)
{
    ASSERT(size != nullptr);
    *size += sizeof(int64_t);
    return true;
}

static bool M_GetInt64WrappedSize(size_t *size, const char *key)
{
    ASSERT(size != nullptr);
    ASSERT(key != nullptr);
    if (!M_GetMarkerSize(size, key)) {
        return false;
    }
    if (!M_GetInt64Size(size)) {
        return false;
    }
    return true;
}

static bool M_GetDoubleSize(size_t *size)
{
    ASSERT(size != nullptr);
    *size += sizeof(double);
    return true;
}

static bool M_GetDoubleWrappedSize(size_t *size, const char *key)
{
    ASSERT(size != nullptr);
    ASSERT(key != nullptr);
    if (!M_GetMarkerSize(size, key)) {
        return false;
    }
    if (!M_GetDoubleSize(size)) {
        return false;
    }
    return true;
}

typedef enum {
    M_NUMBER_INT32,
    M_NUMBER_INT64,
    M_NUMBER_DOUBLE,
} M_NUMBER_KIND;

// A JSON number is a literal, and BSON has three forms to put one in. The
// sizing pass and the writing pass have to land on the same one, so both ask
// here rather than each reading the literal their own way.
static M_NUMBER_KIND M_ClassifyNumber(
    const JSON_NUMBER *const number, int64_t *const int_value,
    double *const dbl_value)
{
    ASSERT(number != nullptr);
    const char *str = number->number;
    ASSERT(str != nullptr);

    // hexadecimal numbers
    if (number->number_size >= 2 && (str[1] == 'x' || str[1] == 'X')) {
        *int_value = (int64_t)json_strtoumax(str, nullptr, 0);
    } else {
        // skip leading sign
        const char *digits = str;
        if (digits[0] == '+' || digits[0] == '-') {
            digits++;
        }
        ASSERT(digits[0] != '\0');

        if (strcmp(digits, "Infinity") == 0) {
            // BSON does not support Infinity.
            *dbl_value = DBL_MAX;
            return M_NUMBER_DOUBLE;
        }
        if (strcmp(digits, "NaN") == 0) {
            // BSON does not support NaN.
            *int_value = 0;
            return M_NUMBER_INT32;
        }
        if (strchr(digits, '.') != nullptr) {
            *dbl_value = atof(str);
            return M_NUMBER_DOUBLE;
        }
        *int_value = strtoll(str, nullptr, 10);
    }

    return *int_value < INT32_MIN || *int_value > INT32_MAX ? M_NUMBER_INT64
                                                            : M_NUMBER_INT32;
}

static bool M_GetNumberWrappedSize(
    size_t *size, const char *key, const JSON_NUMBER *number)
{
    ASSERT(size != nullptr);
    ASSERT(key != nullptr);

    int64_t int_value = 0;
    double dbl_value = 0.0;
    switch (M_ClassifyNumber(number, &int_value, &dbl_value)) {
    case M_NUMBER_INT64:
        return M_GetInt64WrappedSize(size, key);
    case M_NUMBER_DOUBLE:
        return M_GetDoubleWrappedSize(size, key);
    default:
        return M_GetInt32WrappedSize(size, key);
    }
}

static bool M_GetStringSize(size_t *size, const JSON_STRING *string)
{
    ASSERT(size != nullptr);
    ASSERT(string != nullptr);
    *size += sizeof(uint32_t); // size
    *size += string->string_size; // string
    *size += 1; // nullptr terminator
    return true;
}

static bool M_GetStringWrappedSize(
    size_t *size, const char *key, const JSON_STRING *string)
{
    ASSERT(size != nullptr);
    ASSERT(key != nullptr);
    ASSERT(string != nullptr);
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
    ASSERT(size != nullptr);
    ASSERT(array != nullptr);
    char key[12];
    int idx = 0;
    *size += sizeof(int32_t); // object size
    for (JSON_ARRAY_ELEMENT *element = array->start; element != nullptr;
         element = element->next) {
        sprintf(key, "%d", idx);
        idx++;
        if (!M_GetValueWrappedSize(size, key, element->value)) {
            return false;
        }
    }
    *size += 1; // nullptr terminator
    return true;
}

static bool M_GetArrayWrappedSize(
    size_t *size, const char *key, const JSON_ARRAY *array)
{
    ASSERT(size != nullptr);
    ASSERT(key != nullptr);
    ASSERT(array != nullptr);
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
    ASSERT(size != nullptr);
    ASSERT(object != nullptr);
    *size += sizeof(int32_t); // object size
    for (JSON_OBJECT_ELEMENT *element = object->start; element != nullptr;
         element = element->next) {
        if (!M_GetValueWrappedSize(
                size, element->name->string, element->value)) {
            return false;
        }
    }
    *size += 1; // nullptr terminator
    return true;
}

static bool M_GetObjectWrappedSize(
    size_t *size, const char *key, const JSON_OBJECT *object)
{
    ASSERT(size != nullptr);
    ASSERT(key != nullptr);
    ASSERT(object != nullptr);
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
    ASSERT(size != nullptr);
    ASSERT(value != nullptr);
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
    ASSERT(size != nullptr);
    ASSERT(key != nullptr);
    ASSERT(value != nullptr);
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
    ASSERT(data != nullptr);
    ASSERT(key != nullptr);
    *data++ = marker;
    strcpy(data, key);
    data += strlen(key);
    *data++ = '\0';
    return data;
}

static char *M_WriteNullWrapped(char *data, const char *key)
{
    ASSERT(data != nullptr);
    ASSERT(key != nullptr);
    return M_WriteMarker(data, key, '\x0A');
}

static char *M_WriteBoolWrapped(char *data, const char *key, bool value)
{
    ASSERT(data != nullptr);
    ASSERT(key != nullptr);
    data = M_WriteMarker(data, key, '\x08');
    *(int8_t *)data++ = (int8_t)value;
    return data;
}

static char *M_WriteInt32(char *data, const int32_t value)
{
    ASSERT(data != nullptr);
    memcpy(data, &value, sizeof(value));
    data += sizeof(int32_t);
    return data;
}

static char *M_WriteInt32Wrapped(
    char *data, const char *key, const int32_t value)
{
    ASSERT(data != nullptr);
    ASSERT(key != nullptr);
    data = M_WriteMarker(data, key, '\x10');
    return M_WriteInt32(data, value);
}

static char *M_WriteInt64(char *data, const int64_t value)
{
    ASSERT(data != nullptr);
    memcpy(data, &value, sizeof(value));
    data += sizeof(int64_t);
    return data;
}

static char *M_WriteInt64Wrapped(
    char *data, const char *key, const int64_t value)
{
    ASSERT(data != nullptr);
    ASSERT(key != nullptr);
    data = M_WriteMarker(data, key, '\x12');
    return M_WriteInt64(data, value);
}

static char *M_WriteDouble(char *data, const double value)
{
    ASSERT(data != nullptr);
    memcpy(data, &value, sizeof(value));
    data += sizeof(double);
    return data;
}

static char *M_WriteDoubleWrapped(
    char *data, const char *key, const double value)
{
    ASSERT(data != nullptr);
    ASSERT(key != nullptr);
    data = M_WriteMarker(data, key, '\x01');
    return M_WriteDouble(data, value);
}

static char *M_WriteNumberWrapped(
    char *data, const char *key, const JSON_NUMBER *number)
{
    ASSERT(data != nullptr);
    ASSERT(key != nullptr);
    ASSERT(number != nullptr);

    int64_t int_value = 0;
    double dbl_value = 0.0;
    switch (M_ClassifyNumber(number, &int_value, &dbl_value)) {
    case M_NUMBER_INT64:
        return M_WriteInt64Wrapped(data, key, int_value);
    case M_NUMBER_DOUBLE:
        return M_WriteDoubleWrapped(data, key, dbl_value);
    default:
        return M_WriteInt32Wrapped(data, key, (int32_t)int_value);
    }
}

static char *M_WriteString(char *data, const JSON_STRING *string)
{
    ASSERT(data != nullptr);
    ASSERT(string != nullptr);
    const uint32_t bson_string_size = string->string_size + 1;
    memcpy(data, &bson_string_size, sizeof(bson_string_size));
    data += sizeof(uint32_t);
    memcpy(data, string->string, string->string_size);
    data += string->string_size;
    *data++ = '\0';
    return data;
}

static char *M_WriteStringWrapped(
    char *data, const char *key, const JSON_STRING *string)
{
    ASSERT(data != nullptr);
    ASSERT(key != nullptr);
    ASSERT(string != nullptr);
    data = M_WriteMarker(data, key, '\x02');
    data = M_WriteString(data, string);
    return data;
}

static char *M_WriteArray(char *data, const JSON_ARRAY *array)
{
    ASSERT(data != nullptr);
    ASSERT(array != nullptr);
    char key[12];
    int idx = 0;
    char *old = data;
    data += sizeof(int32_t);
    for (JSON_ARRAY_ELEMENT *element = array->start; element != nullptr;
         element = element->next) {
        sprintf(key, "%d", idx);
        idx++;
        data = M_WriteValueWrapped(data, key, element->value);
    }
    *data++ = '\0';
    const int32_t object_size = data - old;
    memcpy(old, &object_size, sizeof(object_size));
    return data;
}

static char *M_WriteArrayWrapped(
    char *data, const char *key, const JSON_ARRAY *array)
{
    ASSERT(data != nullptr);
    ASSERT(key != nullptr);
    ASSERT(array != nullptr);
    data = M_WriteMarker(data, key, '\x04');
    data = M_WriteArray(data, array);
    return data;
}

static char *M_WriteObject(char *data, const JSON_OBJECT *object)
{
    ASSERT(data != nullptr);
    ASSERT(object != nullptr);
    char *old = data;
    data += sizeof(int32_t);
    for (JSON_OBJECT_ELEMENT *element = object->start; element != nullptr;
         element = element->next) {
        data = M_WriteValueWrapped(data, element->name->string, element->value);
    }
    *data++ = '\0';
    const int32_t object_size = data - old;
    memcpy(old, &object_size, sizeof(object_size));
    return data;
}

static char *M_WriteObjectWrapped(
    char *data, const char *key, const JSON_OBJECT *object)
{
    ASSERT(data != nullptr);
    ASSERT(key != nullptr);
    ASSERT(object != nullptr);
    data = M_WriteMarker(data, key, '\x03');
    data = M_WriteObject(data, object);
    return data;
}

static char *M_WriteValue(char *data, const JSON_VALUE *value)
{
    ASSERT(data != nullptr);
    ASSERT(value != nullptr);
    switch (value->type) {
    case JSON_TYPE_ARRAY:
        data = M_WriteArray(data, (JSON_ARRAY *)value->payload);
        break;
    case JSON_TYPE_OBJECT:
        data = M_WriteObject(data, (JSON_OBJECT *)value->payload);
        break;
    default:
        ASSERT_FAIL();
    }
    return data;
}

static char *M_WriteValueWrapped(
    char *data, const char *key, const JSON_VALUE *value)
{
    ASSERT(data != nullptr);
    ASSERT(key != nullptr);
    ASSERT(value != nullptr);
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
        return nullptr;
    }
}

void *BSON_Write(const JSON_VALUE *value, size_t *out_size)
{
    ASSERT(value != nullptr);
    *out_size = -1;
    if (value == nullptr) {
        return nullptr;
    }

    size_t size = 0;
    if (!M_GetValueSize(&size, value)) {
        return nullptr;
    }

    char *data = Memory_Alloc(size);
    char *data_end = M_WriteValue(data, value);
    ASSERT((size_t)(data_end - data) == size);

    if (out_size != nullptr) {
        *out_size = size;
    }

    return data;
}
