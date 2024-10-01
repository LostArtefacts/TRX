#include "json.h"

#include "memory.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

JSON_STRING *JSON_ValueAsString(JSON_VALUE *const value)
{
    if (!value || value->type != JSON_TYPE_STRING) {
        return NULL;
    }

    return (JSON_STRING *)value->payload;
}

JSON_NUMBER *JSON_ValueAsNumber(JSON_VALUE *const value)
{
    if (!value || value->type != JSON_TYPE_NUMBER) {
        return NULL;
    }

    return (JSON_NUMBER *)value->payload;
}

JSON_OBJECT *JSON_ValueAsObject(JSON_VALUE *const value)
{
    if (!value || value->type != JSON_TYPE_OBJECT) {
        return NULL;
    }

    return (JSON_OBJECT *)value->payload;
}

JSON_ARRAY *JSON_ValueAsArray(JSON_VALUE *const value)
{
    if (!value || value->type != JSON_TYPE_ARRAY) {
        return NULL;
    }

    return (JSON_ARRAY *)value->payload;
}

int JSON_ValueIsTrue(const JSON_VALUE *const value)
{
    return value && value->type == JSON_TYPE_TRUE;
}

int JSON_ValueIsFalse(const JSON_VALUE *const value)
{
    return value && value->type == JSON_TYPE_FALSE;
}

int JSON_ValueIsNull(const JSON_VALUE *const value)
{
    return value && value->type == JSON_TYPE_NULL;
}

JSON_NUMBER *JSON_NumberNewInt(int number)
{
    size_t size = snprintf(NULL, 0, "%d", number) + 1;
    char *buf = Memory_Alloc(size);
    sprintf(buf, "%d", number);
    JSON_NUMBER *elem = Memory_Alloc(sizeof(JSON_NUMBER));
    elem->number = buf;
    elem->number_size = strlen(buf);
    return elem;
}

JSON_NUMBER *JSON_NumberNewInt64(int64_t number)
{
    size_t size = snprintf(NULL, 0, "%" PRId64, number) + 1;
    char *buf = Memory_Alloc(size);
    sprintf(buf, "%" PRId64, number);
    JSON_NUMBER *elem = Memory_Alloc(sizeof(JSON_NUMBER));
    elem->number = buf;
    elem->number_size = strlen(buf);
    return elem;
}

JSON_NUMBER *JSON_NumberNewDouble(double number)
{
    size_t size = snprintf(NULL, 0, "%f", number) + 1;
    char *buf = Memory_Alloc(size);
    sprintf(buf, "%f", number);
    JSON_NUMBER *elem = Memory_Alloc(sizeof(JSON_NUMBER));
    elem->number = buf;
    elem->number_size = strlen(buf);
    return elem;
}

void JSON_NumberFree(JSON_NUMBER *num)
{
    if (!num->ref_count) {
        Memory_Free(num->number);
        Memory_Free(num);
    }
}

JSON_STRING *JSON_StringNew(const char *string)
{
    JSON_STRING *str = Memory_Alloc(sizeof(JSON_STRING));
    str->string = Memory_DupStr(string);
    str->string_size = strlen(string);
    return str;
}

void JSON_StringFree(JSON_STRING *str)
{
    if (!str->ref_count) {
        Memory_Free(str->string);
        Memory_Free(str);
    }
}

JSON_ARRAY *JSON_ArrayNew(void)
{
    JSON_ARRAY *arr = Memory_Alloc(sizeof(JSON_ARRAY));
    arr->start = NULL;
    arr->length = 0;
    return arr;
}

void JSON_ArrayFree(JSON_ARRAY *arr)
{
    JSON_ARRAY_ELEMENT *elem = arr->start;
    while (elem) {
        JSON_ARRAY_ELEMENT *next = elem->next;
        JSON_ValueFree(elem->value);
        JSON_ArrayElementFree(elem);
        elem = next;
    }
    if (!arr->ref_count) {
        Memory_Free(arr);
    }
}

void JSON_ArrayElementFree(JSON_ARRAY_ELEMENT *element)
{
    if (!element->ref_count) {
        Memory_FreePointer(&element);
    }
}

