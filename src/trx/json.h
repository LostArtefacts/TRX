#pragma once

#define JSON_INVALID_BOOL -1
#define JSON_INVALID_STRING nullptr
#define JSON_INVALID_NUMBER 0x7FFFFFFF

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#define json_uintmax_t uintmax_t
#define json_strtoumax strtoumax

#define JSON_CONST_DISPATCH(arg, ctype, call)                                  \
    _Generic(0 ? (arg) : (void *)1, const void *: (ctype)call, default: call)

typedef enum {
    JSON_TYPE_STRING,
    JSON_TYPE_NUMBER,
    JSON_TYPE_OBJECT,
    JSON_TYPE_ARRAY,
    JSON_TYPE_TRUE,
    JSON_TYPE_FALSE,
    JSON_TYPE_NULL
} JSON_TYPE;

typedef struct {
    void *payload;
    size_t type;
    size_t ref_count;
} JSON_VALUE;

typedef struct {
    char *string;
    size_t string_size;
    size_t ref_count;
} JSON_STRING;

typedef struct {
    JSON_STRING string;
    size_t offset;
    size_t line_no;
    size_t row_no;
} JSON_STRING_EX;

typedef struct {
    char *number;
    size_t number_size;
    size_t ref_count;
} JSON_NUMBER;

typedef struct JSON_OBJECT_ELEMENT {
    JSON_STRING *name;
    JSON_VALUE *value;
    struct JSON_OBJECT_ELEMENT *next;
    size_t ref_count;
} JSON_OBJECT_ELEMENT;

typedef struct {
    JSON_OBJECT_ELEMENT *start;
    size_t length;
    size_t ref_count;
} JSON_OBJECT;

typedef struct JSON_ARRAY_ELEMENT {
    JSON_VALUE *value;
    struct JSON_ARRAY_ELEMENT *next;
    size_t ref_count;
} JSON_ARRAY_ELEMENT;

typedef struct {
    JSON_ARRAY_ELEMENT *start;
    size_t length;
    size_t ref_count;
} JSON_ARRAY;

typedef struct {
    JSON_VALUE value;
    size_t offset;
    size_t line_no;
    size_t row_no;
} JSON_VALUE_EX;

// values
JSON_VALUE *JSON_ValueFromBool(int b);
JSON_VALUE *JSON_ValueFromInt(int number);
JSON_VALUE *JSON_ValueFromInt64(int64_t number);
JSON_VALUE *JSON_ValueFromDouble(double number);
JSON_VALUE *JSON_ValueFromString(const char *string);
JSON_VALUE *JSON_ValueFromArray(JSON_ARRAY *arr);
JSON_VALUE *JSON_ValueFromObject(JSON_OBJECT *obj);
void JSON_ValueFree(JSON_VALUE *value);

bool JSON_ValueIsNull(const JSON_VALUE *value);
bool JSON_ValueIsTrue(const JSON_VALUE *value);
bool JSON_ValueIsFalse(const JSON_VALUE *value);
int JSON_ValueGetBool(const JSON_VALUE *value, int d);
int JSON_ValueGetInt(const JSON_VALUE *value, int d);
int64_t JSON_ValueGetInt64(const JSON_VALUE *value, int64_t d);
double JSON_ValueGetDouble(const JSON_VALUE *value, double d);
const JSON_NUMBER *JSON_ValueGetNumber(const JSON_VALUE *value);
const char *JSON_ValueGetString(const JSON_VALUE *value, const char *d);

JSON_ARRAY *JSON_ValueAsArray_Impl(const JSON_VALUE *value);
#define JSON_ValueAsArray(value)                                               \
    JSON_CONST_DISPATCH(                                                       \
        value, const JSON_ARRAY *, JSON_ValueAsArray_Impl(value))

JSON_OBJECT *JSON_ValueAsObject_Impl(const JSON_VALUE *value);
#define JSON_ValueAsObject(value)                                              \
    JSON_CONST_DISPATCH(                                                       \
        value, const JSON_OBJECT *, JSON_ValueAsObject_Impl(value))

// arrays
JSON_ARRAY *JSON_ArrayNew(void);
void JSON_ArrayFree(JSON_ARRAY *arr);

