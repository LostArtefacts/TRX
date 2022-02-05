#include "json/bson_parse.h"

#include "log.h"
#include "memory.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

struct bson_parse_state_s {
    const char *src;
    size_t size;
    size_t offset;

    char *data;
    char *dom;
    size_t dom_size;
    size_t data_size;

    size_t error;
};

static bool bson_parse_get_object_key_size(struct bson_parse_state_s *state);
static bool bson_parse_get_null_value_size(struct bson_parse_state_s *state);
static bool bson_parse_get_bool_value_size(struct bson_parse_state_s *state);
static bool bson_parse_get_int32_value_size(struct bson_parse_state_s *state);
static bool bson_parse_get_double_value_size(struct bson_parse_state_s *state);
static bool bson_parse_get_string_value_size(struct bson_parse_state_s *state);
static bool bson_parse_get_array_element_wrapped_size(
    struct bson_parse_state_s *state);
static bool bson_parse_get_array_size(struct bson_parse_state_s *state);
static bool bson_parse_get_array_value_size(struct bson_parse_state_s *state);
static bool bson_parse_get_object_element_wrapped_size(
    struct bson_parse_state_s *state);
static bool bson_parse_get_object_size(struct bson_parse_state_s *state);
static bool bson_parse_get_object_value_size(struct bson_parse_state_s *state);
static bool bson_parse_get_value_size(
    struct bson_parse_state_s *state, uint8_t marker);
static bool bson_parse_get_root_size(struct bson_parse_state_s *state);

static void bson_parse_object_key(
    struct bson_parse_state_s *state, struct json_string_s *string);
static void bson_parse_null_value(
    struct bson_parse_state_s *state, struct json_value_s *value);
static void bson_parse_bool_value(
    struct bson_parse_state_s *state, struct json_value_s *value);
static void bson_parse_int32_value(
    struct bson_parse_state_s *state, struct json_value_s *value);
static void bson_parse_double_value(
    struct bson_parse_state_s *state, struct json_value_s *value);
static void bson_parse_string_value(
    struct bson_parse_state_s *state, struct json_value_s *value);
static void bson_parse_array_element_wrapped(
    struct bson_parse_state_s *state, struct json_array_element_s *element);
static void bson_parse_array(
    struct bson_parse_state_s *state, struct json_array_s *array);
static void bson_parse_array_value(
    struct bson_parse_state_s *state, struct json_value_s *value);
static void bson_parse_object_element_wrapped(
    struct bson_parse_state_s *state, struct json_object_element_s *element);
static void bson_parse_object(
    struct bson_parse_state_s *state, struct json_object_s *object);
static void bson_parse_object_value(
    struct bson_parse_state_s *state, struct json_value_s *value);
static void bson_parse_value(
    struct bson_parse_state_s *state, struct json_value_s *value,
    uint8_t marker);

static bool bson_parse_get_object_key_size(struct bson_parse_state_s *state)
{
    assert(state);
    while (state->src[state->offset]) {
        state->data_size++;
        state->offset++;
    }
    state->data_size++;
    state->offset++;
    return true;
}

static bool bson_parse_get_null_value_size(struct bson_parse_state_s *state)
{
    assert(state);
    return true;
}

static bool bson_parse_get_bool_value_size(struct bson_parse_state_s *state)
{
    assert(state);
    if (state->offset + sizeof(uint8_t) > state->size) {
        state->error = bson_parse_error_premature_end_of_buffer;
        return false;
    }

    switch (state->src[state->offset]) {
    case 0x00:
        break;
    case 0x01:
        break;
    default:
        state->error = bson_parse_error_invalid_value;
        return false;
    }

    state->offset++;
    return true;
}

static bool bson_parse_get_int32_value_size(struct bson_parse_state_s *state)
{
    assert(state);

    if (state->offset + sizeof(int32_t) > state->size) {
        state->error = bson_parse_error_premature_end_of_buffer;
        return false;
    }
    int32_t num = *(int32_t *)&state->src[state->offset];
    state->offset += sizeof(int32_t);

    state->dom_size += sizeof(struct json_number_s);
    state->data_size += snprintf(NULL, 0, "%d", num) + 1;
    return true;
}