void JSON_ArrayAppend(JSON_ARRAY *arr, JSON_VALUE *value)
{
    JSON_ARRAY_ELEMENT *elem = Memory_Alloc(sizeof(JSON_ARRAY_ELEMENT));
    elem->value = value;
    elem->next = NULL;
    if (arr->start) {
        JSON_ARRAY_ELEMENT *target = arr->start;
        while (target->next) {
            target = target->next;
        }
        target->next = elem;
    } else {
        arr->start = elem;
    }
    arr->length++;
}

void JSON_ArrayApendBool(JSON_ARRAY *arr, int b)
{
    JSON_ArrayAppend(arr, JSON_ValueFromBool(b));
}

void JSON_ArrayAppendInt(JSON_ARRAY *arr, int number)
{
    JSON_ArrayAppend(arr, JSON_ValueFromNumber(JSON_NumberNewInt(number)));
}

void JSON_ArrayAppendDouble(JSON_ARRAY *arr, double number)
{
    JSON_ArrayAppend(arr, JSON_ValueFromNumber(JSON_NumberNewDouble(number)));
}

void JSON_ArrayAppendString(JSON_ARRAY *arr, const char *string)
{
    JSON_ArrayAppend(arr, JSON_ValueFromString(JSON_StringNew(string)));
}

void JSON_ArrayAppendArray(JSON_ARRAY *arr, JSON_ARRAY *arr2)
{
    JSON_ArrayAppend(arr, JSON_ValueFromArray(arr2));
}

void JSON_ArrayAppendObject(JSON_ARRAY *arr, JSON_OBJECT *obj)
{
    JSON_ArrayAppend(arr, JSON_ValueFromObject(obj));
}

JSON_VALUE *JSON_ArrayGetValue(JSON_ARRAY *arr, const size_t idx)
{
    if (!arr || idx >= arr->length) {
        return NULL;
    }
    JSON_ARRAY_ELEMENT *elem = arr->start;
    for (size_t i = 0; i < idx; i++) {
        elem = elem->next;
    }
    return elem->value;
}

int JSON_ArrayGetBool(JSON_ARRAY *arr, const size_t idx, int d)
{
    JSON_VALUE *value = JSON_ArrayGetValue(arr, idx);
    if (JSON_ValueIsTrue(value)) {
        return 1;
    } else if (JSON_ValueIsFalse(value)) {
        return 0;
    }
    return d;
}

int JSON_ArrayGetInt(JSON_ARRAY *arr, const size_t idx, int d)
{
    JSON_VALUE *value = JSON_ArrayGetValue(arr, idx);
    JSON_NUMBER *num = JSON_ValueAsNumber(value);
    if (num) {
        return atoi(num->number);
    }
    return d;
}

double JSON_ArrayGetDouble(JSON_ARRAY *arr, const size_t idx, double d)
{
    JSON_VALUE *value = JSON_ArrayGetValue(arr, idx);
    JSON_NUMBER *num = JSON_ValueAsNumber(value);
    if (num) {
        return atof(num->number);
    }
    return d;
}

const char *JSON_ArrayGetString(
    JSON_ARRAY *arr, const size_t idx, const char *d)
{
    JSON_VALUE *value = JSON_ArrayGetValue(arr, idx);
    JSON_STRING *str = JSON_ValueAsString(value);
    if (str) {
        return str->string;
    }
    return d;
}

JSON_ARRAY *JSON_ArrayGetArray(JSON_ARRAY *arr, const size_t idx)
{
    JSON_VALUE *value = JSON_ArrayGetValue(arr, idx);
    JSON_ARRAY *arr2 = JSON_ValueAsArray(value);
    return arr2;
}

JSON_OBJECT *JSON_ArrayGetObject(JSON_ARRAY *arr, const size_t idx)
{
    JSON_VALUE *value = JSON_ArrayGetValue(arr, idx);
    JSON_OBJECT *obj = JSON_ValueAsObject(value);
    return obj;
}

JSON_OBJECT *JSON_ObjectNew(void)
{
    JSON_OBJECT *obj = Memory_Alloc(sizeof(JSON_OBJECT));
    obj->start = NULL;
    obj->length = 0;
    return obj;
}

