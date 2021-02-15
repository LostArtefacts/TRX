#include "json_utils.h"
#include <string.h>

json_value* tr1m_json_get_field(
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

int tr1m_json_get_boolean_value(json_value* root, const char* name)
{
    json_value* field = tr1m_json_get_field(root, json_boolean, name, NULL);
    return field ? field->u.boolean : 0;
}

const char* tr1m_json_get_string_value(json_value* root, const char* name)
{
    json_value* field = tr1m_json_get_field(root, json_string, name, NULL);
    return field ? field->u.string.ptr : NULL;
}