static bool bson_parse_get_double_value_size(struct bson_parse_state_s *state)
{
    assert(state);

    if (state->offset + sizeof(double) > state->size) {
        state->error = bson_parse_error_premature_end_of_buffer;
        return false;
    }
    double num = *(double *)&state->src[state->offset];
    state->offset += sizeof(double);

    state->dom_size += sizeof(struct json_number_s);
    state->data_size += snprintf(NULL, 0, "%f", num) + 1;
    return true;
}

static bool bson_parse_get_string_value_size(struct bson_parse_state_s *state)
{
    assert(state);

    if (state->offset + sizeof(int32_t) > state->size) {
        state->error = bson_parse_error_premature_end_of_buffer;
        return false;
    }
    int32_t size = *(int32_t *)&state->src[state->offset];
    state->offset += sizeof(int32_t);
    if (state->offset + size > state->size) {
        state->error = bson_parse_error_premature_end_of_buffer;
        return false;
    }
    if (state->src[state->offset + size - 1] != '\0') {
        state->error = bson_parse_error_invalid_value;
        return false;
    }
    state->offset += size;
    state->dom_size += sizeof(struct json_string_s);
    state->data_size += size;
    return true;
}

static bool bson_parse_get_array_element_wrapped_size(
    struct bson_parse_state_s *state)
{
    assert(state);

    if (state->offset + sizeof(uint8_t) > state->size) {
        state->error = bson_parse_error_premature_end_of_buffer;
        return false;
    }
    uint8_t marker = state->src[state->offset];
    state->offset++;

    // BSON arrays always use keys
    state->dom_size += sizeof(struct json_string_s);
    if (!bson_parse_get_object_key_size(state)) {
        return false;
    }

    state->dom_size += sizeof(struct json_value_s);
    return bson_parse_get_value_size(state, marker);
}

static bool bson_parse_get_array_size(struct bson_parse_state_s *state)
{
    assert(state);

    const size_t start_offset = state->offset;
    if (state->offset + sizeof(int32_t) > state->size) {
        state->error = bson_parse_error_premature_end_of_buffer;
        return false;
    }
    const int size = *(int32_t *)&state->src[state->offset];
    state->offset += sizeof(int32_t);

    while (state->offset < start_offset + size - 1) {
        state->dom_size += sizeof(struct json_array_element_s);
        if (!bson_parse_get_array_element_wrapped_size(state)) {
            return false;
        }
    }

    if (state->offset + sizeof(char) > state->size) {
        state->error = bson_parse_error_premature_end_of_buffer;
        return false;
    }
    if (state->src[state->offset] != '\0') {
        state->error = bson_parse_error_invalid_value;
        return false;
    }
    state->offset++;
    return true;
}

static bool bson_parse_get_array_value_size(struct bson_parse_state_s *state)
{
    assert(state);
    state->dom_size += sizeof(struct json_array_s);
    return bson_parse_get_array_size(state);
}

static bool bson_parse_get_object_element_wrapped_size(
    struct bson_parse_state_s *state)
{
    assert(state);

    if (state->offset + sizeof(uint8_t) > state->size) {
        state->error = bson_parse_error_premature_end_of_buffer;
        return false;
    }
    uint8_t marker = state->src[state->offset];
    state->offset++;

    state->dom_size += sizeof(struct json_string_s);
    if (!bson_parse_get_object_key_size(state)) {
        return false;
    }

    state->dom_size += sizeof(struct json_value_s);
    return bson_parse_get_value_size(state, marker);
}