void JSON_ObjectFree(JSON_OBJECT *obj)
{
    JSON_OBJECT_ELEMENT *elem = obj->start;
    while (elem) {
        JSON_OBJECT_ELEMENT *next = elem->next;
        JSON_StringFree(elem->name);
        JSON_ValueFree(elem->value);
        JSON_ObjectElementFree(elem);
        elem = next;
    }
    if (!obj->ref_count) {
        Memory_Free(obj);
    }
}

void JSON_ObjectElementFree(JSON_OBJECT_ELEMENT *element)
{
    if (!element->ref_count) {
        Memory_FreePointer(&element);
    }
}

void JSON_ObjectAppend(JSON_OBJECT *obj, const char *key, JSON_VALUE *value)
{
    JSON_OBJECT_ELEMENT *elem = Memory_Alloc(sizeof(JSON_OBJECT_ELEMENT));
    elem->name = JSON_StringNew(key);
    elem->value = value;
    elem->next = NULL;
    if (obj->start) {
        JSON_OBJECT_ELEMENT *target = obj->start;
        while (target->next) {
            target = target->next;
        }
        target->next = elem;
    } else {
        obj->start = elem;
    }
    obj->length++;
}

void JSON_ObjectAppendBool(JSON_OBJECT *obj, const char *key, int b)
{
    JSON_ObjectAppend(obj, key, JSON_ValueFromBool(b));
}

void JSON_ObjectAppendInt(JSON_OBJECT *obj, const char *key, int number)
{
    JSON_ObjectAppend(
        obj, key, JSON_ValueFromNumber(JSON_NumberNewInt(number)));
}

void JSON_ObjectAppendInt64(JSON_OBJECT *obj, const char *key, int64_t number)
{
    JSON_ObjectAppend(
        obj, key, JSON_ValueFromNumber(JSON_NumberNewInt64(number)));
}

void JSON_ObjectAppendDouble(JSON_OBJECT *obj, const char *key, double number)
{
    JSON_ObjectAppend(
        obj, key, JSON_ValueFromNumber(JSON_NumberNewDouble(number)));
}

void JSON_ObjectAppendString(
    JSON_OBJECT *obj, const char *key, const char *string)
{
    JSON_ObjectAppend(obj, key, JSON_ValueFromString(JSON_StringNew(string)));
}

void JSON_ObjectAppendArray(JSON_OBJECT *obj, const char *key, JSON_ARRAY *arr)
{
    JSON_ObjectAppend(obj, key, JSON_ValueFromArray(arr));
}

void JSON_ObjectAppendObject(
    JSON_OBJECT *obj, const char *key, JSON_OBJECT *obj2)
{
    JSON_ObjectAppend(obj, key, JSON_ValueFromObject(obj2));
}

void JSON_ObjectEvictKey(JSON_OBJECT *obj, const char *key)
{
    if (!obj) {
        return;
    }
    JSON_OBJECT_ELEMENT *elem = obj->start;
    JSON_OBJECT_ELEMENT *prev = NULL;
    while (elem) {
        if (!strcmp(elem->name->string, key)) {
            if (!prev) {
                obj->start = elem->next;
            } else {
                prev->next = elem->next;
            }
            JSON_ObjectElementFree(elem);
            return;
        }
        prev = elem;
        elem = elem->next;
    }
}

JSON_VALUE *JSON_ObjectGetValue(JSON_OBJECT *obj, const char *key)
{
    if (!obj) {
        return NULL;
    }
    JSON_OBJECT_ELEMENT *elem = obj->start;
    while (elem) {
        if (!strcmp(elem->name->string, key)) {
            return elem->value;
        }
        elem = elem->next;
    }
    return NULL;
}

int JSON_ObjectGetBool(JSON_OBJECT *obj, const char *key, int d)
{
    JSON_VALUE *value = JSON_ObjectGetValue(obj, key);
    if (JSON_ValueIsTrue(value)) {
        return 1;
    } else if (JSON_ValueIsFalse(value)) {
        return 0;
    }
    return d;
}

