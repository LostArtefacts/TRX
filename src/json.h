#pragma once

#define JSON_INVALID_BOOL -1
#define JSON_INVALID_STRING NULL
#define JSON_INVALID_NUMBER 0x7FFFFFFF

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#define json_null NULL
#define json_uintmax_t uintmax_t
#define json_strtoumax strtoumax

enum json_type_e {
    json_type_string,
    json_type_number,
    json_type_object,
    json_type_array,
    json_type_true,
    json_type_false,
    json_type_null
};

struct json_string_s {
    char *string;
    size_t string_size;
    size_t ref_count;
};

struct json_string_ex_s {
    struct json_string_s string;
    size_t offset;
    size_t line_no;
    size_t row_no;
};

struct json_number_s {
    char *number;
    size_t number_size;
    size_t ref_count;
};

struct json_object_element_s {
    struct json_string_s *name;
    struct json_value_s *value;
    struct json_object_element_s *next;
};

struct json_object_s {
    struct json_object_element_s *start;
    size_t length;
    size_t ref_count;
};

struct json_array_element_s {
    struct json_value_s *value;
    struct json_array_element_s *next;
};

struct json_array_s {
    struct json_array_element_s *start;
    size_t length;
    size_t ref_count;
};

struct json_value_s {
    void *payload;
    size_t type;
    size_t ref_count;
};

struct json_value_ex_s {
    struct json_value_s value;
    size_t offset;
    size_t line_no;
    size_t row_no;
};

enum json_parse_error_e {
    json_parse_error_none = 0,
    json_parse_error_expected_comma_or_closing_bracket,
    json_parse_error_expected_colon,
    json_parse_error_expected_opening_quote,
    json_parse_error_invalid_string_escape_sequence,
    json_parse_error_invalid_number_format,
    json_parse_error_invalid_value,
    json_parse_error_premature_end_of_buffer,
    json_parse_error_invalid_string,
    json_parse_error_allocator_failed,
    json_parse_error_unexpected_trailing_characters,
    json_parse_error_unknown
};

struct json_parse_result_s {
    size_t error;
    size_t error_offset;
    size_t error_line_no;
    size_t error_row_no;
};

struct json_parse_state_s {
    const char *src;
    size_t size;
    size_t offset;
    size_t flags_bitset;
    char *data;
    char *dom;
    size_t dom_size;
    size_t data_size;
    size_t line_no;
    size_t line_offset;
    size_t error;
};

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
char *json_write_minified_object(
    const struct json_object_s *object, char *data);
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
const char *json_get_error_description(enum json_parse_error_e error);

// numbers
struct json_number_s *json_number_new_int(int number);
struct json_number_s *json_number_new_double(double number);
void json_number_free(struct json_number_s *num);

// strings
struct json_string_s *json_string_new(const char *string);
void json_string_free(struct json_string_s *str);

// arrays
struct json_array_s *json_array_new();
void json_array_free(struct json_array_s *arr);

void json_array_append(struct json_array_s *arr, struct json_value_s *value);
void json_array_append_bool(struct json_array_s *arr, int b);
void json_array_append_number_int(struct json_array_s *arr, int number);
void json_array_append_number_double(struct json_array_s *arr, double number);
void json_array_append_array(
    struct json_array_s *arr, struct json_array_s *arr2);

struct json_value_s *json_array_get_value(
    struct json_array_s *arr, const size_t idx);
int json_array_get_bool(struct json_array_s *arr, const size_t idx, int d);
int json_array_get_number_int(
    struct json_array_s *arr, const size_t idx, int d);
double json_array_get_number_double(
    struct json_array_s *arr, const size_t idx, double d);
const char *json_array_get_string(
    struct json_array_s *arr, const size_t idx, const char *d);
struct json_array_s *json_array_get_array(
    struct json_array_s *arr, const size_t idx);
struct json_object_s *json_array_get_object(
    struct json_array_s *arr, const size_t idx);

// objects
struct json_object_s *json_object_new();
void json_object_free(struct json_object_s *obj);

void json_object_append(
    struct json_object_s *obj, const char *key, struct json_value_s *value);
void json_object_append_bool(struct json_object_s *obj, const char *key, int b);
void json_object_append_number_int(
    struct json_object_s *obj, const char *key, int number);
void json_object_append_number_double(
    struct json_object_s *obj, const char *key, double number);
void json_object_append_array(
    struct json_object_s *obj, const char *key, struct json_array_s *arr);

struct json_value_s *json_object_get_value(
    struct json_object_s *obj, const char *key);
int json_object_get_bool(struct json_object_s *obj, const char *key, int d);
int json_object_get_number_int(
    struct json_object_s *obj, const char *key, int d);
double json_object_get_number_double(
    struct json_object_s *obj, const char *key, double d);
const char *json_object_get_string(
    struct json_object_s *obj, const char *key, const char *d);
struct json_array_s *json_object_get_array(
    struct json_object_s *obj, const char *key);
struct json_object_s *json_object_get_object(
    struct json_object_s *obj, const char *key);

// values
struct json_string_s *json_value_as_string(struct json_value_s *const value);
struct json_number_s *json_value_as_number(struct json_value_s *const value);
struct json_object_s *json_value_as_object(struct json_value_s *const value);
struct json_array_s *json_value_as_array(struct json_value_s *const value);
int json_value_is_true(const struct json_value_s *const value);
int json_value_is_false(const struct json_value_s *const value);
int json_value_is_null(const struct json_value_s *const value);

struct json_value_s *json_value_from_bool(int b);
struct json_value_s *json_value_from_number(struct json_number_s *num);
struct json_value_s *json_value_from_string(struct json_string_s *str);
struct json_value_s *json_value_from_array(struct json_array_s *arr);
struct json_value_s *json_value_from_object(struct json_object_s *obj);
void json_value_free(struct json_value_s *value);