static bool bson_parse_get_object_size(struct bson_parse_state_s *state)
{
    assert(state);

    const size_t start_offset = state->offset;
    if (state->offset + sizeof(int32_t) > state->size) {
        state->error = bson_parse_error_premature_end_of_buffer;
        return false;
    }
    const int size = *(int32_t *)&state->src[state->offset];
    state->offset += sizeof(int32_t);

    while (state->offset < start_offset + size - 1) {
        state->dom_size += sizeof(struct json_object_element_s);
        if (!bson_parse_get_object_element_wrapped_size(state)) {
            return false;
        }
    }

    if (state->offset + sizeof(char) > state->size) {
        state->error = bson_parse_error_premature_end_of_buffer;
        return false;
    }
    if (state->src[state->offset] != '\0') {
        state->error = bson_parse_error_invalid_value;
        return false;
    }
    state->offset++;
    return true;
}

static bool bson_parse_get_object_value_size(struct bson_parse_state_s *state)
{
    assert(state);
    state->dom_size += sizeof(struct json_object_s);
    return bson_parse_get_object_size(state);
}

static bool bson_parse_get_value_size(
    struct bson_parse_state_s *state, uint8_t marker)
{
    assert(state);
    switch (marker) {
    case 0x01:
        return bson_parse_get_double_value_size(state);
    case 0x02:
        return bson_parse_get_string_value_size(state);
    case 0x03:
        return bson_parse_get_object_value_size(state);
    case 0x04:
        return bson_parse_get_array_value_size(state);
    case 0x0A:
        return bson_parse_get_null_value_size(state);
    case 0x08:
        return bson_parse_get_bool_value_size(state);
    case 0x10:
        return bson_parse_get_int32_value_size(state);
    default:
        state->error = bson_parse_error_invalid_value;
        return false;
    }
}

static bool bson_parse_get_root_size(struct bson_parse_state_s *state)
{
    // assume the root element to be an object
    state->dom_size += sizeof(struct json_value_s);
    return bson_parse_get_object_value_size(state);
}

static void bson_parse_object_key(
    struct bson_parse_state_s *state, struct json_string_s *string)
{
    assert(state);
    assert(string);
    size_t size = 0;
    string->ref_count = 1;
    string->string = state->data;
    while (state->src[state->offset]) {
        state->data[size++] = state->src[state->offset++];
    }
    string->string_size = size;
    state->data[size++] = state->src[state->offset++];
    state->data += size;
}

static void bson_parse_null_value(
    struct bson_parse_state_s *state, struct json_value_s *value)
{
    assert(state);
    assert(value);
    value->type = json_type_null;
    value->payload = json_null;
}

static void bson_parse_bool_value(
    struct bson_parse_state_s *state, struct json_value_s *value)
{
    assert(state);
    assert(value);
    assert(state->offset + sizeof(char) <= state->size);
    switch (state->src[state->offset]) {
    case 0x00:
        value->type = json_type_false;
        value->payload = json_null;
        break;
    case 0x01:
        value->type = json_type_true;
        value->payload = json_null;
        break;
    default:
        assert(0);
    }
    state->offset++;
}

static void bson_parse_int32_value(
    struct bson_parse_state_s *state, struct json_value_s *value)
{
    assert(state);
    assert(value);

    assert(state->offset + sizeof(int32_t) <= state->size);
    int32_t num = *(int32_t *)&state->src[state->offset];
    state->offset += sizeof(int32_t);

    struct json_number_s *number = (struct json_number_s *)state->dom;
    number->ref_count = 1;
    state->dom += sizeof(struct json_number_s);

    number->number = state->data;
    sprintf(state->data, "%d", num);
    number->number_size = strlen(number->number);
    state->data += number->number_size + 1;

    value->type = json_type_number;
    value->payload = number;
}

static void bson_parse_double_value(
    struct bson_parse_state_s *state, struct json_value_s *value)
{
    assert(state);
    assert(value);

    assert(state->offset + sizeof(double) <= state->size);
    double num = *(double *)&state->src[state->offset];
    state->offset += sizeof(double);

    struct json_number_s *number = (struct json_number_s *)state->dom;
    number->ref_count = 1;
    state->dom += sizeof(struct json_number_s);

    number->number = state->data;
    sprintf(state->data, "%f", num);
    number->number_size = strlen(number->number);
    state->data += number->number_size + 1;