void JSON_ArrayAppend(JSON_ARRAY *arr, JSON_VALUE *value);
void JSON_ArrayAppendBool(JSON_ARRAY *arr, int b);
void JSON_ArrayAppendInt(JSON_ARRAY *arr, int number);
void JSON_ArrayAppendDouble(JSON_ARRAY *arr, double number);
void JSON_ArrayAppendString(JSON_ARRAY *arr, const char *string);
void JSON_ArrayAppendArray(JSON_ARRAY *arr, JSON_ARRAY *arr2);
void JSON_ArrayAppendObject(JSON_ARRAY *arr, JSON_OBJECT *obj);

JSON_VALUE *JSON_ArrayGetValue(const JSON_ARRAY *arr, size_t idx);
int JSON_ArrayGetBool(const JSON_ARRAY *arr, size_t idx, int d);
int JSON_ArrayGetInt(const JSON_ARRAY *arr, size_t idx, int d);
double JSON_ArrayGetDouble(const JSON_ARRAY *arr, size_t idx, double d);
const char *JSON_ArrayGetString(
    const JSON_ARRAY *arr, size_t idx, const char *d);

JSON_ARRAY *JSON_ArrayGetArray_Impl(const JSON_ARRAY *arr, size_t idx);
#define JSON_ArrayGetArray(value, ...)                                         \
    JSON_CONST_DISPATCH(                                                       \
        value, const JSON_ARRAY *,                                             \
        JSON_ArrayGetArray_Impl(value, __VA_ARGS__))

JSON_OBJECT *JSON_ArrayGetObject_Impl(const JSON_ARRAY *arr, size_t idx);
#define JSON_ArrayGetObject(value, ...)                                        \
    JSON_CONST_DISPATCH(                                                       \
        value, const JSON_ARRAY *,                                             \
        JSON_ArrayGetObject_Impl(value, __VA_ARGS__))

// objects
JSON_OBJECT *JSON_ObjectNew(void);
void JSON_ObjectFree(JSON_OBJECT *obj);

void JSON_ObjectAppend(JSON_OBJECT *obj, const char *key, JSON_VALUE *value);
void JSON_ObjectAppendBool(JSON_OBJECT *obj, const char *key, int b);
void JSON_ObjectAppendInt(JSON_OBJECT *obj, const char *key, int number);
void JSON_ObjectAppendInt64(JSON_OBJECT *obj, const char *key, int64_t number);
void JSON_ObjectAppendDouble(JSON_OBJECT *obj, const char *key, double number);
void JSON_ObjectAppendString(
    JSON_OBJECT *obj, const char *key, const char *string);
void JSON_ObjectAppendArray(JSON_OBJECT *obj, const char *key, JSON_ARRAY *arr);
void JSON_ObjectAppendObject(
    JSON_OBJECT *obj, const char *key, JSON_OBJECT *obj2);

bool JSON_ObjectContainsKey(JSON_OBJECT *obj, const char *key);
void JSON_ObjectEvictKey(JSON_OBJECT *obj, const char *key);
void JSON_ObjectMerge(JSON_OBJECT *root, const JSON_OBJECT *obj);

JSON_VALUE *JSON_ObjectGetValue(const JSON_OBJECT *obj, const char *key);
int JSON_ObjectGetBool(const JSON_OBJECT *obj, const char *key, int d);
int JSON_ObjectGetInt(const JSON_OBJECT *obj, const char *key, int d);
int64_t JSON_ObjectGetInt64(const JSON_OBJECT *obj, const char *key, int64_t d);
double JSON_ObjectGetDouble(const JSON_OBJECT *obj, const char *key, double d);
const char *JSON_ObjectGetString(
    const JSON_OBJECT *obj, const char *key, const char *d);

JSON_ARRAY *JSON_ObjectGetArray_Impl(const JSON_OBJECT *obj, const char *key);
#define JSON_ObjectGetArray(value, ...)                                        \
    JSON_CONST_DISPATCH(                                                       \
        value, const JSON_ARRAY *,                                             \
        JSON_ObjectGetArray_Impl(value, __VA_ARGS__))

