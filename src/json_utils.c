#include "json_utils.h"
#include <string.h>

json_value* JSONGetField(
    json_value* root, json_type field_type, const char* name, int* pIndex)
{
    if (root == NULL || root->type != json_object) {
        return NULL;
    }
    json_value* result = NULL;
    unsigned int len = name ? strlen(name) : 0;
    unsigned int i = pIndex ? *pIndex : 0;
    for (; i < root->u.object.length; ++i) {
        if (root->u.object.values[i].value->type == field_type) {
            if (!name
                || (len == root->u.object.values[i].name_length
                    && !strncmp(root->u.object.values[i].name, name, len))) {
                result = root->u.object.values[i].value;
                break;
            }
        }
    }
    if (pIndex) {
        *pIndex = i;
    }
    return result;
}

int JSONGetBooleanValue(json_value* root, const char* name)
{
    json_value* field = JSONGetField(root, json_boolean, name, NULL);
    return field ? field->u.boolean : 0;
}

const char* JSONGetStringValue(json_value* root, const char* name)
{
    json_value* field = JSONGetField(root, json_string, name, NULL);
    return field ? field->u.string.ptr : NULL;
}
