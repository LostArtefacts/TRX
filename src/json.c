#include "json.h"

#include "global/const.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int json_hexadecimal_digit(const char c)
{
    if ('0' <= c && c <= '9') {
        return c - '0';
    }
    if ('a' <= c && c <= 'f') {
        return c - 'a' + 10;
    }
    if ('A' <= c && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

int json_hexadecimal_value(
    const char *c, const unsigned long size, unsigned long *result)
{
    const char *p;
    int digit;

    if (size > sizeof(unsigned long) * 2) {
        return 0;
    }

    *result = 0;
    for (p = c; (unsigned long)(p - c) < size; ++p) {
        *result <<= 4;
        digit = json_hexadecimal_digit(*p);
        if (digit < 0 || digit > 15) {
            return 0;
        }
        *result |= (unsigned char)digit;
    }
    return 1;
}

int json_skip_whitespace(struct json_parse_state_s *state)
{
    size_t offset = state->offset;
    const size_t size = state->size;
    const char *const src = state->src;

    /* the only valid whitespace according to ECMA-404 is ' ', '\n', '\r' and
     * '\t'. */
    switch (src[offset]) {
    default:
        return 0;
    case ' ':
    case '\r':
    case '\t':
    case '\n':
        break;
    }

    do {
        switch (src[offset]) {
        default:
            /* Update offset. */
            state->offset = offset;
            return 1;
        case ' ':
        case '\r':
        case '\t':
            break;
        case '\n':
            state->line_no++;
            state->line_offset = offset;
            break;
        }

        offset++;
    } while (offset < size);

    /* Update offset. */
    state->offset = offset;
    return 1;
}

int json_skip_c_style_comments(struct json_parse_state_s *state)
{
    /* do we have a comment?. */
    if ('/' == state->src[state->offset]) {
        /* skip '/'. */
        state->offset++;

        if ('/' == state->src[state->offset]) {
            /* we had a comment of the form //. */

            /* skip second '/'. */
            state->offset++;

            while (state->offset < state->size) {
                switch (state->src[state->offset]) {
                default:
                    /* skip the character in the comment. */
                    state->offset++;
                    break;
                case '\n':
                    /* if we have a newline, our comment has ended! Skip the
                     * newline. */
                    state->offset++;

                    /* we entered a newline, so move our line info forward. */
                    state->line_no++;
                    state->line_offset = state->offset;
                    return 1;
                }
            }

            /* we reached the end of the JSON file! */
            return 1;
        } else if ('*' == state->src[state->offset]) {
            /* we had a comment in the C-style long form. */

            /* skip '*'. */
            state->offset++;

            while (state->offset + 1 < state->size) {
                if (('*' == state->src[state->offset])
                    && ('/' == state->src[state->offset + 1])) {
                    /* we reached the end of our comment! */
                    state->offset += 2;
                    return 1;
                } else if ('\n' == state->src[state->offset]) {
                    /* we entered a newline, so move our line info forward. */
                    state->line_no++;
                    state->line_offset = state->offset;
                }

                /* skip character within comment. */
                state->offset++;
            }

            /* Comment wasn't ended correctly which is a failure. */
            return 1;
        }
    }

    /* we didn't have any comment, which is ok too! */
    return 0;
}

int json_skip_all_skippables(struct json_parse_state_s *state)
{
    /* skip all whitespace and other skippables until there are none left. note
     * that the previous version suffered from read past errors should. the
     * stream end on json_skip_c_style_comments eg. '{"a" ' with comments flag.
     */

    int did_consume = 0;
    const size_t size = state->size;

    if (json_parse_flags_allow_c_style_comments & state->flags_bitset) {
        do {
            if (state->offset == size) {
                state->error = json_parse_error_premature_end_of_buffer;
                return 1;
            }

            did_consume = json_skip_whitespace(state);

            /* This should really be checked on access, not in front of every
             * call.
             */
            if (state->offset == size) {
                state->error = json_parse_error_premature_end_of_buffer;
                return 1;
            }

            did_consume |= json_skip_c_style_comments(state);
        } while (0 != did_consume);
    } else {
        do {
            if (state->offset == size) {
                state->error = json_parse_error_premature_end_of_buffer;
                return 1;
            }

            did_consume = json_skip_whitespace(state);
        } while (0 != did_consume);
    }

    if (state->offset == size) {
        state->error = json_parse_error_premature_end_of_buffer;
        return 1;
    }

    return 0;
}

int json_get_string_size(struct json_parse_state_s *state, size_t is_key)
{
    size_t offset = state->offset;
    const size_t size = state->size;
    size_t data_size = 0;
    const char *const src = state->src;
    const int is_single_quote = '\'' == src[offset];
    const char quote_to_use = is_single_quote ? '\'' : '"';
    const size_t flags_bitset = state->flags_bitset;
    unsigned long codepoint;
    unsigned long high_surrogate = 0;

    if ((json_parse_flags_allow_location_information & flags_bitset) != 0
        && is_key != 0) {
        state->dom_size += sizeof(struct json_string_ex_s);
    } else {
        state->dom_size += sizeof(struct json_string_s);
    }

    if ('"' != src[offset]) {
        /* if we are allowed single quoted strings check for that too. */
        if (!((json_parse_flags_allow_single_quoted_strings & flags_bitset)
              && is_single_quote)) {
            state->error = json_parse_error_expected_opening_quote;
            state->offset = offset;
            return 1;
        }
    }

    /* skip leading '"' or '\''. */
    offset++;

    while ((offset < size) && (quote_to_use != src[offset])) {
        /* add space for the character. */
        data_size++;

        switch (src[offset]) {
        case '\0':
        case '\t':
            state->error = json_parse_error_invalid_string;
            state->offset = offset;
            return 1;
        }

        if ('\\' == src[offset]) {
            /* skip reverse solidus character. */
            offset++;

            if (offset == size) {
                state->error = json_parse_error_premature_end_of_buffer;
                state->offset = offset;
                return 1;
            }

            switch (src[offset]) {
            default:
                state->error = json_parse_error_invalid_string_escape_sequence;
                state->offset = offset;
                return 1;
            case '"':
            case '\\':
            case '/':
            case 'b':
            case 'f':
            case 'n':
            case 'r':
            case 't':
                /* all valid characters! */
                offset++;
                break;
            case 'u':
                if (!(offset + 5 < size)) {
                    /* invalid escaped unicode sequence! */
                    state->error =
                        json_parse_error_invalid_string_escape_sequence;
                    state->offset = offset;
                    return 1;
                }

                codepoint = 0;
                if (!json_hexadecimal_value(&src[offset + 1], 4, &codepoint)) {
                    /* escaped unicode sequences must contain 4 hexadecimal
                     * digits! */
                    state->error =
                        json_parse_error_invalid_string_escape_sequence;
                    state->offset = offset;
                    return 1;
                }

                /* Valid sequence!
                 * see: https://en.wikipedia.org/wiki/UTF-8#Invalid_code_points.
                 *      1       7       U + 0000        U + 007F 0xxxxxxx. 2 11
                 * U + 0080        U + 07FF        110xxxxx 10xxxxxx. 3       16
                 * U + 0800        U + FFFF        1110xxxx 10xxxxxx 10xxxxxx.
                 *      4       21      U + 10000       U + 10FFFF      11110xxx
                 * 10xxxxxx        10xxxxxx        10xxxxxx.
                 * Note: the high and low surrogate halves used by UTF-16
                 * (U+D800 through U+DFFF) and code points not encodable by
                 * UTF-16 (those after U+10FFFF) are not legal Unicode values,
                 * and their UTF-8 encoding must be treated as an invalid byte
                 * sequence. */

                if (high_surrogate != 0) {
                    /* we previously read the high half of the \uxxxx\uxxxx
                     * pair, so now we expect the low half. */
                    if (codepoint >= 0xdc00
                        && codepoint <= 0xdfff) { /* low surrogate range. */
                        data_size += 3;
                        high_surrogate = 0;
                    } else {
                        state->error =
                            json_parse_error_invalid_string_escape_sequence;
                        state->offset = offset;
                        return 1;
                    }
                } else if (codepoint <= 0x7f) {
                    data_size += 0;
                } else if (codepoint <= 0x7ff) {
                    data_size += 1;
                } else if (
                    codepoint >= 0xd800 && codepoint <= 0xdbff) { /* high
                                                                     surrogate
                                                                     range.
                                                                   */
                    /* The codepoint is the first half of a "utf-16 surrogate
                     * pair". so we need the other half for it to be valid:
                     * \uHHHH\uLLLL. */
                    if (offset + 11 > size || '\\' != src[offset + 5]
                        || 'u' != src[offset + 6]) {
                        state->error =
                            json_parse_error_invalid_string_escape_sequence;
                        state->offset = offset;
                        return 1;
                    }
                    high_surrogate = codepoint;
                } else if (
                    codepoint >= 0xd800 && codepoint <= 0xdfff) { /* low
                                                                     surrogate
                                                                     range.
                                                                   */
                    /* we did not read the other half before. */
                    state->error =
                        json_parse_error_invalid_string_escape_sequence;
                    state->offset = offset;
                    return 1;
                } else {
                    data_size += 2;
                }
                /* escaped codepoints after 0xffff are supported in json through
                 * utf-16 surrogate pairs: \uD83D\uDD25 for U+1F525. */

                offset += 5;
                break;
            }
        } else if (('\r' == src[offset]) || ('\n' == src[offset])) {
            if (!(json_parse_flags_allow_multi_line_strings & flags_bitset)) {
                /* invalid escaped unicode sequence! */
                state->error = json_parse_error_invalid_string_escape_sequence;
                state->offset = offset;
                return 1;
            }

            offset++;
        } else {
            /* skip character (valid part of sequence). */
            offset++;
        }
    }

    /* If the offset is equal to the size, we had a non-terminated string! */
    if (offset == size) {
        state->error = json_parse_error_premature_end_of_buffer;
        state->offset = offset - 1;
        return 1;
    }

    /* skip trailing '"' or '\''. */
    offset++;

    /* add enough space to store the string. */
    state->data_size += data_size;

    /* one more byte for null terminator ending the string! */
    state->data_size++;

    /* update offset. */
    state->offset = offset;

    return 0;
}

int is_valid_unquoted_key_char(const char c)
{
    return (
        ('0' <= c && c <= '9') || ('a' <= c && c <= 'z')
        || ('A' <= c && c <= 'Z') || ('_' == c));
}

int json_get_key_size(struct json_parse_state_s *state)
{
    const size_t flags_bitset = state->flags_bitset;

    if (json_parse_flags_allow_unquoted_keys & flags_bitset) {
        size_t offset = state->offset;
        const size_t size = state->size;
        const char *const src = state->src;
        size_t data_size = state->data_size;

        /* if we are allowing unquoted keys, first grok for a quote... */
        if ('"' == src[offset]) {
            /* ... if we got a comma, just parse the key as a string as normal.
             */
            return json_get_string_size(state, 1);
        } else if (
            (json_parse_flags_allow_single_quoted_strings & flags_bitset)
            && ('\'' == src[offset])) {
            /* ... if we got a comma, just parse the key as a string as normal.
             */
            return json_get_string_size(state, 1);
        } else {
            while ((offset < size) && is_valid_unquoted_key_char(src[offset])) {
                offset++;
                data_size++;
            }

            /* one more byte for null terminator ending the string! */
            data_size++;

            if (json_parse_flags_allow_location_information & flags_bitset) {
                state->dom_size += sizeof(struct json_string_ex_s);
            } else {
                state->dom_size += sizeof(struct json_string_s);
            }

            /* update offset. */
            state->offset = offset;

            /* update data_size. */
            state->data_size = data_size;

            return 0;
        }
    } else {
        /* we are only allowed to have quoted keys, so just parse a string! */
        return json_get_string_size(state, 1);
    }
}

int json_get_object_size(struct json_parse_state_s *state, int is_global_object)
{
    const size_t flags_bitset = state->flags_bitset;
    const char *const src = state->src;
    const size_t size = state->size;
    size_t elements = 0;
    int allow_comma = 0;
    int found_closing_brace = 0;

    if (is_global_object) {
        /* if we found an opening '{' of an object, we actually have a normal
         * JSON object at the root of the DOM... */
        if (!json_skip_all_skippables(state)
            && '{' == state->src[state->offset]) {
            /* . and we don't actually have a global object after all! */
            is_global_object = 0;
        }
    }

    if (!is_global_object) {
        if ('{' != src[state->offset]) {
            state->error = json_parse_error_unknown;
            return 1;
        }

        /* skip leading '{'. */
        state->offset++;
    }

    state->dom_size += sizeof(struct json_object_s);

    if ((state->offset == size) && !is_global_object) {
        state->error = json_parse_error_premature_end_of_buffer;
        return 1;
    }

    do {
        if (!is_global_object) {
            if (json_skip_all_skippables(state)) {
                state->error = json_parse_error_premature_end_of_buffer;
                return 1;
            }

            if ('}' == src[state->offset]) {
                /* skip trailing '}'. */
                state->offset++;

                found_closing_brace = 1;

                /* finished the object! */
                break;
            }
        } else {
            /* we don't require brackets, so that means the object ends when the
             * input stream ends! */
            if (json_skip_all_skippables(state)) {
                break;
            }
        }

        /* if we parsed at least once element previously, grok for a comma. */
        if (allow_comma) {
            if (',' == src[state->offset]) {
                /* skip comma. */
                state->offset++;
                allow_comma = 0;
            } else if (json_parse_flags_allow_no_commas & flags_bitset) {
                /* we don't require a comma, and we didn't find one, which is
                 * ok! */
                allow_comma = 0;
            } else {
                /* otherwise we are required to have a comma, and we found none.
                 */
                state->error =
                    json_parse_error_expected_comma_or_closing_bracket;
                return 1;
            }

            if (json_parse_flags_allow_trailing_comma & flags_bitset) {
                continue;
            } else {
                if (json_skip_all_skippables(state)) {
                    state->error = json_parse_error_premature_end_of_buffer;
                    return 1;
                }
            }
        }

        if (json_get_key_size(state)) {
            /* key parsing failed! */
            state->error = json_parse_error_invalid_string;
            return 1;
        }

        if (json_skip_all_skippables(state)) {
            state->error = json_parse_error_premature_end_of_buffer;
            return 1;
        }

        if (json_parse_flags_allow_equals_in_object & flags_bitset) {
            const char current = src[state->offset];
            if ((':' != current) && ('=' != current)) {
                state->error = json_parse_error_expected_colon;
                return 1;
            }
        } else {
            if (':' != src[state->offset]) {
                state->error = json_parse_error_expected_colon;
                return 1;
            }
        }

        /* skip colon. */
        state->offset++;

        if (json_skip_all_skippables(state)) {
            state->error = json_parse_error_premature_end_of_buffer;
            return 1;
        }

        if (json_get_value_size(state, /* is_global_object = */ 0)) {
            /* value parsing failed! */
            return 1;
        }

        /* successfully parsed a name/value pair! */
        elements++;
        allow_comma = 1;
    } while (state->offset < size);

    if ((state->offset == size) && !is_global_object && !found_closing_brace) {
        state->error = json_parse_error_premature_end_of_buffer;
        return 1;
    }

    state->dom_size += sizeof(struct json_object_element_s) * elements;

    return 0;
}

int json_get_array_size(struct json_parse_state_s *state)
{
    const size_t flags_bitset = state->flags_bitset;
    size_t elements = 0;
    int allow_comma = 0;
    const char *const src = state->src;
    const size_t size = state->size;

    if ('[' != src[state->offset]) {
        /* expected array to begin with leading '['. */
        state->error = json_parse_error_unknown;
        return 1;
    }

    /* skip leading '['. */
    state->offset++;

    state->dom_size += sizeof(struct json_array_s);

    while (state->offset < size) {
        if (json_skip_all_skippables(state)) {
            state->error = json_parse_error_premature_end_of_buffer;
            return 1;
        }

        if (']' == src[state->offset]) {
            /* skip trailing ']'. */
            state->offset++;

            state->dom_size += sizeof(struct json_array_element_s) * elements;

            /* finished the object! */
            return 0;
        }

        /* if we parsed at least once element previously, grok for a comma. */
        if (allow_comma) {
            if (',' == src[state->offset]) {
                /* skip comma. */
                state->offset++;
                allow_comma = 0;
            } else if (!(json_parse_flags_allow_no_commas & flags_bitset)) {
                state->error =
                    json_parse_error_expected_comma_or_closing_bracket;
                return 1;
            }

            if (json_parse_flags_allow_trailing_comma & flags_bitset) {
                allow_comma = 0;
                continue;
            } else {
                if (json_skip_all_skippables(state)) {
                    state->error = json_parse_error_premature_end_of_buffer;
                    return 1;
                }
            }
        }

        if (json_get_value_size(state, /* is_global_object = */ 0)) {
            /* value parsing failed! */
            return 1;
        }

        /* successfully parsed an array element! */
        elements++;
        allow_comma = 1;
    }

    /* we consumed the entire input before finding the closing ']' of the array!
     */
    state->error = json_parse_error_premature_end_of_buffer;
    return 1;
}

int json_get_number_size(struct json_parse_state_s *state)
{
    const size_t flags_bitset = state->flags_bitset;
    size_t offset = state->offset;
    const size_t size = state->size;
    int had_leading_digits = 0;
    const char *const src = state->src;

    state->dom_size += sizeof(struct json_number_s);

    if ((json_parse_flags_allow_hexadecimal_numbers & flags_bitset)
        && (offset + 1 < size) && ('0' == src[offset])
        && (('x' == src[offset + 1]) || ('X' == src[offset + 1]))) {
        /* skip the leading 0x that identifies a hexadecimal number. */
        offset += 2;

        /* consume hexadecimal digits. */
        while ((offset < size)
               && (('0' <= src[offset] && src[offset] <= '9')
                   || ('a' <= src[offset] && src[offset] <= 'f')
                   || ('A' <= src[offset] && src[offset] <= 'F'))) {
            offset++;
        }
    } else {
        int found_sign = 0;
        int inf_or_nan = 0;

        if ((offset < size)
            && (('-' == src[offset])
                || ((json_parse_flags_allow_leading_plus_sign & flags_bitset)
                    && ('+' == src[offset])))) {
            /* skip valid leading '-' or '+'. */
            offset++;

            found_sign = 1;
        }

        if (json_parse_flags_allow_inf_and_nan & flags_bitset) {
            const char inf[9] = "Infinity";
            const size_t inf_strlen = sizeof(inf) - 1;
            const char nan[4] = "NaN";
            const size_t nan_strlen = sizeof(nan) - 1;

            if (offset + inf_strlen < size) {
                int found = 1;
                size_t i;
                for (i = 0; i < inf_strlen; i++) {
                    if (inf[i] != src[offset + i]) {
                        found = 0;
                        break;
                    }
                }

                if (found) {
                    /* We found our special 'Infinity' keyword! */
                    offset += inf_strlen;

                    inf_or_nan = 1;
                }
            }

            if (offset + nan_strlen < size) {
                int found = 1;
                size_t i;
                for (i = 0; i < nan_strlen; i++) {
                    if (nan[i] != src[offset + i]) {
                        found = 0;
                        break;
                    }
                }

                if (found) {
                    /* We found our special 'NaN' keyword! */
                    offset += nan_strlen;

                    inf_or_nan = 1;
                }
            }
        }

        if (found_sign && !inf_or_nan && (offset < size)
            && !('0' <= src[offset] && src[offset] <= '9')) {
            /* check if we are allowing leading '.'. */
            if (!(json_parse_flags_allow_leading_or_trailing_decimal_point
                  & flags_bitset)
                || ('.' != src[offset])) {
                /* a leading '-' must be immediately followed by any digit! */
                state->error = json_parse_error_invalid_number_format;
                state->offset = offset;
                return 1;
            }
        }

        if ((offset < size) && ('0' == src[offset])) {
            /* skip valid '0'. */
            offset++;

            /* we need to record whether we had any leading digits for checks
             * later.
             */
            had_leading_digits = 1;

            if ((offset < size) && ('0' <= src[offset] && src[offset] <= '9')) {
                /* a leading '0' must not be immediately followed by any digit!
                 */
                state->error = json_parse_error_invalid_number_format;
                state->offset = offset;
                return 1;
            }
        }

        /* the main digits of our number next. */
        while ((offset < size) && ('0' <= src[offset] && src[offset] <= '9')) {
            offset++;

            /* we need to record whether we had any leading digits for checks
             * later.
             */
            had_leading_digits = 1;
        }

        if ((offset < size) && ('.' == src[offset])) {
            offset++;

            if (!('0' <= src[offset] && src[offset] <= '9')) {
                if (!(json_parse_flags_allow_leading_or_trailing_decimal_point
                      & flags_bitset)
                    || !had_leading_digits) {
                    /* a decimal point must be followed by at least one digit.
                     */
                    state->error = json_parse_error_invalid_number_format;
                    state->offset = offset;
                    return 1;
                }
            }

            /* a decimal point can be followed by more digits of course! */
            while ((offset < size)
                   && ('0' <= src[offset] && src[offset] <= '9')) {
                offset++;
            }
        }

        if ((offset < size) && ('e' == src[offset] || 'E' == src[offset])) {
            /* our number has an exponent! Skip 'e' or 'E'. */
            offset++;

            if ((offset < size) && ('-' == src[offset] || '+' == src[offset])) {
                /* skip optional '-' or '+'. */
                offset++;
            }

            if ((offset < size)
                && !('0' <= src[offset] && src[offset] <= '9')) {
                /* an exponent must have at least one digit! */
                state->error = json_parse_error_invalid_number_format;
                state->offset = offset;
                return 1;
            }

            /* consume exponent digits. */
            do {
                offset++;
            } while ((offset < size)
                     && ('0' <= src[offset] && src[offset] <= '9'));
        }
    }

    if (offset < size) {
        switch (src[offset]) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case '}':
        case ',':
        case ']':
            /* all of the above are ok. */
            break;
        case '=':
            if (json_parse_flags_allow_equals_in_object & flags_bitset) {
                break;
            }

            state->error = json_parse_error_invalid_number_format;
            state->offset = offset;
            return 1;
        default:
            state->error = json_parse_error_invalid_number_format;
            state->offset = offset;
            return 1;
        }
    }

    state->data_size += offset - state->offset;

    /* one more byte for null terminator ending the number string! */
    state->data_size++;

    /* update offset. */
    state->offset = offset;

    return 0;
}

int json_get_value_size(struct json_parse_state_s *state, int is_global_object)
{
    const size_t flags_bitset = state->flags_bitset;
    const char *const src = state->src;
    size_t offset;
    const size_t size = state->size;

    if (json_parse_flags_allow_location_information & flags_bitset) {
        state->dom_size += sizeof(struct json_value_ex_s);
    } else {
        state->dom_size += sizeof(struct json_value_s);
    }

    if (is_global_object) {
        return json_get_object_size(state, /* is_global_object = */ 1);
    } else {
        if (json_skip_all_skippables(state)) {
            state->error = json_parse_error_premature_end_of_buffer;
            return 1;
        }

        /* can cache offset now. */
        offset = state->offset;

        switch (src[offset]) {
        case '"':
            return json_get_string_size(state, 0);
        case '\'':
            if (json_parse_flags_allow_single_quoted_strings & flags_bitset) {
                return json_get_string_size(state, 0);
            } else {
                /* invalid value! */
                state->error = json_parse_error_invalid_value;
                return 1;
            }
        case '{':
            return json_get_object_size(state, /* is_global_object = */ 0);
        case '[':
            return json_get_array_size(state);
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return json_get_number_size(state);
        case '+':
            if (json_parse_flags_allow_leading_plus_sign & flags_bitset) {
                return json_get_number_size(state);
            } else {
                /* invalid value! */
                state->error = json_parse_error_invalid_number_format;
                return 1;
            }
        case '.':
            if (json_parse_flags_allow_leading_or_trailing_decimal_point
                & flags_bitset) {
                return json_get_number_size(state);
            } else {
                /* invalid value! */
                state->error = json_parse_error_invalid_number_format;
                return 1;
            }
        default:
            if ((offset + 4) <= size && 't' == src[offset + 0]
                && 'r' == src[offset + 1] && 'u' == src[offset + 2]
                && 'e' == src[offset + 3]) {
                state->offset += 4;
                return 0;
            } else if (
                (offset + 5) <= size && 'f' == src[offset + 0]
                && 'a' == src[offset + 1] && 'l' == src[offset + 2]
                && 's' == src[offset + 3] && 'e' == src[offset + 4]) {
                state->offset += 5;
                return 0;
            } else if (
                (offset + 4) <= size && 'n' == state->src[offset + 0]
                && 'u' == state->src[offset + 1]
                && 'l' == state->src[offset + 2]
                && 'l' == state->src[offset + 3]) {
                state->offset += 4;
                return 0;
            } else if (
                (json_parse_flags_allow_inf_and_nan & flags_bitset)
                && (offset + 3) <= size && 'N' == src[offset + 0]
                && 'a' == src[offset + 1] && 'N' == src[offset + 2]) {
                return json_get_number_size(state);
            } else if (
                (json_parse_flags_allow_inf_and_nan & flags_bitset)
                && (offset + 8) <= size && 'I' == src[offset + 0]
                && 'n' == src[offset + 1] && 'f' == src[offset + 2]
                && 'i' == src[offset + 3] && 'n' == src[offset + 4]
                && 'i' == src[offset + 5] && 't' == src[offset + 6]
                && 'y' == src[offset + 7]) {
                return json_get_number_size(state);
            }

            /* invalid value! */
            state->error = json_parse_error_invalid_value;
            return 1;
        }
    }
}

void json_parse_string(
    struct json_parse_state_s *state, struct json_string_s *string)
{
    size_t offset = state->offset;
    size_t bytes_written = 0;
    const char *const src = state->src;
    const char quote_to_use = '\'' == src[offset] ? '\'' : '"';
    char *data = state->data;
    unsigned long high_surrogate = 0;
    unsigned long codepoint;

    string->ref_count = 1;
    string->string = data;

    /* skip leading '"' or '\''. */
    offset++;

    while (quote_to_use != src[offset]) {
        if ('\\' == src[offset]) {
            /* skip the reverse solidus. */
            offset++;

            switch (src[offset++]) {
            default:
                return; /* we cannot ever reach here. */
            case 'u': {
                codepoint = 0;
                if (!json_hexadecimal_value(&src[offset], 4, &codepoint)) {
                    return; /* this shouldn't happen as the value was already
                             * validated.
                             */
                }

                offset += 4;

                if (codepoint <= 0x7fu) {
                    data[bytes_written++] = (char)codepoint; /* 0xxxxxxx. */
                } else if (codepoint <= 0x7ffu) {
                    data[bytes_written++] =
                        (char)(0xc0u | (codepoint >> 6)); /* 110xxxxx. */
                    data[bytes_written++] =
                        (char)(0x80u | (codepoint & 0x3fu)); /* 10xxxxxx. */
                } else if (
                    codepoint >= 0xd800 && codepoint <= 0xdbff) { /* high
                                                                     surrogate.
                                                                   */
                    high_surrogate = codepoint;
                    continue; /* we need the low half to form a complete
                                 codepoint. */
                } else if (
                    codepoint >= 0xdc00 && codepoint <= 0xdfff) { /* low
                                                                     surrogate.
                                                                   */
                    /* combine with the previously read half to obtain the
                     * complete codepoint. */
                    const unsigned long surrogate_offset =
                        0x10000u - (0xD800u << 10) - 0xDC00u;
                    codepoint =
                        (high_surrogate << 10) + codepoint + surrogate_offset;
                    high_surrogate = 0;
                    data[bytes_written++] =
                        (char)(0xF0u | (codepoint >> 18)); /* 11110xxx. */
                    data[bytes_written++] =
                        (char)(0x80u | ((codepoint >> 12) & 0x3fu)); /* 10xxxxxx.
                                                                      */
                    data[bytes_written++] =
                        (char)(0x80u | ((codepoint >> 6) & 0x3fu)); /* 10xxxxxx.
                                                                     */
                    data[bytes_written++] =
                        (char)(0x80u | (codepoint & 0x3fu)); /* 10xxxxxx. */
                } else {
                    /* we assume the value was validated and thus is within the
                     * valid range. */
                    data[bytes_written++] =
                        (char)(0xe0u | (codepoint >> 12)); /* 1110xxxx. */
                    data[bytes_written++] =
                        (char)(0x80u | ((codepoint >> 6) & 0x3fu)); /* 10xxxxxx.
                                                                     */
                    data[bytes_written++] =
                        (char)(0x80u | (codepoint & 0x3fu)); /* 10xxxxxx. */
                }
            } break;
            case '"':
                data[bytes_written++] = '"';
                break;
            case '\\':
                data[bytes_written++] = '\\';
                break;
            case '/':
                data[bytes_written++] = '/';
                break;
            case 'b':
                data[bytes_written++] = '\b';
                break;
            case 'f':
                data[bytes_written++] = '\f';
                break;
            case 'n':
                data[bytes_written++] = '\n';
                break;
            case 'r':
                data[bytes_written++] = '\r';
                break;
            case 't':
                data[bytes_written++] = '\t';
                break;
            case '\r':
                data[bytes_written++] = '\r';

                /* check if we have a "\r\n" sequence. */
                if ('\n' == src[offset]) {
                    data[bytes_written++] = '\n';
                    offset++;
                }

                break;
            case '\n':
                data[bytes_written++] = '\n';
                break;
            }
        } else {
            /* copy the character. */
            data[bytes_written++] = src[offset++];
        }
    }

    /* skip trailing '"' or '\''. */
    offset++;

    /* record the size of the string. */
    string->string_size = bytes_written;

    /* add null terminator to string. */
    data[bytes_written++] = '\0';

    /* move data along. */
    state->data += bytes_written;

    /* update offset. */
    state->offset = offset;
}

void json_parse_key(
    struct json_parse_state_s *state, struct json_string_s *string)
{
    if (json_parse_flags_allow_unquoted_keys & state->flags_bitset) {
        const char *const src = state->src;
        char *const data = state->data;
        size_t offset = state->offset;

        /* if we are allowing unquoted keys, check for quoted anyway... */
        if (('"' == src[offset]) || ('\'' == src[offset])) {
            /* ... if we got a quote, just parse the key as a string as normal.
             */
            json_parse_string(state, string);
        } else {
            size_t size = 0;

            string->ref_count = 1;
            string->string = state->data;

            while (is_valid_unquoted_key_char(src[offset])) {
                data[size++] = src[offset++];
            }

            /* add null terminator to string. */
            data[size] = '\0';

            /* record the size of the string. */
            string->string_size = size++;

            /* move data along. */
            state->data += size;

            /* update offset. */
            state->offset = offset;
        }
    } else {
        /* we are only allowed to have quoted keys, so just parse a string! */
        json_parse_string(state, string);
    }
}

void json_parse_object(
    struct json_parse_state_s *state, int is_global_object,
    struct json_object_s *object)
{
    const size_t flags_bitset = state->flags_bitset;
    const size_t size = state->size;
    const char *const src = state->src;
    size_t elements = 0;
    int allow_comma = 0;
    struct json_object_element_s *previous = json_null;

    if (is_global_object) {
        /* if we skipped some whitespace, and then found an opening '{' of an.
         */
        /* object, we actually have a normal JSON object at the root of the
         * DOM...
         */
        if ('{' == src[state->offset]) {
            /* . and we don't actually have a global object after all! */
            is_global_object = 0;
        }
    }

    if (!is_global_object) {
        /* skip leading '{'. */
        state->offset++;
    }

    (void)json_skip_all_skippables(state);

    /* reset elements. */
    elements = 0;

    while (state->offset < size) {
        struct json_object_element_s *element = json_null;
        struct json_string_s *string = json_null;
        struct json_value_s *value = json_null;

        if (!is_global_object) {
            (void)json_skip_all_skippables(state);

            if ('}' == src[state->offset]) {
                /* skip trailing '}'. */
                state->offset++;

                /* finished the object! */
                break;
            }
        } else {
            if (json_skip_all_skippables(state)) {
                /* global object ends when the file ends! */
                break;
            }
        }

        /* if we parsed at least one element previously, grok for a comma. */
        if (allow_comma) {
            if (',' == src[state->offset]) {
                /* skip comma. */
                state->offset++;
                allow_comma = 0;
                continue;
            }
        }

        element = (struct json_object_element_s *)state->dom;

        state->dom += sizeof(struct json_object_element_s);

        if (json_null == previous) {
            /* this is our first element, so record it in our object. */
            object->start = element;
        } else {
            previous->next = element;
        }

        previous = element;

        if (json_parse_flags_allow_location_information & flags_bitset) {
            struct json_string_ex_s *string_ex =
                (struct json_string_ex_s *)state->dom;
            state->dom += sizeof(struct json_string_ex_s);

            string_ex->offset = state->offset;
            string_ex->line_no = state->line_no;
            string_ex->row_no = state->offset - state->line_offset;

            string = &(string_ex->string);
        } else {
            string = (struct json_string_s *)state->dom;
            state->dom += sizeof(struct json_string_s);
        }

        element->name = string;

        (void)json_parse_key(state, string);

        (void)json_skip_all_skippables(state);

        /* skip colon or equals. */
        state->offset++;

        (void)json_skip_all_skippables(state);

        if (json_parse_flags_allow_location_information & flags_bitset) {
            struct json_value_ex_s *value_ex =
                (struct json_value_ex_s *)state->dom;
            state->dom += sizeof(struct json_value_ex_s);

            value_ex->offset = state->offset;
            value_ex->line_no = state->line_no;
            value_ex->row_no = state->offset - state->line_offset;

            value = &(value_ex->value);
        } else {
            value = (struct json_value_s *)state->dom;
            state->dom += sizeof(struct json_value_s);
        }

        element->value = value;

        json_parse_value(state, /* is_global_object = */ 0, value);

        /* successfully parsed a name/value pair! */
        elements++;
        allow_comma = 1;
    }

    /* if we had at least one element, end the linked list. */
    if (previous) {
        previous->next = json_null;
    }

    if (0 == elements) {
        object->start = json_null;
    }

    object->ref_count = 1;
    object->length = elements;
}

void json_parse_array(
    struct json_parse_state_s *state, struct json_array_s *array)
{
    const char *const src = state->src;
    const size_t size = state->size;
    size_t elements = 0;
    int allow_comma = 0;
    struct json_array_element_s *previous = json_null;

    /* skip leading '['. */
    state->offset++;

    (void)json_skip_all_skippables(state);

    /* reset elements. */
    elements = 0;

    do {
        struct json_array_element_s *element = json_null;
        struct json_value_s *value = json_null;

        (void)json_skip_all_skippables(state);

        if (']' == src[state->offset]) {
            /* skip trailing ']'. */
            state->offset++;

            /* finished the array! */
            break;
        }

        /* if we parsed at least one element previously, grok for a comma. */
        if (allow_comma) {
            if (',' == src[state->offset]) {
                /* skip comma. */
                state->offset++;
                allow_comma = 0;
                continue;
            }
        }

        element = (struct json_array_element_s *)state->dom;

        state->dom += sizeof(struct json_array_element_s);

        if (json_null == previous) {
            /* this is our first element, so record it in our array. */
            array->start = element;
        } else {
            previous->next = element;
        }

        previous = element;

        if (json_parse_flags_allow_location_information & state->flags_bitset) {
            struct json_value_ex_s *value_ex =
                (struct json_value_ex_s *)state->dom;
            state->dom += sizeof(struct json_value_ex_s);

            value_ex->offset = state->offset;
            value_ex->line_no = state->line_no;
            value_ex->row_no = state->offset - state->line_offset;

            value = &(value_ex->value);
        } else {
            value = (struct json_value_s *)state->dom;
            state->dom += sizeof(struct json_value_s);
        }

        element->value = value;

        json_parse_value(state, /* is_global_object = */ 0, value);

        /* successfully parsed an array element! */
        elements++;
        allow_comma = 1;
    } while (state->offset < size);

    /* end the linked list. */
    if (previous) {
        previous->next = json_null;
    }

    if (0 == elements) {
        array->start = json_null;
    }

    array->ref_count = 1;
    array->length = elements;
}

void json_parse_number(
    struct json_parse_state_s *state, struct json_number_s *number)
{
    const size_t flags_bitset = state->flags_bitset;
    size_t offset = state->offset;
    const size_t size = state->size;
    size_t bytes_written = 0;
    const char *const src = state->src;
    char *data = state->data;

    number->ref_count = 1;
    number->number = data;

    if (json_parse_flags_allow_hexadecimal_numbers & flags_bitset) {
        if (('0' == src[offset])
            && (('x' == src[offset + 1]) || ('X' == src[offset + 1]))) {
            /* consume hexadecimal digits. */
            while ((offset < size)
                   && (('0' <= src[offset] && src[offset] <= '9')
                       || ('a' <= src[offset] && src[offset] <= 'f')
                       || ('A' <= src[offset] && src[offset] <= 'F')
                       || ('x' == src[offset]) || ('X' == src[offset]))) {
                data[bytes_written++] = src[offset++];
            }
        }
    }

    while (offset < size) {
        int end = 0;

        switch (src[offset]) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '.':
        case 'e':
        case 'E':
        case '+':
        case '-':
            data[bytes_written++] = src[offset++];
            break;
        default:
            end = 1;
            break;
        }

        if (0 != end) {
            break;
        }
    }

    if (json_parse_flags_allow_inf_and_nan & flags_bitset) {
        const size_t inf_strlen = 8; /* = strlen("Infinity");. */
        const size_t nan_strlen = 3; /* = strlen("NaN");. */

        if (offset + inf_strlen < size) {
            if ('I' == src[offset]) {
                size_t i;
                /* We found our special 'Infinity' keyword! */
                for (i = 0; i < inf_strlen; i++) {
                    data[bytes_written++] = src[offset++];
                }
            }
        }

        if (offset + nan_strlen < size) {
            if ('N' == src[offset]) {
                size_t i;
                /* We found our special 'NaN' keyword! */
                for (i = 0; i < nan_strlen; i++) {
                    data[bytes_written++] = src[offset++];
                }
            }
        }
    }

    /* record the size of the number. */
    number->number_size = bytes_written;
    /* add null terminator to number string. */
    data[bytes_written++] = '\0';
    /* move data along. */
    state->data += bytes_written;
    /* update offset. */
    state->offset = offset;
}

