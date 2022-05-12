#include "json/json_write.h"

#include "json/json_base.h"
#include "memory.h"

static int json_write_minified_get_value_size(
    const struct json_value_s *value, size_t *size);
static int json_write_get_number_size(
    const struct json_number_s *number, size_t *size);
static int json_write_get_string_size(
    const struct json_string_s *string, size_t *size);
static int json_write_minified_get_array_size(
    const struct json_array_s *array, size_t *size);
static int json_write_minified_get_object_size(
    const struct json_object_s *object, size_t *size);
static char *json_write_minified_value(
    const struct json_value_s *value, char *data);
static char *json_write_number(const struct json_number_s *number, char *data);
static char *json_write_string(const struct json_string_s *string, char *data);
static char *json_write_minified_array(
    const struct json_array_s *array, char *data);
static char *json_write_minified_object(
    const struct json_object_s *object, char *data);
static int json_write_pretty_get_value_size(
    const struct json_value_s *value, size_t depth, size_t indent_size,
    size_t newline_size, size_t *size);
static int json_write_pretty_get_array_size(
    const struct json_array_s *array, size_t depth, size_t indent_size,
    size_t newline_size, size_t *size);
static int json_write_pretty_get_object_size(
    const struct json_object_s *object, size_t depth, size_t indent_size,
    size_t newline_size, size_t *size);
static char *json_write_pretty_value(
    const struct json_value_s *value, size_t depth, const char *indent,
    const char *newline, char *data);
static char *json_write_pretty_array(
    const struct json_array_s *array, size_t depth, const char *indent,
    const char *newline, char *data);
static char *json_write_pretty_object(
    const struct json_object_s *object, size_t depth, const char *indent,
    const char *newline, char *data);

static int json_write_get_number_size(
    const struct json_number_s *number, size_t *size)
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

static int json_write_get_string_size(
    const struct json_string_s *string, size_t *size)
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

static int json_write_minified_get_array_size(
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

static int json_write_minified_get_object_size(
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

static int json_write_minified_get_value_size(
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

static char *json_write_number(const struct json_number_s *number, char *data)
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

static char *json_write_string(const struct json_string_s *string, char *data)
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

static char *json_write_minified_array(
    const struct json_array_s *array, char *data)
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

static char *json_write_minified_object(
    const struct json_object_s *object, char *data)
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

static char *json_write_minified_value(
    const struct json_value_s *value, char *data)
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

    data = (char *)Memory_Alloc(size);

    if (json_null == data) {
        /* malloc failed! */
        return json_null;
    }

    data_end = json_write_minified_value(value, data);

    if (json_null == data_end) {
        /* bad chi occurred! */
        Memory_Free(data);
        return json_null;
    }

    /* null terminated the string. */
    *data_end = '\0';

    if (json_null != out_size) {
        *out_size = size;
    }

    return data;
}

static int json_write_pretty_get_array_size(
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

static int json_write_pretty_get_object_size(
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

static int json_write_pretty_get_value_size(
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

static char *json_write_pretty_array(
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

static char *json_write_pretty_object(
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

static char *json_write_pretty_value(
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

    data = (char *)Memory_Alloc(size);

    if (json_null == data) {
        /* malloc failed! */
        return json_null;
    }

    data_end = json_write_pretty_value(value, 0, indent, newline, data);

    if (json_null == data_end) {
        /* bad chi occurred! */
        Memory_Free(data);
        return json_null;
    }

    /* null terminated the string. */
    *data_end = '\0';

    if (json_null != out_size) {
        *out_size = size;
    }

    return data;
}
