/*
   The latest version of this library is available on GitHub;
   https://github.com/sheredom/json.h.
*/

/*
   This is free and unencumbered software released into the public domain.

   Anyone is free to copy, modify, publish, use, compile, sell, or
   distribute this software, either in source code form or as a compiled
   binary, for any purpose, commercial or non-commercial, and by any
   means.

   In jurisdictions that recognize copyright laws, the author or authors
   of this software dedicate any and all copyright interest in the
   software to the public domain. We make this dedication for the benefit
   of the public at large and to the detriment of our heirs and
   successors. We intend this dedication to be an overt act of
   relinquishment in perpetuity of all present and future rights to this
   software under copyright law.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.

   For more information, please refer to <http://unlicense.org/>.
*/

#ifndef SHEREDOM_JSON_H_INCLUDED
#define SHEREDOM_JSON_H_INCLUDED

#if defined(_MSC_VER)
    #pragma warning(push)

    /* disable warning: no function prototype given: converting '()' to '(void)'
     */
    #pragma warning(disable : 4255)

    /* disable warning: '__cplusplus' is not defined as a preprocessor macro,
     * replacing with '0' for '#if/#elif' */
    #pragma warning(disable : 4668)

    /* disable warning: 'bytes padding added after construct' */
    #pragma warning(disable : 4820)
#endif

#include <stddef.h>

struct json_value_s;
struct json_parse_result_s;

enum json_parse_flags_e {
    json_parse_flags_default = 0,

    /* allow trailing commas in objects and arrays. For example, both [true,]
       and
       {"a" : null,} would be allowed with this option on. */
    json_parse_flags_allow_trailing_comma = 0x1,

    /* allow unquoted keys for objects. For example, {a : null} would be allowed
       with this option on. */
    json_parse_flags_allow_unquoted_keys = 0x2,

    /* allow a global unbracketed object. For example, a : null, b : true, c :
       {} would be allowed with this option on. */
    json_parse_flags_allow_global_object = 0x4,

    /* allow objects to use '=' instead of ':' between key/value pairs. For
       example, a = null, b : true would be allowed with this option on. */
    json_parse_flags_allow_equals_in_object = 0x8,

    /* allow that objects don't have to have comma separators between key/value
       pairs. */
    json_parse_flags_allow_no_commas = 0x10,

    /* allow c-style comments (either variants) to be ignored in the input JSON
       file. */
    json_parse_flags_allow_c_style_comments = 0x20,

    /* deprecated flag, unused. */
    json_parse_flags_deprecated = 0x40,

    /* record location information for each value. */
    json_parse_flags_allow_location_information = 0x80,

    /* allow strings to be 'single quoted'. */
    json_parse_flags_allow_single_quoted_strings = 0x100,

    /* allow numbers to be hexadecimal. */
    json_parse_flags_allow_hexadecimal_numbers = 0x200,

    /* allow numbers like +123 to be parsed. */
    json_parse_flags_allow_leading_plus_sign = 0x400,

    /* allow numbers like .0123 or 123. to be parsed. */
    json_parse_flags_allow_leading_or_trailing_decimal_point = 0x800,

    /* allow Infinity, -Infinity, NaN, -NaN. */
    json_parse_flags_allow_inf_and_nan = 0x1000,

    /* allow multi line string values. */
    json_parse_flags_allow_multi_line_strings = 0x2000,

    /* allow simplified JSON to be parsed. Simplified JSON is an enabling of a
       set of other parsing options. */
    json_parse_flags_allow_simplified_json =
        (json_parse_flags_allow_trailing_comma
         | json_parse_flags_allow_unquoted_keys
         | json_parse_flags_allow_global_object
         | json_parse_flags_allow_equals_in_object
         | json_parse_flags_allow_no_commas),

    /* allow JSON5 to be parsed. JSON5 is an enabling of a set of other parsing
       options. */
    json_parse_flags_allow_json5 =
        (json_parse_flags_allow_trailing_comma
         | json_parse_flags_allow_unquoted_keys
         | json_parse_flags_allow_c_style_comments
         | json_parse_flags_allow_single_quoted_strings
         | json_parse_flags_allow_hexadecimal_numbers
         | json_parse_flags_allow_leading_plus_sign
         | json_parse_flags_allow_leading_or_trailing_decimal_point
         | json_parse_flags_allow_inf_and_nan
         | json_parse_flags_allow_multi_line_strings)
};

