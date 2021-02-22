#include "json_utils.h"
#include <string.h>

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

int8_t JSONGetBooleanValue(struct json_value_s* root, const char* name)
{
    struct json_value_s* field = JSONGetField(root, name);
    if (!field
        || (field->type != json_type_true && field->type != json_type_false)) {
        return 0;
    }
    return field->type == json_type_true;
}

const char* JSONGetStringValue(struct json_value_s* root, const char* name)
{
    struct json_value_s* field = JSONGetField(root, name);
    if (!field || field->type != json_type_string) {
        return NULL;
    }
    struct json_string_s* string = json_value_as_string(field);
    if (!string) {
        return NULL;
    }
    const char* ret = string->string;
    return ret;
}
