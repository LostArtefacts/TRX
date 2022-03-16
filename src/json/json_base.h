#pragma once

#define JSON_INVALID_BOOL -1
#define JSON_INVALID_STRING NULL
#define JSON_INVALID_NUMBER 0x7FFFFFFF

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#define json_null NULL
#define json_uintmax_t uintmax_t
#define json_strtoumax strtoumax

enum json_type_e {
    json_type_string,
    json_type_number,
    json_type_object,
    json_type_array,
    json_type_true,
    json_type_false,
    json_type_null
};

struct json_string_s {
    char *string;
    size_t string_size;
    size_t ref_count;
};

struct json_string_ex_s {
    struct json_string_s string;
    size_t offset;
    size_t line_no;
    size_t row_no;
};

struct json_number_s {
    char *number;
    size_t number_size;
    size_t ref_count;
};

struct json_object_element_s {
    struct json_string_s *name;
    struct json_value_s *value;
    struct json_object_element_s *next;
    size_t ref_count;
};

struct json_object_s {
    struct json_object_element_s *start;
    size_t length;
    size_t ref_count;
};

struct json_array_element_s {
    struct json_value_s *value;
    struct json_array_element_s *next;
    size_t ref_count;
};

struct json_array_s {
    struct json_array_element_s *start;
    size_t length;
    size_t ref_count;
};

struct json_value_s {
    void *payload;
    size_t type;
    size_t ref_count;
};

struct json_value_ex_s {
    struct json_value_s value;
    size_t offset;
    size_t line_no;
    size_t row_no;
};

// numbers
struct json_number_s *json_number_new_int(int number);
struct json_number_s *json_number_new_double(double number);
void json_number_free(struct json_number_s *num);

// strings
struct json_string_s *json_string_new(const char *string);
void json_string_free(struct json_string_s *str);

// arrays
struct json_array_s *json_array_new(void);
void json_array_free(struct json_array_s *arr);
void json_array_element_free(struct json_array_element_s *element);

void json_array_append(struct json_array_s *arr, struct json_value_s *value);
void json_array_append_bool(struct json_array_s *arr, int b);
void json_array_append_int(struct json_array_s *arr, int number);
void json_array_append_double(struct json_array_s *arr, double number);
void json_array_append_string(struct json_array_s *arr, const char *string);
void json_array_append_array(
    struct json_array_s *arr, struct json_array_s *arr2);
void json_array_append_object(
    struct json_array_s *arr, struct json_object_s *obj);

struct json_value_s *json_array_get_value(
    struct json_array_s *arr, const size_t idx);
int json_array_get_bool(struct json_array_s *arr, const size_t idx, int d);
int json_array_get_int(struct json_array_s *arr, const size_t idx, int d);
double json_array_get_double(
    struct json_array_s *arr, const size_t idx, double d);
const char *json_array_get_string(
    struct json_array_s *arr, const size_t idx, const char *d);
struct json_array_s *json_array_get_array(
    struct json_array_s *arr, const size_t idx);
struct json_object_s *json_array_get_object(
    struct json_array_s *arr, const size_t idx);

// objects
struct json_object_s *json_object_new(void);
void json_object_free(struct json_object_s *obj);
void json_object_element_free(struct json_object_element_s *element);

void json_object_append(
    struct json_object_s *obj, const char *key, struct json_value_s *value);
void json_object_append_bool(struct json_object_s *obj, const char *key, int b);
void json_object_append_int(
    struct json_object_s *obj, const char *key, int number);
void json_object_append_double(
    struct json_object_s *obj, const char *key, double number);
void json_object_append_string(
    struct json_object_s *obj, const char *key, const char *string);
void json_object_append_array(
    struct json_object_s *obj, const char *key, struct json_array_s *arr);
void json_object_append_object(
    struct json_object_s *obj, const char *key, struct json_object_s *obj2);

void json_object_evict_key(struct json_object_s *obj, const char *key);

struct json_value_s *json_object_get_value(
    struct json_object_s *obj, const char *key);
int json_object_get_bool(struct json_object_s *obj, const char *key, int d);
int json_object_get_int(struct json_object_s *obj, const char *key, int d);
double json_object_get_double(
    struct json_object_s *obj, const char *key, double d);
const char *json_object_get_string(
    struct json_object_s *obj, const char *key, const char *d);
struct json_array_s *json_object_get_array(
    struct json_object_s *obj, const char *key);
struct json_object_s *json_object_get_object(
    struct json_object_s *obj, const char *key);

// values
struct json_string_s *json_value_as_string(struct json_value_s *const value);
struct json_number_s *json_value_as_number(struct json_value_s *const value);
struct json_object_s *json_value_as_object(struct json_value_s *const value);
struct json_array_s *json_value_as_array(struct json_value_s *const value);
int json_value_is_true(const struct json_value_s *const value);
int json_value_is_false(const struct json_value_s *const value);
int json_value_is_null(const struct json_value_s *const value);

struct json_value_s *json_value_from_bool(int b);
struct json_value_s *json_value_from_number(struct json_number_s *num);
struct json_value_s *json_value_from_string(struct json_string_s *str);
struct json_value_s *json_value_from_array(struct json_array_s *arr);
struct json_value_s *json_value_from_object(struct json_object_s *obj);

void json_value_free(struct json_value_s *value);