    // strip trailing zeroes after decimal point
    if (strchr(number->number, '.')) {
        while (number->number[number->number_size - 1] == '0'
               && number->number_size > 1) {
            number->number_size--;
        }
        number->number[number->number_size] = '\0';
    }

    value->type = json_type_number;
    value->payload = number;
}

static void bson_parse_string_value(
    struct bson_parse_state_s *state, struct json_value_s *value)
{
    assert(state);
    assert(value);

    assert(state->offset + sizeof(int32_t) <= state->size);
    int32_t size = *(int32_t *)&state->src[state->offset];
    state->offset += sizeof(int32_t);

    struct json_string_s *string = (struct json_string_s *)state->dom;
    string->ref_count = 1;
    state->dom += sizeof(struct json_string_s);

    memcpy(state->data, state->src + state->offset, size);
    state->offset += size;

    string->string = state->data;
    string->string_size = size;
    state->data += size;

    value->type = json_type_string;
    value->payload = string;
}

static void bson_parse_array_element_wrapped(
    struct bson_parse_state_s *state, struct json_array_element_s *element)
{
    assert(state);
    assert(element);

    assert(state->offset + sizeof(uint8_t) <= state->size);
    uint8_t marker = state->src[state->offset];
    state->offset++;

    // BSON arrays always use keys
    struct json_string_s *key = (struct json_string_s *)state->dom;
    key->ref_count = 1;
    state->dom += sizeof(struct json_string_s);
    bson_parse_object_key(state, key);

    struct json_value_s *value = (struct json_value_s *)state->dom;
    value->ref_count = 1;
    state->dom += sizeof(struct json_value_s);

    element->value = value;

    bson_parse_value(state, value, marker);
}

static void bson_parse_array(
    struct bson_parse_state_s *state, struct json_array_s *array)
{
    assert(state);
    assert(array);

    const size_t start_offset = state->offset;
    assert(state->offset + sizeof(int32_t) <= state->size);
    const int size = *(int32_t *)&state->src[state->offset];
    state->offset += sizeof(int32_t);

    struct json_array_element_s *previous = NULL;
    int count = 0;
    while (state->offset < start_offset + size - 1) {
        struct json_array_element_s *element =
            (struct json_array_element_s *)state->dom;
        state->dom += sizeof(struct json_array_element_s);
        if (!previous) {
            array->start = element;
        } else {
            previous->next = element;
        }
        previous = element;
        bson_parse_array_element_wrapped(state, element);
        count++;
    }
    if (previous) {
        previous->next = NULL;
    }
    if (!count) {
        array->start = NULL;
    }
    array->ref_count = 1;
    array->length = count;
    assert(state->offset + sizeof(char) <= state->size);
    assert(!state->src[state->offset]);
    state->offset++;
}

static void bson_parse_array_value(
    struct bson_parse_state_s *state, struct json_value_s *value)
{
    assert(state);
    assert(value);

    struct json_array_s *array = (struct json_array_s *)state->dom;
    array->ref_count = 1;
    state->dom += sizeof(struct json_array_s);

    bson_parse_array(state, array);

    value->type = json_type_array;
    value->payload = array;
}

static void bson_parse_object_element_wrapped(
    struct bson_parse_state_s *state, struct json_object_element_s *element)
{
    assert(state);
    assert(element);

    assert(state->offset + sizeof(uint8_t) <= state->size);
    uint8_t marker = state->src[state->offset];
    state->offset++;

    struct json_string_s *key = (struct json_string_s *)state->dom;
    key->ref_count = 1;
    state->dom += sizeof(struct json_string_s);
    bson_parse_object_key(state, key);

    struct json_value_s *value = (struct json_value_s *)state->dom;
    value->ref_count = 1;
    state->dom += sizeof(struct json_value_s);

    element->name = key;
    element->value = value;

    bson_parse_value(state, value, marker);
}

static void bson_parse_object(
    struct bson_parse_state_s *state, struct json_object_s *object)
{
    assert(state);
    assert(object);

    const size_t start_offset = state->offset;
    assert(state->offset + sizeof(int32_t) <= state->size);
    const int size = *(int32_t *)&state->src[state->offset];
    state->offset += sizeof(int32_t);