/* Parse a JSON text file, returning a pointer to the root of the JSON
 * structure. json_parse performs 1 call to malloc for the entire encoding.
 * Returns 0 if an error occurred (malformed JSON input, or malloc failed). */
struct json_value_s *json_parse(const void *src, size_t src_size);

/* Parse a JSON text file, returning a pointer to the root of the JSON
 * structure. json_parse performs 1 call to alloc_func_ptr for the entire
 * encoding. Returns 0 if an error occurred (malformed JSON input, or malloc
 * failed). If an error occurred, the result struct (if not NULL) will explain
 * the type of error, and the location in the input it occurred. If
 * alloc_func_ptr is null then malloc is used. */
struct json_value_s *json_parse_ex(
    const void *src, size_t src_size, size_t flags_bitset,
    void *(*alloc_func_ptr)(void *, size_t), void *user_data,
    struct json_parse_result_s *result);

/* Write out a minified JSON utf-8 string. This string is an encoding of the
 * minimal string characters required to still encode the same data.
 * json_write_minified performs 1 call to malloc for the entire encoding. Return
 * 0 if an error occurred (malformed JSON input, or malloc failed). The out_size
 * parameter is optional as the utf-8 string is null terminated. */
void *json_write_minified(const struct json_value_s *value, size_t *out_size);

/* Write out a pretty JSON utf-8 string. This string is encoded such that the
 * resultant JSON is pretty in that it is easily human readable. The indent and
 * newline parameters allow a user to specify what kind of indentation and
 * newline they want (two spaces / three spaces / tabs? \r, \n, \r\n ?). Both
 * indent and newline can be NULL, indent defaults to two spaces ("  "), and
 * newline defaults to linux newlines ('\n' as the newline character).
 * json_write_pretty performs 1 call to malloc for the entire encoding. Return 0
 * if an error occurred (malformed JSON input, or malloc failed). The out_size
 * parameter is optional as the utf-8 string is null terminated. */
void *json_write_pretty(
    const struct json_value_s *value, const char *indent, const char *newline,
    size_t *out_size);

/* Reinterpret a JSON value as a string. Returns null is the value was not a
 * string. */
struct json_string_s *json_value_as_string(struct json_value_s *const value);

/* Reinterpret a JSON value as a number. Returns null is the value was not a
 * number. */
struct json_number_s *json_value_as_number(struct json_value_s *const value);

/* Reinterpret a JSON value as an object. Returns null is the value was not an
 * object. */
struct json_object_s *json_value_as_object(struct json_value_s *const value);

/* Reinterpret a JSON value as an array. Returns null is the value was not an
 * array. */
struct json_array_s *json_value_as_array(struct json_value_s *const value);

/* Whether the value is true. */
int json_value_is_true(const struct json_value_s *const value);

/* Whether the value is false. */
int json_value_is_false(const struct json_value_s *const value);

/* Whether the value is null. */
int json_value_is_null(const struct json_value_s *const value);

/* The various types JSON values can be. Used to identify what a value is. */
enum json_type_e {
    json_type_string,
    json_type_number,
    json_type_object,
    json_type_array,
    json_type_true,
    json_type_false,
    json_type_null
};

/* A JSON string value. */
struct json_string_s {
    /* utf-8 string */
    const char *string;
    /* The size (in bytes) of the string */
    size_t string_size;
};

/* A JSON string value (extended). */
struct json_string_ex_s {
    /* The JSON string this extends. */
    struct json_string_s string;

    /* The character offset for the value in the JSON input. */
    size_t offset;

    /* The line number for the value in the JSON input. */
    size_t line_no;

    /* The row number for the value in the JSON input, in bytes. */
    size_t row_no;
};

/* A JSON number value. */
struct json_number_s {
    /* ASCII string containing representation of the number. */
    const char *number;
    /* the size (in bytes) of the number. */
    size_t number_size;
};