void json_parse_value(
    struct json_parse_state_s *state, int is_global_object,
    struct json_value_s *value)
{
    const size_t flags_bitset = state->flags_bitset;
    const char *const src = state->src;
    const size_t size = state->size;
    size_t offset;

    (void)json_skip_all_skippables(state);

    /* cache offset now. */
    offset = state->offset;

    if (is_global_object) {
        value->type = json_type_object;
        value->payload = state->dom;
        state->dom += sizeof(struct json_object_s);
        json_parse_object(
            state, /* is_global_object = */ 1,
            (struct json_object_s *)value->payload);
    } else {
        value->ref_count = 1;
        switch (src[offset]) {
        case '"':
        case '\'':
            value->type = json_type_string;
            value->payload = state->dom;
            state->dom += sizeof(struct json_string_s);
            json_parse_string(state, (struct json_string_s *)value->payload);
            break;
        case '{':
            value->type = json_type_object;
            value->payload = state->dom;
            state->dom += sizeof(struct json_object_s);
            json_parse_object(
                state, /* is_global_object = */ 0,
                (struct json_object_s *)value->payload);
            break;
        case '[':
            value->type = json_type_array;
            value->payload = state->dom;
            state->dom += sizeof(struct json_array_s);
            json_parse_array(state, (struct json_array_s *)value->payload);
            break;
        case '-':
        case '+':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '.':
            value->type = json_type_number;
            value->payload = state->dom;
            state->dom += sizeof(struct json_number_s);
            json_parse_number(state, (struct json_number_s *)value->payload);
            break;
        default:
            if ((offset + 4) <= size && 't' == src[offset + 0]
                && 'r' == src[offset + 1] && 'u' == src[offset + 2]
                && 'e' == src[offset + 3]) {
                value->type = json_type_true;
                value->payload = json_null;
                state->offset += 4;
            } else if (
                (offset + 5) <= size && 'f' == src[offset + 0]
                && 'a' == src[offset + 1] && 'l' == src[offset + 2]
                && 's' == src[offset + 3] && 'e' == src[offset + 4]) {
                value->type = json_type_false;
                value->payload = json_null;
                state->offset += 5;
            } else if (
                (offset + 4) <= size && 'n' == src[offset + 0]
                && 'u' == src[offset + 1] && 'l' == src[offset + 2]
                && 'l' == src[offset + 3]) {
                value->type = json_type_null;
                value->payload = json_null;
                state->offset += 4;
            } else if (
                (json_parse_flags_allow_inf_and_nan & flags_bitset)
                && (offset + 3) <= size && 'N' == src[offset + 0]
                && 'a' == src[offset + 1] && 'N' == src[offset + 2]) {
                value->type = json_type_number;
                value->payload = state->dom;
                state->dom += sizeof(struct json_number_s);
                json_parse_number(
                    state, (struct json_number_s *)value->payload);
            } else if (
                (json_parse_flags_allow_inf_and_nan & flags_bitset)
                && (offset + 8) <= size && 'I' == src[offset + 0]
                && 'n' == src[offset + 1] && 'f' == src[offset + 2]
                && 'i' == src[offset + 3] && 'n' == src[offset + 4]
                && 'i' == src[offset + 5] && 't' == src[offset + 6]
                && 'y' == src[offset + 7]) {
                value->type = json_type_number;
                value->payload = state->dom;
                state->dom += sizeof(struct json_number_s);
                json_parse_number(
                    state, (struct json_number_s *)value->payload);
            }
            break;
        }
    }
}