JSON_OBJECT *JSON_ObjectGetObject_Impl(const JSON_OBJECT *obj, const char *key);
#define JSON_ObjectGetObject(value, ...)                                       \
    JSON_CONST_DISPATCH(                                                       \
        value, const JSON_OBJECT *,                                            \
        JSON_ObjectGetObject_Impl(value, __VA_ARGS__))

typedef enum {
    JSON_PARSE_ERROR_NONE = 0,
    JSON_PARSE_ERROR_EXPECTED_COMMA_OR_CLOSING_BRACKET,
    JSON_PARSE_ERROR_EXPECTED_COLON,
    JSON_PARSE_ERROR_EXPECTED_OPENING_QUOTE,
    JSON_PARSE_ERROR_INVALID_STRING_ESCAPE_SEQUENCE,
    JSON_PARSE_ERROR_INVALID_NUMBER_FORMAT,
    JSON_PARSE_ERROR_INVALID_VALUE,
    JSON_PARSE_ERROR_PREMATURE_END_OF_BUFFER,
    JSON_PARSE_ERROR_INVALID_STRING,
    JSON_PARSE_ERROR_ALLOCATOR_FAILED,
    JSON_PARSE_ERROR_UNEXPECTED_TRAILING_CHARACTERS,
    JSON_PARSE_ERROR_UNKNOWN
} JSON_PARSE_ERROR;

typedef struct {
    size_t error;
    size_t error_offset;
    size_t error_line_no;
    size_t error_row_no;
} JSON_PARSE_RESULT;

typedef enum {
    JSON_PARSE_FLAGS_DEFAULT = 0,

    /* allow trailing commas in objects and arrays. For example, both [true,]
       and
       {"a" : null,} would be allowed with this option on. */
    JSON_PARSE_FLAGS_ALLOW_TRAILING_COMMA = 0x1,

    /* allow unquoted keys for objects. For example, {a : null} would be allowed
       with this option on. */
    JSON_PARSE_FLAGS_ALLOW_UNQUOTED_KEYS = 0x2,

    /* allow a global unbracketed object. For example, a : null, b : true, c :
       {} would be allowed with this option on. */
    JSON_PARSE_FLAGS_ALLOW_GLOBAL_OBJECT = 0x4,

    /* allow objects to use '=' instead of ':' between key/value pairs. For
       example, a = null, b : true would be allowed with this option on. */
    JSON_PARSE_FLAGS_ALLOW_EQUALS_IN_OBJECT = 0x8,

    /* allow that objects don't have to have comma separators between key/value
       pairs. */
    JSON_PARSE_FLAGS_ALLOW_NO_COMMAS = 0x10,

    /* allow c-style comments (either variants) to be ignored in the input JSON
       file. */
    JSON_PARSE_FLAGS_ALLOW_C_STYLE_COMMENTS = 0x20,

    /* deprecated flag, unused. */
    JSON_PARSE_FLAGS_DEPRECATED = 0x40,

    /* record location information for each value. */
    JSON_PARSE_FLAGS_ALLOW_LOCATION_INFORMATION = 0x80,

    /* allow strings to be 'single quoted'. */
    JSON_PARSE_FLAGS_ALLOW_SINGLE_QUOTED_STRINGS = 0x100,

    /* allow numbers to be binary. */
    JSON_PARSE_FLAGS_ALLOW_BINARY_NUMBERS = 0x4000,

    /* allow numbers to be hexadecimal. */
    JSON_PARSE_FLAGS_ALLOW_HEXADECIMAL_NUMBERS = 0x200,

    /* allow numbers like +123 to be parsed. */
    JSON_PARSE_FLAGS_ALLOW_LEADING_PLUS_SIGN = 0x400,

    /* allow numbers like .0123 or 123. to be parsed. */
    JSON_PARSE_FLAGS_ALLOW_LEADING_OR_TRAILING_DECIMAL_POINT = 0x800,

    /* allow Infinity, -Infinity, NaN, -NaN. */
    JSON_PARSE_FLAGS_ALLOW_INF_AND_NAN = 0x1000,

    /* allow multi line string values. */
    JSON_PARSE_FLAGS_ALLOW_MULTI_LINE_STRINGS = 0x2000,

    /* allow simplified JSON to be parsed. Simplified JSON is an enabling of a
       set of other parsing options. */
    JSON_PARSE_FLAGS_ALLOW_SIMPLIFIED_JSON =
        (JSON_PARSE_FLAGS_ALLOW_TRAILING_COMMA
         | JSON_PARSE_FLAGS_ALLOW_UNQUOTED_KEYS
         | JSON_PARSE_FLAGS_ALLOW_GLOBAL_OBJECT
         | JSON_PARSE_FLAGS_ALLOW_EQUALS_IN_OBJECT
         | JSON_PARSE_FLAGS_ALLOW_NO_COMMAS),

    /* allow JSON5 to be parsed. JSON5 is an enabling of a set of other parsing
       options. */
    JSON_PARSE_FLAGS_ALLOW_JSON5 =
        (JSON_PARSE_FLAGS_ALLOW_TRAILING_COMMA
         | JSON_PARSE_FLAGS_ALLOW_UNQUOTED_KEYS
         | JSON_PARSE_FLAGS_ALLOW_C_STYLE_COMMENTS
         | JSON_PARSE_FLAGS_ALLOW_SINGLE_QUOTED_STRINGS
         | JSON_PARSE_FLAGS_ALLOW_HEXADECIMAL_NUMBERS
         | JSON_PARSE_FLAGS_ALLOW_BINARY_NUMBERS
         | JSON_PARSE_FLAGS_ALLOW_LEADING_PLUS_SIGN
         | JSON_PARSE_FLAGS_ALLOW_LEADING_OR_TRAILING_DECIMAL_POINT
         | JSON_PARSE_FLAGS_ALLOW_INF_AND_NAN
         | JSON_PARSE_FLAGS_ALLOW_MULTI_LINE_STRINGS)
} JSON_PARSE_FLAGS;

