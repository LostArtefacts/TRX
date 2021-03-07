#include "json_utils.h"
#include <string.h>
#include "util.h"

struct json_value_s* JSONGetField(struct json_value_s* root, const char* name)
{
    if (root == NULL || root->type != json_type_object) {
        return NULL;
    }
    struct json_object_s* object = json_value_as_object(root);
    struct json_object_element_s* item = object->start;
    while (item) {
        if (!strcmp(item->name->string, name)) {
            return item->value;
        }
        item = item->next;
    }
    return NULL;
}

int JSONGetBooleanValue(
    struct json_value_s* root, const char* name, int8_t* value)
{
    struct json_value_s* field = JSONGetField(root, name);
    if (!field
        || (field->type != json_type_true && field->type != json_type_false)) {
        return 0;
    }
    *value = field->type == json_type_true;
    return 1;
}

int JSONGetIntegerValue(
    struct json_value_s* root, const char* name, int32_t* value)
{
    struct json_value_s* field = JSONGetField(root, name);
    if (!field) {
        return 0;
    }
    struct json_number_s* number = json_value_as_number(field);
    if (!number) {
        return 0;
    }
    *value = atoi(number->number);
    return 1;
}

int JSONGetStringValue(
    struct json_value_s* root, const char* name, const char** value)
{
    struct json_value_s* field = JSONGetField(root, name);
    if (!field || field->type != json_type_string) {
        return 0;
    }
    struct json_string_s* string = json_value_as_string(field);
    if (!string) {
        return 0;
    }
    *value = string->string;
    return 1;
}