/* an element of a JSON object. */
struct json_object_element_s {
    /* the name of this element. */
    struct json_string_s *name;
    /* the value of this element. */
    struct json_value_s *value;
    /* the next object element (can be NULL if the last element in the object).
     */
    struct json_object_element_s *next;
};

/* a JSON object value. */
struct json_object_s {
    /* a linked list of the elements in the object. */
    struct json_object_element_s *start;
    /* the number of elements in the object. */
    size_t length;
};

/* an element of a JSON array. */
struct json_array_element_s {
    /* the value of this element. */
    struct json_value_s *value;
    /* the next array element (can be NULL if the last element in the array). */
    struct json_array_element_s *next;
};

/* a JSON array value. */
struct json_array_s {
    /* a linked list of the elements in the array. */
    struct json_array_element_s *start;
    /* the number of elements in the array. */
    size_t length;
};

/* a JSON value. */
struct json_value_s {
    /* a pointer to either a json_string_s, json_number_s, json_object_s, or. */
    /* json_array_s. Should be cast to the appropriate struct type based on
     * what.
     */
    /* the type of this value is. */
    void *payload;
    /* must be one of json_type_e. If type is json_type_true, json_type_false,
     * or.
     */
    /* json_type_null, payload will be NULL. */
    size_t type;
};

/* a JSON value (extended). */
struct json_value_ex_s {
    /* the JSON value this extends. */
    struct json_value_s value;

    /* the character offset for the value in the JSON input. */
    size_t offset;

    /* the line number for the value in the JSON input. */
    size_t line_no;

    /* the row number for the value in the JSON input, in bytes. */
    size_t row_no;
};

/* a parsing error code. */
enum json_parse_error_e {
    /* no error occurred (huzzah!). */
    json_parse_error_none = 0,

    /* expected either a comma or a closing '}' or ']' to close an object or. */
    /* array! */
    json_parse_error_expected_comma_or_closing_bracket,

    /* colon separating name/value pair was missing! */
    json_parse_error_expected_colon,

    /* expected string to begin with '"'! */
    json_parse_error_expected_opening_quote,

    /* invalid escaped sequence in string! */
    json_parse_error_invalid_string_escape_sequence,

    /* invalid number format! */
    json_parse_error_invalid_number_format,

    /* invalid value! */
    json_parse_error_invalid_value,

    /* reached end of buffer before object/array was complete! */
    json_parse_error_premature_end_of_buffer,

    /* string was malformed! */
    json_parse_error_invalid_string,

    /* a call to malloc, or a user provider allocator, failed. */
    json_parse_error_allocator_failed,

    /* the JSON input had unexpected trailing characters that weren't part of
     * the.
     */
    /* JSON value. */
    json_parse_error_unexpected_trailing_characters,

    /* catch-all error for everything else that exploded (real bad chi!). */
    json_parse_error_unknown
};

/* error report from json_parse_ex(). */
struct json_parse_result_s {
    /* the error code (one of json_parse_error_e). */
    size_t error;

    /* the character offset for the error in the JSON input. */
    size_t error_offset;

    /* the line number for the error in the JSON input. */
    size_t error_line_no;

    /* the row number for the error, in bytes. */
    size_t error_row_no;
};

#if defined(_MSC_VER)
    #pragma warning(pop)
#endif

#if defined(_MSC_VER) && (_MSC_VER < 1920)
    #define json_uintmax_t unsigned __int64
#else
    #include <inttypes.h>
    #define json_uintmax_t uintmax_t
#endif

#if defined(_MSC_VER)
    #define json_strtoumax _strtoui64
#else
    #define json_strtoumax strtoumax
#endif

#if defined(__cplusplus) && (__cplusplus >= 201103L)
    #define json_null nullptr
#else
    #define json_null 0
#endif

#if defined(__clang__)
    #pragma clang diagnostic push

    /* we do one big allocation via malloc, then cast aligned slices of this
     * for. */
    /* our structures - we don't have a way to tell the compiler we know what
     * we. */
    /* are doing, so disable the warning instead! */
    #pragma clang diagnostic ignored "-Wcast-align"

    /* We use C style casts everywhere. */
    #pragma clang diagnostic ignored "-Wold-style-cast"

    /* We need long long for strtoull. */
    #pragma clang diagnostic ignored "-Wc++11-long-long"

    /* Who cares if nullptr doesn't work with C++98, we don't use it there! */
    #pragma clang diagnostic ignored "-Wc++98-compat"
    #pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
