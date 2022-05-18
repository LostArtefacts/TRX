#pragma once

#include "json/json_base.h"

#include <stddef.h>

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

const char *json_get_error_description(enum json_parse_error_e error);