struct json_value_s *json_parse(const void *src, size_t src_size)
{
    return json_parse_ex(
        src, src_size, json_parse_flags_default, json_null, json_null,
        json_null);
}

struct json_value_s *json_parse_ex(
    const void *src, size_t src_size, size_t flags_bitset,
    void *(*alloc_func_ptr)(void *user_data, size_t size), void *user_data,
    struct json_parse_result_s *result)
{
    struct json_parse_state_s state;
    void *allocation;
    struct json_value_s *value;
    size_t total_size;
    int input_error;

    if (result) {
        result->error = json_parse_error_none;
        result->error_offset = 0;
        result->error_line_no = 0;
        result->error_row_no = 0;
    }

    if (json_null == src) {
        /* invalid src pointer was null! */
        return json_null;
    }

    state.src = (const char *)src;
    state.size = src_size;
    state.offset = 0;
    state.line_no = 1;
    state.line_offset = 0;
    state.error = json_parse_error_none;
    state.dom_size = 0;
    state.data_size = 0;
    state.flags_bitset = flags_bitset;

    input_error = json_get_value_size(
        &state,
        (int)(json_parse_flags_allow_global_object & state.flags_bitset));

    if (0 == input_error) {
        json_skip_all_skippables(&state);

        if (state.offset != state.size) {
            /* our parsing didn't have an error, but there are characters
             * remaining in the input that weren't part of the JSON! */

            state.error = json_parse_error_unexpected_trailing_characters;
            input_error = 1;
        }
    }

