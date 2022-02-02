#include "json/json_base.h"

#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

struct json_number_s *json_number_new_int(int number)
{
    size_t size = 1;
    int tmp = number;
    while (tmp) {
        tmp /= 10;
        size++;
    }

    char *buf = Memory_Alloc(size);
    sprintf(buf, "%d", number);
    struct json_number_s *elem = Memory_Alloc(sizeof(struct json_number_s));
    elem->number = buf;
    elem->number_size = strlen(buf);
    return elem;
}

struct json_number_s *json_number_new_double(double number)
{
    size_t size = 30;
    char *buf = Memory_Alloc(size);
    sprintf(buf, "%f", number);
    struct json_number_s *elem = Memory_Alloc(sizeof(struct json_number_s));
    elem->number = buf;
    elem->number_size = strlen(buf);
    return elem;
}

void json_number_free(struct json_number_s *num)
{
    if (!num->ref_count) {
        Memory_Free(num->number);
        Memory_Free(num);
    }
}

struct json_string_s *json_string_new(const char *string)
{
    struct json_string_s *str = Memory_Alloc(sizeof(struct json_string_s));
    str->string = Memory_Dup(string);
    str->string_size = strlen(string);
    return str;
}

void json_string_free(struct json_string_s *str)
{
    if (!str->ref_count) {
        Memory_Free(str->string);
        Memory_Free(str);
    }
}

struct json_array_s *json_array_new()
{
    struct json_array_s *arr = Memory_Alloc(sizeof(struct json_array_s));
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
            Memory_Free(elem);
            elem = next;
        }
        Memory_Free(arr);
    }
}

void json_array_append(struct json_array_s *arr, struct json_value_s *value)
{
    struct json_array_element_s *elem =
        Memory_Alloc(sizeof(struct json_array_element_s));
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

struct json_value_s *json_array_get_value(
    struct json_array_s *arr, const size_t idx)
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

const char *json_array_get_string(
    struct json_array_s *arr, const size_t idx, const char *d)
{
    struct json_value_s *value = json_array_get_value(arr, idx);
    struct json_string_s *str = json_value_as_string(value);
    if (str) {
        return str->string;
    }
    return d;
}

struct json_array_s *json_array_get_array(
    struct json_array_s *arr, const size_t idx)
{
    struct json_value_s *value = json_array_get_value(arr, idx);
    struct json_array_s *arr2 = json_value_as_array(value);
    return arr2;
}

struct json_object_s *json_array_get_object(
    struct json_array_s *arr, const size_t idx)
{
    struct json_value_s *value = json_array_get_value(arr, idx);
    struct json_object_s *obj = json_value_as_object(value);
    return obj;
}

struct json_object_s *json_object_new()
{
    struct json_object_s *obj = Memory_Alloc(sizeof(struct json_object_s));
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
            Memory_Free(elem);
            elem = next;
        }
        Memory_Free(obj);
    }
}

void json_object_append(
    struct json_object_s *obj, const char *key, struct json_value_s *value)
{
    struct json_object_element_s *elem =
        Memory_Alloc(sizeof(struct json_object_element_s));
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

struct json_value_s *json_object_get_value(
    struct json_object_s *obj, const char *key)
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

struct json_array_s *json_object_get_array(
    struct json_object_s *obj, const char *key)
{
    struct json_value_s *value = json_object_get_value(obj, key);
    struct json_array_s *arr = json_value_as_array(value);
    return arr;
}

struct json_object_s *json_object_get_object(
    struct json_object_s *obj, const char *key)
{
    struct json_value_s *value = json_object_get_value(obj, key);
    struct json_object_s *obj2 = json_value_as_object(value);
    return obj2;
}

struct json_value_s *json_value_from_bool(int b)
{
    struct json_value_s *value = Memory_Alloc(sizeof(struct json_value_s));
    value->type = b ? json_type_true : json_type_false;
    value->payload = NULL;
    return value;
}

struct json_value_s *json_value_from_number(struct json_number_s *num)
{
    struct json_value_s *value = Memory_Alloc(sizeof(struct json_value_s));
    value->type = json_type_number;
    value->payload = num;
    return value;
}

struct json_value_s *json_value_from_string(struct json_string_s *str)
{
    struct json_value_s *value = Memory_Alloc(sizeof(struct json_value_s));
    value->type = json_type_string;
    value->payload = str;
    return value;
}

struct json_value_s *json_value_from_array(struct json_array_s *arr)
{
    struct json_value_s *value = Memory_Alloc(sizeof(struct json_value_s));
    value->type = json_type_array;
    value->payload = arr;
    return value;
}

struct json_value_s *json_value_from_object(struct json_object_s *obj)
{
    struct json_value_s *value = Memory_Alloc(sizeof(struct json_value_s));
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

        Memory_Free(value);
    }
}
