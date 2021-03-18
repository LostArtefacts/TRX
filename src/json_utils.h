#ifndef T1M_JSON_UTILS_H
#define T1M_JSON_UTILS_H

#include "json_parser/json.h"

#define JSON_INVALID_BOOL -1
#define JSON_INVALID_STRING NULL
#define JSON_INVALID_NUMBER 0x7FFFFFFF

const char *json_get_error_description(enum json_parse_error_e error);

// numbers
struct json_number_s *json_number_new_int(int number);
struct json_number_s *json_number_new_double(double number);
void json_number_free(struct json_number_s *num);

// strings
struct json_string_s *json_string_new(const char *string);
void json_string_free(struct json_string_s *str);

// arrays
struct json_array_s *json_array_new();
void json_array_free(struct json_array_s *arr);

void json_array_append(struct json_array_s *arr, struct json_value_s *value);
void json_array_append_bool(struct json_array_s *arr, int b);
void json_array_append_number_int(struct json_array_s *arr, int number);
void json_array_append_number_double(struct json_array_s *arr, double number);
void json_array_append_array(
    struct json_array_s *arr, struct json_array_s *arr2);

struct json_value_s *
json_array_get_value(struct json_array_s *arr, const int idx);
int json_array_get_bool(struct json_array_s *arr, const int idx, int d);
int json_array_get_number_int(struct json_array_s *arr, const int idx, int d);
double
json_array_get_number_double(struct json_array_s *arr, const int idx, double d);
const char *
json_array_get_string(struct json_array_s *arr, const int idx, const char *d);
struct json_array_s *
json_array_get_array(struct json_array_s *arr, const int idx);
struct json_object_s *
json_array_get_object(struct json_array_s *arr, const int idx);

// objects
struct json_object_s *json_object_new();
void json_object_free(struct json_object_s *obj);

void json_object_append(
    struct json_object_s *obj, const char *key, struct json_value_s *value);
void json_object_append_bool(struct json_object_s *obj, const char *key, int b);
void json_object_append_number_int(
    struct json_object_s *obj, const char *key, int number);
void json_object_append_number_double(
    struct json_object_s *obj, const char *key, double number);
void json_object_append_array(
    struct json_object_s *obj, const char *key, struct json_array_s *arr);

struct json_value_s *
json_object_get_value(struct json_object_s *obj, const char *key);
int json_object_get_bool(struct json_object_s *obj, const char *key, int d);
int json_object_get_number_int(
    struct json_object_s *obj, const char *key, int d);
double json_object_get_number_double(
    struct json_object_s *obj, const char *key, double d);
const char *json_object_get_string(
    struct json_object_s *obj, const char *key, const char *d);
struct json_array_s *
json_object_get_array(struct json_object_s *obj, const char *key);
struct json_object_s *
json_object_get_object(struct json_object_s *obj, const char *key);

// values
struct json_value_s *json_value_from_bool(int b);
struct json_value_s *json_value_from_number(struct json_number_s *num);
struct json_value_s *json_value_from_string(struct json_string_s *str);
struct json_value_s *json_value_from_array(struct json_array_s *arr);
struct json_value_s *json_value_from_object(struct json_object_s *obj);
void json_value_free(struct json_value_s *value);

#endif
