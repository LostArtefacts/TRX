#include "json_utils.h"
#include <string.h>
#include "util.h"

struct json_value_s *JSONGetField(struct json_value_s *root, const char *name)
{
    if (root == NULL || root->type != json_type_object) {
        return NULL;
    }
    struct json_object_s *object = json_value_as_object(root);
    struct json_object_element_s *item = object->start;
    while (item) {
        if (!strcmp(item->name->string, name)) {
            return item->value;
        }
        item = item->next;
    }
    return NULL;
}

int JSONGetBooleanValue(
    struct json_value_s *root, const char *name, int8_t *value)
{
    if (!root) {
        return 0;
    }
    struct json_value_s *field = JSONGetField(root, name);
    if (!field
        || (field->type != json_type_true && field->type != json_type_false)) {
        return 0;
    }
    *value = field->type == json_type_true;
    return 1;
}

int JSONGetIntegerValue(
    struct json_value_s *root, const char *name, int32_t *value)
{
    if (!root) {
        return 0;
    }
    struct json_value_s *field = JSONGetField(root, name);
    if (!field) {
        return 0;
    }
    struct json_number_s *number = json_value_as_number(field);
    if (!number) {
        return 0;
    }
    *value = atoi(number->number);
    return 1;
}

int JSONGetStringValue(
    struct json_value_s *root, const char *name, const char **value)
{
    if (!root) {
        return 0;
    }
    struct json_value_s *field = JSONGetField(root, name);
    if (!field || field->type != json_type_string) {
        return 0;
    }
    struct json_string_s *string = json_value_as_string(field);
    if (!string) {
        return 0;
    }
    *value = string->string;
    return 1;
}

const char *JSONGetErrorDescription(enum json_parse_error_e error)
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