int JSON_ObjectGetInt(JSON_OBJECT *obj, const char *key, int d)
{
    JSON_VALUE *value = JSON_ObjectGetValue(obj, key);
    JSON_NUMBER *num = JSON_ValueAsNumber(value);
    if (num) {
        return atoi(num->number);
    }
    return d;
}

int64_t JSON_ObjectGetInt64(JSON_OBJECT *obj, const char *key, int64_t d)
{
    JSON_VALUE *value = JSON_ObjectGetValue(obj, key);
    JSON_NUMBER *num = JSON_ValueAsNumber(value);
    if (num) {
        return strtoll(num->number, NULL, 10);
    }
    return d;
}

double JSON_ObjectGetDouble(JSON_OBJECT *obj, const char *key, double d)
{
    JSON_VALUE *value = JSON_ObjectGetValue(obj, key);
    JSON_NUMBER *num = JSON_ValueAsNumber(value);
    if (num) {
        return atof(num->number);
    }
    return d;
}

const char *JSON_ObjectGetString(
    JSON_OBJECT *obj, const char *key, const char *d)
{
    JSON_VALUE *value = JSON_ObjectGetValue(obj, key);
    JSON_STRING *str = JSON_ValueAsString(value);
    if (str) {
        return str->string;
    }
    return d;
}

JSON_ARRAY *JSON_ObjectGetArray(JSON_OBJECT *obj, const char *key)
{
    JSON_VALUE *value = JSON_ObjectGetValue(obj, key);
    JSON_ARRAY *arr = JSON_ValueAsArray(value);
    return arr;
}

JSON_OBJECT *JSON_ObjectGetObject(JSON_OBJECT *obj, const char *key)
{
    JSON_VALUE *value = JSON_ObjectGetValue(obj, key);
    JSON_OBJECT *obj2 = JSON_ValueAsObject(value);
    return obj2;
}

JSON_VALUE *JSON_ValueFromBool(int b)
{
    JSON_VALUE *value = Memory_Alloc(sizeof(JSON_VALUE));
    value->type = b ? JSON_TYPE_TRUE : JSON_TYPE_FALSE;
    value->payload = NULL;
    return value;
}

JSON_VALUE *JSON_ValueFromNumber(JSON_NUMBER *num)
{
    JSON_VALUE *value = Memory_Alloc(sizeof(JSON_VALUE));
    value->type = JSON_TYPE_NUMBER;
    value->payload = num;
    return value;
}

JSON_VALUE *JSON_ValueFromString(JSON_STRING *str)
{
    JSON_VALUE *value = Memory_Alloc(sizeof(JSON_VALUE));
    value->type = JSON_TYPE_STRING;
    value->payload = str;
    return value;
}

JSON_VALUE *JSON_ValueFromArray(JSON_ARRAY *arr)
{
    JSON_VALUE *value = Memory_Alloc(sizeof(JSON_VALUE));
    value->type = JSON_TYPE_ARRAY;
    value->payload = arr;
    return value;
}

JSON_VALUE *JSON_ValueFromObject(JSON_OBJECT *obj)
{
    JSON_VALUE *value = Memory_Alloc(sizeof(JSON_VALUE));
    value->type = JSON_TYPE_OBJECT;
    value->payload = obj;
    return value;
}

void JSON_ValueFree(JSON_VALUE *value)
{
    if (!value) {
        return;
    }
    if (!value->ref_count) {
        switch (value->type) {
        case JSON_TYPE_NUMBER:
            JSON_NumberFree((JSON_NUMBER *)value->payload);
            break;
        case JSON_TYPE_STRING:
            JSON_StringFree((JSON_STRING *)value->payload);
            break;
        case JSON_TYPE_ARRAY:
            JSON_ArrayFree((JSON_ARRAY *)value->payload);
            break;
        case JSON_TYPE_OBJECT:
            JSON_ObjectFree((JSON_OBJECT *)value->payload);
            break;
        case JSON_TYPE_TRUE:
        case JSON_TYPE_NULL:
        case JSON_TYPE_FALSE:
            break;
        }

        Memory_Free(value);
    }
}