    if (input_error) {
        /* parsing value's size failed (most likely an invalid JSON DOM!). */
        if (result) {
            result->error = state.error;
            result->error_offset = state.offset;
            result->error_line_no = state.line_no;
            result->error_row_no = state.offset - state.line_offset;
        }
        return json_null;
    }

    /* our total allocation is the combination of the dom and data sizes (we. */
    /* first encode the structure of the JSON, and then the data referenced by.
     */
    /* the JSON values). */
    total_size = state.dom_size + state.data_size;

    if (json_null == alloc_func_ptr) {
        allocation = malloc(total_size);
    } else {
        allocation = alloc_func_ptr(user_data, total_size);
    }

    if (json_null == allocation) {
        /* malloc failed! */
        if (result) {
            result->error = json_parse_error_allocator_failed;
            result->error_offset = 0;
            result->error_line_no = 0;
            result->error_row_no = 0;
        }

        return json_null;
    }

    /* reset offset so we can reuse it. */
    state.offset = 0;

    /* reset the line information so we can reuse it. */
    state.line_no = 1;
    state.line_offset = 0;

    state.dom = (char *)allocation;
    state.data = state.dom + state.dom_size;

    if (json_parse_flags_allow_location_information & state.flags_bitset) {
        struct json_value_ex_s *value_ex = (struct json_value_ex_s *)state.dom;
        state.dom += sizeof(struct json_value_ex_s);

        value_ex->offset = state.offset;
        value_ex->line_no = state.line_no;
        value_ex->row_no = state.offset - state.line_offset;

        value = &(value_ex->value);
    } else {
        value = (struct json_value_s *)state.dom;
        state.dom += sizeof(struct json_value_s);
    }