/* Parse a JSON text file, returning a pointer to the root of the JSON
 * structure. JSON_Parse performs 1 call to malloc for the entire encoding.
 * Returns 0 if an error occurred (malformed JSON input, or malloc failed). */
JSON_VALUE *JSON_Parse(const void *src, size_t src_size);

/* Parse a JSON text file, returning a pointer to the root of the JSON
 * structure. JSON_Parse performs 1 call to alloc_func_ptr for the entire
 * encoding. Returns 0 if an error occurred (malformed JSON input, or malloc
 * failed). If an error occurred, the result struct (if not nullptr) will
 * explain the type of error, and the location in the input it occurred. If
 * alloc_func_ptr is null then malloc is used. */
JSON_VALUE *JSON_ParseEx(
    const void *src, size_t src_size, size_t flags_bitset,
    void *(*alloc_func_ptr)(void *, size_t), void *user_data,
    JSON_PARSE_RESULT *result);

const char *JSON_GetErrorDescription(JSON_PARSE_ERROR error);

/* Write out a minified JSON utf-8 string. This string is an encoding of the
 * minimal string characters required to still encode the same data.
 * json_write_minified performs 1 call to malloc for the entire encoding. Return
 * 0 if an error occurred (malformed JSON input, or malloc failed). The out_size
 * parameter is optional as the utf-8 string is null terminated. */
void *JSON_WriteMinified(const JSON_VALUE *value, size_t *out_size);

/* Write out a pretty JSON utf-8 string. This string is encoded such that the
 * resultant JSON is pretty in that it is easily human readable. The indent and
 * newline parameters allow a user to specify what kind of indentation and
 * newline they want (two spaces / three spaces / tabs? \r, \n, \r\n ?). Both
 * indent and newline can be nullptr, indent defaults to two spaces ("  "), and
 * newline defaults to linux newlines ('\n' as the newline character).
 * json_write_pretty performs 1 call to malloc for the entire encoding. Return 0
 * if an error occurred (malformed JSON input, or malloc failed). The out_size
 * parameter is optional as the utf-8 string is null terminated. */
void *JSON_WritePretty(
    const JSON_VALUE *value, const char *indent, const char *newline,
    size_t *out_size);
