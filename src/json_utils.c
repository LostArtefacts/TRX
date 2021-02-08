#include <string.h>
#include "json_utils.h"

json_value *get_json_field(json_value *root, json_type fieldType, const char *name, int *pIndex) {
    if (root == NULL || root->type != json_object) {
        return NULL;
    }
    json_value *result = NULL;
    unsigned int len = name ? strlen(name) : 0;
    unsigned int i = pIndex ? *pIndex : 0;
    for (; i < root->u.object.length; ++i) {
        if (root->u.object.values[i].value->type == fieldType) {
            if (
                !name
                || (
                    len == root->u.object.values[i].name_length
                    && !strncmp(root->u.object.values[i].name, name, len)
                )
            ) {
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

int get_json_boolean_field_value(json_value *root, const char *name) {
    json_value *field = get_json_field(root, json_boolean, name, NULL);
    return field ? field->u.boolean : 0;
}