    json_parse_value(
        &state,
        (int)(json_parse_flags_allow_global_object & state.flags_bitset),
        value);

    ((struct json_value_s *)allocation)->ref_count = 0;

    return (struct json_value_s *)allocation;
}

struct json_string_s *json_value_as_string(struct json_value_s *const value)
{
    if (!value || value->type != json_type_string) {
        return json_null;
    }

    return (struct json_string_s *)value->payload;
}

struct json_number_s *json_value_as_number(struct json_value_s *const value)
{
    if (!value || value->type != json_type_number) {
        return json_null;
    }

    return (struct json_number_s *)value->payload;
}

struct json_object_s *json_value_as_object(struct json_value_s *const value)
{
    if (!value || value->type != json_type_object) {
        return json_null;
    }

    return (struct json_object_s *)value->payload;
}

struct json_array_s *json_value_as_array(struct json_value_s *const value)
{
    if (!value || value->type != json_type_array) {
        return json_null;
    }

    return (struct json_array_s *)value->payload;
}

int json_value_is_true(const struct json_value_s *const value)
{
    return value && value->type == json_type_true;
}

int json_value_is_false(const struct json_value_s *const value)
{
    return value && value->type == json_type_false;
}

int json_value_is_null(const struct json_value_s *const value)
{
    return value && value->type == json_type_null;
}

int json_write_get_number_size(const struct json_number_s *number, size_t *size)
{
    json_uintmax_t parsed_number;
    size_t i;

    if (number->number_size >= 2) {
        switch (number->number[1]) {
        case 'x':
        case 'X':
            /* the number is a json_parse_flags_allow_hexadecimal_numbers
             * hexadecimal so we have to do extra work to convert it to a
             * non-hexadecimal for JSON output. */
            parsed_number = json_strtoumax(number->number, json_null, 0);

            i = 0;

            while (0 != parsed_number) {
                parsed_number /= 10;
                i++;
            }

            *size += i;
            return 0;
        }
    }

    /* check to see if the number has leading/trailing decimal point. */
    i = 0;

    /* skip any leading '+' or '-'. */
    if ((i < number->number_size)
        && (('+' == number->number[i]) || ('-' == number->number[i]))) {
        i++;
    }

    /* check if we have infinity. */
    if ((i < number->number_size) && ('I' == number->number[i])) {
        const char *inf = "Infinity";
        size_t k;

        for (k = i; k < number->number_size; k++) {
            const char c = *inf++;

            /* Check if we found the Infinity string! */
            if ('\0' == c) {
                break;
            } else if (c != number->number[k]) {
                break;
            }
        }

        if ('\0' == *inf) {
            /* Inf becomes 1.7976931348623158e308 because JSON can't support it.
             */
            *size += 22;

            /* if we had a leading '-' we need to record it in the JSON output.
             */
            if ('-' == number->number[0]) {
                *size += 1;
            }
        }

        return 0;
    }

    /* check if we have nan. */
    if ((i < number->number_size) && ('N' == number->number[i])) {
        const char *nan = "NaN";
        size_t k;

        for (k = i; k < number->number_size; k++) {
            const char c = *nan++;

            /* Check if we found the NaN string! */
            if ('\0' == c) {
                break;
            } else if (c != number->number[k]) {
                break;
            }
        }

        if ('\0' == *nan) {
            /* NaN becomes 1 because JSON can't support it. */
            *size += 1;

            return 0;
        }
    }

    /* if we had a leading decimal point. */
    if ((i < number->number_size) && ('.' == number->number[i])) {
        /* 1 + because we had a leading decimal point. */
        *size += 1;
        goto cleanup;
    }

    for (; i < number->number_size; i++) {
        const char c = number->number[i];
        if (!('0' <= c && c <= '9')) {
            break;
        }
    }

    /* if we had a trailing decimal point. */
    if ((i + 1 == number->number_size) && ('.' == number->number[i])) {
        /* 1 + because we had a trailing decimal point. */
        *size += 1;
        goto cleanup;
    }

cleanup:
    *size += number->number_size; /* the actual string of the number. */

    /* if we had a leading '+' we don't record it in the JSON output. */
    if ('+' == number->number[0]) {
        *size -= 1;
    }

    return 0;
}