    struct json_object_element_s *previous = NULL;
    int count = 0;
    while (state->offset < start_offset + size - 1) {
        struct json_object_element_s *element =
            (struct json_object_element_s *)state->dom;
        state->dom += sizeof(struct json_object_element_s);
        if (!previous) {
            object->start = element;
        } else {
            previous->next = element;
        }
        previous = element;
        bson_parse_object_element_wrapped(state, element);
        count++;
    }
    if (previous) {
        previous->next = NULL;
    }
    if (!count) {
        object->start = NULL;
    }
    object->ref_count = 1;
    object->length = count;
    assert(state->offset + sizeof(char) <= state->size);
    assert(!state->src[state->offset]);
    state->offset++;
}

static void bson_parse_object_value(
    struct bson_parse_state_s *state, struct json_value_s *value)
{
    assert(state);
    assert(value);

    struct json_object_s *object = (struct json_object_s *)state->dom;
    object->ref_count = 1;
    state->dom += sizeof(struct json_object_s);

    bson_parse_object(state, object);

    value->type = json_type_object;
    value->payload = object;
}

static void bson_parse_value(
    struct bson_parse_state_s *state, struct json_value_s *value,
    uint8_t marker)
{
    assert(state);
    assert(value);
    switch (marker) {
    case 0x01:
        bson_parse_double_value(state, value);
        break;
    case 0x02:
        bson_parse_string_value(state, value);
        break;
    case 0x03:
        bson_parse_object_value(state, value);
        break;
    case 0x04:
        bson_parse_array_value(state, value);
        break;
    case 0x0A:
        bson_parse_null_value(state, value);
        break;
    case 0x08:
        bson_parse_bool_value(state, value);
        break;
    case 0x10:
        bson_parse_int32_value(state, value);
        break;
    default:
        assert(0);
    }
}

struct json_value_s *bson_parse(const char *src, size_t src_size)
{
    return bson_parse_ex(src, src_size, NULL);
}

struct json_value_s *bson_parse_ex(
    const char *src, size_t src_size, struct bson_parse_result_s *result)
{
    struct bson_parse_state_s state;
    void *allocation;
    struct json_value_s *value;
    size_t total_size;

    if (result) {
        result->error = bson_parse_error_none;
        result->error_offset = 0;
    }

    if (!src) {
        return NULL;
    }

    state.src = src;
    state.size = src_size;
    state.offset = 0;
    state.error = bson_parse_error_none;
    state.dom_size = 0;
    state.data_size = 0;

    if (bson_parse_get_root_size(&state)) {
        if (state.offset != state.size) {
            state.error = bson_parse_error_unexpected_trailing_bytes;
        }
    }

    if (state.error != bson_parse_error_none) {
        if (result) {
            result->error = state.error;
            result->error_offset = state.offset;
        }
        LOG_ERROR(
            "Error while reading BSON near offset %d: %s", state.offset,
            bson_get_error_description(state.error));
        return NULL;
    }

    total_size = state.dom_size + state.data_size;

    allocation = Memory_Alloc(total_size);
    state.offset = 0;
    state.dom = (char *)allocation;
    state.data = state.dom + state.dom_size;

    // assume the root element to be an object
    value = (struct json_value_s *)state.dom;
    value->ref_count = 0;
    state.dom += sizeof(struct json_value_s);
    bson_parse_object_value(&state, value);

    assert(state.dom == allocation + state.dom_size);
    assert(state.data == allocation + state.dom_size + state.data_size);

    return value;
}

const char *bson_get_error_description(enum bson_parse_error_e error)
{
    switch (error) {
    case bson_parse_error_none:
        return "no error";

    case bson_parse_error_invalid_value:
        return "invalid value";

    case bson_parse_error_premature_end_of_buffer:
        return "premature end of buffer";

    case bson_parse_error_unexpected_trailing_bytes:
        return "unexpected trailing bytes";

    case bson_parse_error_unknown:
    default:
        return "unknown";
    }
}