#elif defined(_MSC_VER)
    #pragma warning(push)

    /* disable 'function selected for inline expansion' warning. */
    #pragma warning(disable : 4711)

    /* disable '#pragma warning: there is no warning number' warning. */
    #pragma warning(disable : 4619)

    /* disable 'warning number not a valid compiler warning' warning. */
    #pragma warning(disable : 4616)

    /* disable 'Compiler will insert Spectre mitigation for memory load if
     * /Qspectre. */
    /* switch specified' warning. */
    #pragma warning(disable : 5045)
#endif

struct json_parse_state_s {
    const char *src;
    size_t size;
    size_t offset;
    size_t flags_bitset;
    char *data;
    char *dom;
    size_t dom_size;
    size_t data_size;
    size_t line_no; /* line counter for error reporting. */
    size_t line_offset; /* (offset-line_offset) is the character number (in
                           bytes). */
    size_t error;
};

int json_hexadecimal_digit(const char c);

int json_hexadecimal_value(
    const char *c, const unsigned long size, unsigned long *result);

int json_skip_whitespace(struct json_parse_state_s *state);

int json_skip_c_style_comments(struct json_parse_state_s *state);

int json_skip_all_skippables(struct json_parse_state_s *state);

int json_get_value_size(struct json_parse_state_s *state, int is_global_object);

int json_get_string_size(struct json_parse_state_s *state, size_t is_key);

int is_valid_unquoted_key_char(const char c);

int json_get_key_size(struct json_parse_state_s *state);

int json_get_object_size(
    struct json_parse_state_s *state, int is_global_object);

int json_get_array_size(struct json_parse_state_s *state);

int json_get_number_size(struct json_parse_state_s *state);

void json_parse_value(
    struct json_parse_state_s *state, int is_global_object,
    struct json_value_s *value);

void json_parse_string(
    struct json_parse_state_s *state, struct json_string_s *string);

void json_parse_key(
    struct json_parse_state_s *state, struct json_string_s *string);

void json_parse_object(
    struct json_parse_state_s *state, int is_global_object,
    struct json_object_s *object);

void json_parse_array(
    struct json_parse_state_s *state, struct json_array_s *array);

void json_parse_number(
    struct json_parse_state_s *state, struct json_number_s *number);

int json_write_minified_get_value_size(
    const struct json_value_s *value, size_t *size);

int json_write_get_number_size(
    const struct json_number_s *number, size_t *size);

int json_write_get_string_size(
    const struct json_string_s *string, size_t *size);

int json_write_minified_get_array_size(
    const struct json_array_s *array, size_t *size);

int json_write_minified_get_object_size(
    const struct json_object_s *object, size_t *size);

char *json_write_minified_value(const struct json_value_s *value, char *data);

char *json_write_number(const struct json_number_s *number, char *data);

char *json_write_string(const struct json_string_s *string, char *data);

char *json_write_minified_array(const struct json_array_s *array, char *data);

char *
json_write_minified_object(const struct json_object_s *object, char *data);

int json_write_pretty_get_value_size(
    const struct json_value_s *value, size_t depth, size_t indent_size,
    size_t newline_size, size_t *size);

int json_write_pretty_get_array_size(
    const struct json_array_s *array, size_t depth, size_t indent_size,
    size_t newline_size, size_t *size);

int json_write_pretty_get_object_size(
    const struct json_object_s *object, size_t depth, size_t indent_size,
    size_t newline_size, size_t *size);

char *json_write_pretty_value(
    const struct json_value_s *value, size_t depth, const char *indent,
    const char *newline, char *data);

char *json_write_pretty_array(
    const struct json_array_s *array, size_t depth, const char *indent,
    const char *newline, char *data);

char *json_write_pretty_object(
    const struct json_object_s *object, size_t depth, const char *indent,
    const char *newline, char *data);

#if defined(__clang__)
    #pragma clang diagnostic pop
#elif defined(_MSC_VER)
    #pragma warning(pop)
#endif

#endif /* SHEREDOM_JSON_H_INCLUDED. */