int json_write_get_string_size(const struct json_string_s *string, size_t *size)
{
    size_t i;
    for (i = 0; i < string->string_size; i++) {
        switch (string->string[i]) {
        case '"':
        case '\\':
        case '\b':
        case '\f':
        case '\n':
        case '\r':
        case '\t':
            *size += 2;
            break;
        default:
            *size += 1;
            break;
        }
    }

    *size += 2; /* need to encode the surrounding '"' characters. */

    return 0;
}

int json_write_minified_get_array_size(
    const struct json_array_s *array, size_t *size)
{
    struct json_array_element_s *element;

    *size += 2; /* '[' and ']'. */

    if (1 < array->length) {
        *size += array->length - 1; /* ','s seperate each element. */
    }

    for (element = array->start; json_null != element;
         element = element->next) {
        if (json_write_minified_get_value_size(element->value, size)) {
            /* value was malformed! */
            return 1;
        }
    }

    return 0;
}

int json_write_minified_get_object_size(
    const struct json_object_s *object, size_t *size)
{
    struct json_object_element_s *element;

    *size += 2; /* '{' and '}'. */

    *size += object->length; /* ':'s seperate each name/value pair. */

    if (1 < object->length) {
        *size += object->length - 1; /* ','s seperate each element. */
    }

    for (element = object->start; json_null != element;
         element = element->next) {
        if (json_write_get_string_size(element->name, size)) {
            /* string was malformed! */
            return 1;
        }

        if (json_write_minified_get_value_size(element->value, size)) {
            /* value was malformed! */
            return 1;
        }
    }

    return 0;
}

int json_write_minified_get_value_size(
    const struct json_value_s *value, size_t *size)
{
    switch (value->type) {
    default:
        /* unknown value type found! */
        return 1;
    case json_type_number:
        return json_write_get_number_size(
            (struct json_number_s *)value->payload, size);
    case json_type_string:
        return json_write_get_string_size(
            (struct json_string_s *)value->payload, size);
    case json_type_array:
        return json_write_minified_get_array_size(
            (struct json_array_s *)value->payload, size);
    case json_type_object:
        return json_write_minified_get_object_size(
            (struct json_object_s *)value->payload, size);
    case json_type_true:
        *size += 4; /* the string "true". */
        return 0;
    case json_type_false:
        *size += 5; /* the string "false". */
        return 0;
    case json_type_null:
        *size += 4; /* the string "null". */
        return 0;
    }
}

char *json_write_number(const struct json_number_s *number, char *data)
{
    json_uintmax_t parsed_number, backup;
    size_t i;

    if (number->number_size >= 2) {
        switch (number->number[1]) {
        case 'x':
        case 'X':
            /* The number is a json_parse_flags_allow_hexadecimal_numbers
             * hexadecimal so we have to do extra work to convert it to a
             * non-hexadecimal for JSON output. */
            parsed_number = json_strtoumax(number->number, json_null, 0);

            /* We need a copy of parsed number twice, so take a backup of it. */
            backup = parsed_number;

            i = 0;

            while (0 != parsed_number) {
                parsed_number /= 10;
                i++;
            }

            /* Restore parsed_number to its original value stored in the backup.
             */
            parsed_number = backup;

            /* Now use backup to take a copy of i, or the length of the string.
             */
            backup = i;

            do {
                *(data + i - 1) = '0' + (char)(parsed_number % 10);
                parsed_number /= 10;
                i--;
            } while (0 != parsed_number);

            data += backup;

            return data;
        }
    }

    /* check to see if the number has leading/trailing decimal point. */
    i = 0;

    /* skip any leading '-'. */
    if ((i < number->number_size)
        && (('+' == number->number[i]) || ('-' == number->number[i]))) {
        i++;
    }

    /* check if we have infinity. */
    if ((i < number->number_size) && ('I' == number->number[i])) {
        const char *inf = "Infinity";
        size_t k;

        for (k = i; k < number->number_size; k++) {
            const char c = *inf++;

            /* Check if we found the Infinity string! */
            if ('\0' == c) {
                break;
            } else if (c != number->number[k]) {
                break;
            }
        }

        if ('\0' == *inf++) {
            const char *dbl_max;

            /* if we had a leading '-' we need to record it in the JSON output.
             */
            if ('-' == number->number[0]) {
                *data++ = '-';
            }

            /* Inf becomes 1.7976931348623158e308 because JSON can't support it.
             */
            for (dbl_max = "1.7976931348623158e308"; '\0' != *dbl_max;
                 dbl_max++) {
                *data++ = *dbl_max;
            }

            return data;
        }
    }

    /* check if we have nan. */
    if ((i < number->number_size) && ('N' == number->number[i])) {
        const char *nan = "NaN";
        size_t k;

        for (k = i; k < number->number_size; k++) {
            const char c = *nan++;

            /* Check if we found the NaN string! */
            if ('\0' == c) {
                break;
            } else if (c != number->number[k]) {
                break;
            }
        }

        if ('\0' == *nan++) {
            /* NaN becomes 0 because JSON can't support it. */
            *data++ = '0';
            return data;
        }
    }

    /* if we had a leading decimal point. */
    if ((i < number->number_size) && ('.' == number->number[i])) {
        i = 0;

        /* skip any leading '+'. */
        if ('+' == number->number[i]) {
            i++;
        }

        /* output the leading '-' if we had one. */
        if ('-' == number->number[i]) {
            *data++ = '-';
            i++;
        }

        /* insert a '0' to fix the leading decimal point for JSON output. */
        *data++ = '0';

        /* and output the rest of the number as normal. */
        for (; i < number->number_size; i++) {
            *data++ = number->number[i];
        }

        return data;
    }

    for (; i < number->number_size; i++) {
        const char c = number->number[i];
        if (!('0' <= c && c <= '9')) {
            break;
        }
    }

    /* if we had a trailing decimal point. */
    if ((i + 1 == number->number_size) && ('.' == number->number[i])) {
        i = 0;

        /* skip any leading '+'. */
        if ('+' == number->number[i]) {
            i++;
        }

        /* output the leading '-' if we had one. */
        if ('-' == number->number[i]) {
            *data++ = '-';
            i++;
        }

        /* and output the rest of the number as normal. */
        for (; i < number->number_size; i++) {
            *data++ = number->number[i];
        }

        /* insert a '0' to fix the trailing decimal point for JSON output. */
        *data++ = '0';

        return data;
    }

    i = 0;

    /* skip any leading '+'. */
    if ('+' == number->number[i]) {
        i++;
    }

    for (; i < number->number_size; i++) {
        *data++ = number->number[i];
    }

    return data;
}

char *json_write_string(const struct json_string_s *string, char *data)
{
    size_t i;

    *data++ = '"'; /* open the string. */

    for (i = 0; i < string->string_size; i++) {
        switch (string->string[i]) {
        case '"':
            *data++ = '\\'; /* escape the control character. */
            *data++ = '"';
            break;
        case '\\':
            *data++ = '\\'; /* escape the control character. */
            *data++ = '\\';
            break;
        case '\b':
            *data++ = '\\'; /* escape the control character. */
            *data++ = 'b';
            break;
        case '\f':
            *data++ = '\\'; /* escape the control character. */
            *data++ = 'f';
            break;
        case '\n':
            *data++ = '\\'; /* escape the control character. */
            *data++ = 'n';
            break;
        case '\r':
            *data++ = '\\'; /* escape the control character. */
            *data++ = 'r';
            break;
        case '\t':
            *data++ = '\\'; /* escape the control character. */
            *data++ = 't';
            break;
        default:
            *data++ = string->string[i];
            break;
        }
    }

    *data++ = '"'; /* close the string. */

    return data;
}

char *json_write_minified_array(const struct json_array_s *array, char *data)
{
    struct json_array_element_s *element = json_null;

    *data++ = '['; /* open the array. */

    for (element = array->start; json_null != element;
         element = element->next) {
        if (element != array->start) {
            *data++ = ','; /* ','s seperate each element. */
        }

        data = json_write_minified_value(element->value, data);

        if (json_null == data) {
            /* value was malformed! */
            return json_null;
        }
    }

    *data++ = ']'; /* close the array. */

    return data;
}

char *json_write_minified_object(const struct json_object_s *object, char *data)
{
    struct json_object_element_s *element = json_null;

    *data++ = '{'; /* open the object. */

    for (element = object->start; json_null != element;
         element = element->next) {
        if (element != object->start) {
            *data++ = ','; /* ','s seperate each element. */
        }

        data = json_write_string(element->name, data);

        if (json_null == data) {
            /* string was malformed! */
            return json_null;
        }

        *data++ = ':'; /* ':'s seperate each name/value pair. */

        data = json_write_minified_value(element->value, data);

        if (json_null == data) {
            /* value was malformed! */
            return json_null;
        }
    }

    *data++ = '}'; /* close the object. */

    return data;
}

char *json_write_minified_value(const struct json_value_s *value, char *data)
{
    switch (value->type) {
    default:
        /* unknown value type found! */
        return json_null;
    case json_type_number:
        return json_write_number((struct json_number_s *)value->payload, data);
    case json_type_string:
        return json_write_string((struct json_string_s *)value->payload, data);
    case json_type_array:
        return json_write_minified_array(
            (struct json_array_s *)value->payload, data);
    case json_type_object:
        return json_write_minified_object(
            (struct json_object_s *)value->payload, data);
    case json_type_true:
        data[0] = 't';
        data[1] = 'r';
        data[2] = 'u';
        data[3] = 'e';
        return data + 4;
    case json_type_false:
        data[0] = 'f';
        data[1] = 'a';
        data[2] = 'l';
        data[3] = 's';
        data[4] = 'e';
        return data + 5;
    case json_type_null:
        data[0] = 'n';
        data[1] = 'u';
        data[2] = 'l';
        data[3] = 'l';
        return data + 4;
    }
}

void *json_write_minified(const struct json_value_s *value, size_t *out_size)
{
    size_t size = 0;
    char *data = json_null;
    char *data_end = json_null;

    if (json_null == value) {
        return json_null;
    }

    if (json_write_minified_get_value_size(value, &size)) {
        /* value was malformed! */
        return json_null;
    }

    size += 1; /* for the '\0' null terminating character. */

    data = (char *)malloc(size);

    if (json_null == data) {
        /* malloc failed! */
        return json_null;
    }

    data_end = json_write_minified_value(value, data);

    if (json_null == data_end) {
        /* bad chi occurred! */
        free(data);
        return json_null;
    }

    /* null terminated the string. */
    *data_end = '\0';

    if (json_null != out_size) {
        *out_size = size;
    }

    return data;
}

int json_write_pretty_get_array_size(
    const struct json_array_s *array, size_t depth, size_t indent_size,
    size_t newline_size, size_t *size)
{
    struct json_array_element_s *element;

    *size += 1; /* '['. */

    if (0 < array->length) {
        /* if we have any elements we need to add a newline after our '['. */
        *size += newline_size;

        *size += array->length - 1; /* ','s seperate each element. */

        for (element = array->start; json_null != element;
             element = element->next) {
            /* each element gets an indent. */
            *size += (depth + 1) * indent_size;

            if (json_write_pretty_get_value_size(
                    element->value, depth + 1, indent_size, newline_size,
                    size)) {
                /* value was malformed! */
                return 1;
            }

            /* each element gets a newline too. */
            *size += newline_size;
        }

        /* since we wrote out some elements, need to add a newline and
         * indentation.
         */
        /* to the trailing ']'. */
        *size += depth * indent_size;
    }

    *size += 1; /* ']'. */

    return 0;
}

int json_write_pretty_get_object_size(
    const struct json_object_s *object, size_t depth, size_t indent_size,
    size_t newline_size, size_t *size)
{
    struct json_object_element_s *element;

    *size += 1; /* '{'. */

    if (0 < object->length) {
        *size += newline_size; /* need a newline next. */

        *size += object->length - 1; /* ','s seperate each element. */

        for (element = object->start; json_null != element;
             element = element->next) {
            /* each element gets an indent and newline. */
            *size += (depth + 1) * indent_size;
            *size += newline_size;

            if (json_write_get_string_size(element->name, size)) {
                /* string was malformed! */
                return 1;
            }

            *size += 3; /* seperate each name/value pair with " : ". */

            if (json_write_pretty_get_value_size(
                    element->value, depth + 1, indent_size, newline_size,
                    size)) {
                /* value was malformed! */
                return 1;
            }
        }

        *size += depth * indent_size;
    }

    *size += 1; /* '}'. */

    return 0;
}

int json_write_pretty_get_value_size(
    const struct json_value_s *value, size_t depth, size_t indent_size,
    size_t newline_size, size_t *size)
{
    switch (value->type) {
    default:
        /* unknown value type found! */
        return 1;
    case json_type_number:
        return json_write_get_number_size(
            (struct json_number_s *)value->payload, size);
    case json_type_string:
        return json_write_get_string_size(
            (struct json_string_s *)value->payload, size);
    case json_type_array:
        return json_write_pretty_get_array_size(
            (struct json_array_s *)value->payload, depth, indent_size,
            newline_size, size);
    case json_type_object:
        return json_write_pretty_get_object_size(
            (struct json_object_s *)value->payload, depth, indent_size,
            newline_size, size);
    case json_type_true:
        *size += 4; /* the string "true". */
        return 0;
    case json_type_false:
        *size += 5; /* the string "false". */
        return 0;
    case json_type_null:
        *size += 4; /* the string "null". */
        return 0;
    }
}

char *json_write_pretty_array(
    const struct json_array_s *array, size_t depth, const char *indent,
    const char *newline, char *data)
{
    size_t k, m;
    struct json_array_element_s *element;

    *data++ = '['; /* open the array. */

    if (0 < array->length) {
        for (k = 0; '\0' != newline[k]; k++) {
            *data++ = newline[k];
        }

        for (element = array->start; json_null != element;
             element = element->next) {
            if (element != array->start) {
                *data++ = ','; /* ','s seperate each element. */

                for (k = 0; '\0' != newline[k]; k++) {
                    *data++ = newline[k];
                }
            }

            for (k = 0; k < depth + 1; k++) {
                for (m = 0; '\0' != indent[m]; m++) {
                    *data++ = indent[m];
                }
            }

            data = json_write_pretty_value(
                element->value, depth + 1, indent, newline, data);

            if (json_null == data) {
                /* value was malformed! */
                return json_null;
            }
        }

        for (k = 0; '\0' != newline[k]; k++) {
            *data++ = newline[k];
        }

        for (k = 0; k < depth; k++) {
            for (m = 0; '\0' != indent[m]; m++) {
                *data++ = indent[m];
            }
        }
    }

    *data++ = ']'; /* close the array. */

    return data;
}

char *json_write_pretty_object(
    const struct json_object_s *object, size_t depth, const char *indent,
    const char *newline, char *data)
{
    size_t k, m;
    struct json_object_element_s *element;

    *data++ = '{'; /* open the object. */

    if (0 < object->length) {
        for (k = 0; '\0' != newline[k]; k++) {
            *data++ = newline[k];
        }

        for (element = object->start; json_null != element;
             element = element->next) {
            if (element != object->start) {
                *data++ = ','; /* ','s seperate each element. */

                for (k = 0; '\0' != newline[k]; k++) {
                    *data++ = newline[k];
                }
            }

            for (k = 0; k < depth + 1; k++) {
                for (m = 0; '\0' != indent[m]; m++) {
                    *data++ = indent[m];
                }
            }

            data = json_write_string(element->name, data);

            if (json_null == data) {
                /* string was malformed! */
                return json_null;
            }

            /* " : "s seperate each name/value pair. */
            *data++ = ' ';
            *data++ = ':';
            *data++ = ' ';

            data = json_write_pretty_value(
                element->value, depth + 1, indent, newline, data);

            if (json_null == data) {
                /* value was malformed! */
                return json_null;
            }
        }

        for (k = 0; '\0' != newline[k]; k++) {
            *data++ = newline[k];
        }

        for (k = 0; k < depth; k++) {
            for (m = 0; '\0' != indent[m]; m++) {
                *data++ = indent[m];
            }
        }
    }

    *data++ = '}'; /* close the object. */

    return data;
}

char *json_write_pretty_value(
    const struct json_value_s *value, size_t depth, const char *indent,
    const char *newline, char *data)
{
    switch (value->type) {
    default:
        /* unknown value type found! */
        return json_null;
    case json_type_number:
        return json_write_number((struct json_number_s *)value->payload, data);
    case json_type_string:
        return json_write_string((struct json_string_s *)value->payload, data);
    case json_type_array:
        return json_write_pretty_array(
            (struct json_array_s *)value->payload, depth, indent, newline,
            data);
    case json_type_object:
        return json_write_pretty_object(
            (struct json_object_s *)value->payload, depth, indent, newline,
            data);
    case json_type_true:
        data[0] = 't';
        data[1] = 'r';
        data[2] = 'u';
        data[3] = 'e';
        return data + 4;
    case json_type_false:
        data[0] = 'f';
        data[1] = 'a';
        data[2] = 'l';
        data[3] = 's';
        data[4] = 'e';
        return data + 5;
    case json_type_null:
        data[0] = 'n';
        data[1] = 'u';
        data[2] = 'l';
        data[3] = 'l';
        return data + 4;
    }
}

void *json_write_pretty(
    const struct json_value_s *value, const char *indent, const char *newline,
    size_t *out_size)
{
    size_t size = 0;
    size_t indent_size = 0;
    size_t newline_size = 0;
    char *data = json_null;
    char *data_end = json_null;

    if (json_null == value) {
        return json_null;
    }

    if (json_null == indent) {
        indent = "  "; /* default to two spaces. */
    }

    if (json_null == newline) {
        newline = "\n"; /* default to linux newlines. */
    }

    while ('\0' != indent[indent_size]) {
        ++indent_size; /* skip non-null terminating characters. */
    }

    while ('\0' != newline[newline_size]) {
        ++newline_size; /* skip non-null terminating characters. */
    }

    if (json_write_pretty_get_value_size(
            value, 0, indent_size, newline_size, &size)) {
        /* value was malformed! */
        return json_null;
    }

    size += 1; /* for the '\0' null terminating character. */

    data = (char *)malloc(size);

    if (json_null == data) {
        /* malloc failed! */
        return json_null;
    }

    data_end = json_write_pretty_value(value, 0, indent, newline, data);

    if (json_null == data_end) {
        /* bad chi occurred! */
        free(data);
        return json_null;
    }

    /* null terminated the string. */
    *data_end = '\0';

    if (json_null != out_size) {
        *out_size = size;
    }

    return data;
}

const char *json_get_error_description(enum json_parse_error_e error)
{
    switch (error) {
    case json_parse_error_none:
        return "no error";

    case json_parse_error_expected_comma_or_closing_bracket:
        return "expected comma or closing bracket";

    case json_parse_error_expected_colon:
        return "expected colon";

    case json_parse_error_expected_opening_quote:
        return "expected opening quote";

    case json_parse_error_invalid_string_escape_sequence:
        return "invalid string escape sequence";

    case json_parse_error_invalid_number_format:
        return "invalid number format";

    case json_parse_error_invalid_value:
        return "invalid value";

    case json_parse_error_premature_end_of_buffer:
        return "premature end of buffer";

    case json_parse_error_invalid_string:
        return "allocator failed";

    case json_parse_error_allocator_failed:
        return "allocator failed";

    case json_parse_error_unexpected_trailing_characters:
        return "unexpected trailing characters";

    case json_parse_error_unknown:
    default:
        return "unknown";
    }
}

struct json_number_s *json_number_new_int(int number)
{
    size_t size = 1;
    int tmp = number;
    while (tmp) {
        tmp /= 10;
        size++;
    }

    char *buf = malloc(size);
    sprintf(buf, "%d", number);
    struct json_number_s *elem = malloc(sizeof(struct json_number_s));
    elem->number = buf;
    elem->number_size = strlen(buf);
    return elem;
}

struct json_number_s *json_number_new_double(double number)
{
    size_t size = 30;
    char *buf = malloc(size);
    sprintf(buf, "%f", number);
    struct json_number_s *elem = malloc(sizeof(struct json_number_s));
    elem->number = buf;
    elem->number_size = strlen(buf);
    return elem;
}

void json_number_free(struct json_number_s *num)
{
    if (!num->ref_count) {
        free(num->number);
        free(num);
    }
}

struct json_string_s *json_string_new(const char *string)
{
    struct json_string_s *str = malloc(sizeof(struct json_string_s));
    str->string = strdup(string);
    str->string_size = strlen(string);
    return str;
}

void json_string_free(struct json_string_s *str)
{
    if (!str->ref_count) {
        free(str->string);
        free(str);
    }
}

struct json_array_s *json_array_new()
{
    struct json_array_s *arr = malloc(sizeof(struct json_array_s));
    arr->start = NULL;
    arr->length = 0;
    return arr;
}

void json_array_free(struct json_array_s *arr)
{
    if (!arr->ref_count) {
        struct json_array_element_s *elem = arr->start;
        while (elem) {
            struct json_array_element_s *next = elem->next;
            json_value_free(elem->value);
            free(elem);
            elem = next;
        }
        free(arr);
    }
}

void json_array_append(struct json_array_s *arr, struct json_value_s *value)
{
    struct json_array_element_s *elem =
        malloc(sizeof(struct json_array_element_s));
    elem->value = value;
    elem->next = NULL;
    if (arr->start) {
        struct json_array_element_s *target = arr->start;
        while (target->next) {
            target = target->next;
        }
        target->next = elem;
    } else {
        arr->start = elem;
    }
    arr->length++;
}

void json_array_append_bool(struct json_array_s *arr, int b)
{
    json_array_append(arr, json_value_from_bool(b));
}

void json_array_append_number_int(struct json_array_s *arr, int number)
{
    json_array_append(arr, json_value_from_number(json_number_new_int(number)));
}

void json_array_append_number_double(struct json_array_s *arr, double number)
{
    json_array_append(
        arr, json_value_from_number(json_number_new_double(number)));
}

void json_array_append_array(
    struct json_array_s *arr, struct json_array_s *arr2)
{
    json_array_append(arr, json_value_from_array(arr2));
}

struct json_value_s *
json_array_get_value(struct json_array_s *arr, const size_t idx)
{
    if (!arr || idx >= arr->length) {
        return json_null;
    }
    struct json_array_element_s *elem = arr->start;
    for (size_t i = 0; i < idx; i++) {
        elem = elem->next;
    }
    return elem->value;
}

int json_array_get_bool(struct json_array_s *arr, const size_t idx, int d)
{
    struct json_value_s *value = json_array_get_value(arr, idx);
    if (json_value_is_true(value)) {
        return 1;
    } else if (json_value_is_false(value)) {
        return 0;
    }
    return d;
}

int json_array_get_number_int(struct json_array_s *arr, const size_t idx, int d)
{
    struct json_value_s *value = json_array_get_value(arr, idx);
    struct json_number_s *num = json_value_as_number(value);
    if (num) {
        return atoi(num->number);
    }
    return d;
}

double json_array_get_number_double(
    struct json_array_s *arr, const size_t idx, double d)
{
    struct json_value_s *value = json_array_get_value(arr, idx);
    struct json_number_s *num = json_value_as_number(value);
    if (num) {
        return atof(num->number);
    }
    return d;
}

const char *
json_array_get_string(struct json_array_s *arr, const size_t idx, const char *d)
{
    struct json_value_s *value = json_array_get_value(arr, idx);
    struct json_string_s *str = json_value_as_string(value);
    if (str) {
        return str->string;
    }
    return d;
}

struct json_array_s *
json_array_get_array(struct json_array_s *arr, const size_t idx)
{
    struct json_value_s *value = json_array_get_value(arr, idx);
    struct json_array_s *arr2 = json_value_as_array(value);
    return arr2;
}

struct json_object_s *
json_array_get_object(struct json_array_s *arr, const size_t idx)
{
    struct json_value_s *value = json_array_get_value(arr, idx);
    struct json_object_s *obj = json_value_as_object(value);
    return obj;
}

struct json_object_s *json_object_new()
{
    struct json_object_s *obj = malloc(sizeof(struct json_object_s));
    obj->start = NULL;
    obj->length = 0;
    return obj;
}

void json_object_free(struct json_object_s *obj)
{
    if (!obj->ref_count) {
        struct json_object_element_s *elem = obj->start;
        while (elem) {
            struct json_object_element_s *next = elem->next;
            json_string_free(elem->name);
            json_value_free(elem->value);
            free(elem);
            elem = next;
        }
        free(obj);
    }
}

void json_object_append(
    struct json_object_s *obj, const char *key, struct json_value_s *value)
{
    struct json_object_element_s *elem =
        malloc(sizeof(struct json_object_element_s));
    elem->name = json_string_new(key);
    elem->value = value;
    elem->next = NULL;
    if (obj->start) {
        struct json_object_element_s *target = obj->start;
        while (target->next) {
            target = target->next;
        }
        target->next = elem;
    } else {
        obj->start = elem;
    }
    obj->length++;
}

void json_object_append_bool(struct json_object_s *obj, const char *key, int b)
{
    json_object_append(obj, key, json_value_from_bool(b));
}

void json_object_append_number_int(
    struct json_object_s *obj, const char *key, int number)
{
    json_object_append(
        obj, key, json_value_from_number(json_number_new_int(number)));
}

void json_object_append_number_double(
    struct json_object_s *obj, const char *key, double number)
{
    json_object_append(
        obj, key, json_value_from_number(json_number_new_double(number)));
}

void json_object_append_array(
    struct json_object_s *obj, const char *key, struct json_array_s *arr)
{
    json_object_append(obj, key, json_value_from_array(arr));
}

struct json_value_s *
json_object_get_value(struct json_object_s *obj, const char *key)
{
    if (!obj) {
        return json_null;
    }
    struct json_object_element_s *elem = obj->start;
    while (elem) {
        if (!strcmp(elem->name->string, key)) {
            return elem->value;
        }
        elem = elem->next;
    }
    return json_null;
}

int json_object_get_bool(struct json_object_s *obj, const char *key, int d)
{
    struct json_value_s *value = json_object_get_value(obj, key);
    if (json_value_is_true(value)) {
        return 1;
    } else if (json_value_is_false(value)) {
        return 0;
    }
    return d;
}

int json_object_get_number_int(
    struct json_object_s *obj, const char *key, int d)
{
    struct json_value_s *value = json_object_get_value(obj, key);
    struct json_number_s *num = json_value_as_number(value);
    if (num) {
        return atoi(num->number);
    }
    return d;
}

double json_object_get_number_double(
    struct json_object_s *obj, const char *key, double d)
{
    struct json_value_s *value = json_object_get_value(obj, key);
    struct json_number_s *num = json_value_as_number(value);
    if (num) {
        return atof(num->number);
    }
    return d;
}

const char *json_object_get_string(
    struct json_object_s *obj, const char *key, const char *d)
{
    struct json_value_s *value = json_object_get_value(obj, key);
    struct json_string_s *str = json_value_as_string(value);
    if (str) {
        return str->string;
    }
    return d;
}

struct json_array_s *
json_object_get_array(struct json_object_s *obj, const char *key)
{
    struct json_value_s *value = json_object_get_value(obj, key);
    struct json_array_s *arr = json_value_as_array(value);
    return arr;
}

struct json_object_s *
json_object_get_object(struct json_object_s *obj, const char *key)
{
    struct json_value_s *value = json_object_get_value(obj, key);
    struct json_object_s *obj2 = json_value_as_object(value);
    return obj2;
}

struct json_value_s *json_value_from_bool(int b)
{
    struct json_value_s *value = malloc(sizeof(struct json_value_s));
    value->type = b ? json_type_true : json_type_false;
    value->payload = NULL;
    return value;
}

struct json_value_s *json_value_from_number(struct json_number_s *num)
{
    struct json_value_s *value = malloc(sizeof(struct json_value_s));
    value->type = json_type_number;
    value->payload = num;
    return value;
}

struct json_value_s *json_value_from_string(struct json_string_s *str)
{
    struct json_value_s *value = malloc(sizeof(struct json_value_s));
    value->type = json_type_string;
    value->payload = str;
    return value;
}

struct json_value_s *json_value_from_array(struct json_array_s *arr)
{
    struct json_value_s *value = malloc(sizeof(struct json_value_s));
    value->type = json_type_array;
    value->payload = arr;
    return value;
}

struct json_value_s *json_value_from_object(struct json_object_s *obj)
{
    struct json_value_s *value = malloc(sizeof(struct json_value_s));
    value->type = json_type_object;
    value->payload = obj;
    return value;
}

void json_value_free(struct json_value_s *value)
{
    if (!value->ref_count) {
        switch (value->type) {
        case json_type_number:
            json_number_free((struct json_number_s *)value->payload);
            break;
        case json_type_string:
            json_string_free((struct json_string_s *)value->payload);
            break;
        case json_type_array:
            json_array_free((struct json_array_s *)value->payload);
            break;
        case json_type_object:
            json_object_free((struct json_object_s *)value->payload);
            break;
        case json_type_true:
        case json_type_null:
        case json_type_false:
            break;
        }

        free(value);
    }
}
