#include <trx/json/util/read_io.h>

#include <trx/debug.h>
#include <trx/json.h>
#include <trx/memory.h>
#include <trx/strings.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define M_MAX_STACK_SIZE 10

typedef struct JSON_READ_IO {
    char path[256];
    int32_t path_index_stack[M_MAX_STACK_SIZE];
    int32_t path_top;
    char error_msg[256];
    JSON_VALUE *stack[M_MAX_STACK_SIZE];
    JSON_VALUE *current;
    size_t current_pos;
    uint16_t version;
} JSON_READ_IO;

static void M_SetError(JSON_READ_IO *const io, const char *fmt, ...)
{
    char body[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(body, sizeof(body), fmt, ap);
    va_end(ap);
    if (io && io->path[0] != '\0') {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
        snprintf(
            io->error_msg, sizeof(io->error_msg), "%s: %s", io->path, body);
#pragma GCC diagnostic pop
    } else {
        snprintf(io->error_msg, sizeof(io->error_msg), "%s", body);
    }
}

static bool M_PushPathKey(JSON_READ_IO *const io, const char *const key)
{
    if (io->path_top + 1 >= M_MAX_STACK_SIZE) {
        return false;
    }
    const size_t pos = strlen(io->path);
    io->path_index_stack[io->path_top++] = pos;
    if (pos != 0) {
        strncat(io->path, ".", sizeof(io->path) - strlen(io->path) - 1);
    }
    strncat(io->path, key, sizeof(io->path) - strlen(io->path) - 1);
    return true;
}

static bool M_PushPathIndex(JSON_READ_IO *const io, const size_t idx)
{
    if (io->path_top + 1 >= M_MAX_STACK_SIZE) {
        return false;
    }
    io->path_index_stack[io->path_top++] = strlen(io->path);
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "[%zu]", idx);
    strncat(io->path, tmp, sizeof(io->path) - strlen(io->path) - 1);
    return true;
}

static void M_PopPath(JSON_READ_IO *const io)
{
    if (io->path_top <= 0) {
        io->path[0] = '\0';
        io->path_top = 0;
        return;
    }
    int pos = io->path_index_stack[--io->path_top];
    io->path[pos] = '\0';
}

static bool M_PushValue(JSON_READ_IO *const io, JSON_VALUE *const value)
{
    if (value == nullptr) {
        M_SetError(io, "pushing null value");
        return false;
    }
    if (io->current_pos + 1 >= M_MAX_STACK_SIZE) {
        M_SetError(io, "stack overflow");
        return false;
    }
    io->current_pos++;
    io->stack[io->current_pos] = value;
    io->current = io->stack[io->current_pos];
    return true;
}

static bool M_ReadBoolDirect(JSON_READ_IO *const io, bool *const target)
{
    if (JSON_ValueIsTrue(io->current)) {
        *target = true;
        return true;
    } else if (JSON_ValueIsFalse(io->current)) {
        *target = false;
        return true;
    } else {
        M_SetError(io, "not a bool");
        return false;
    }
}

#define L_DEFINE_M_READ_NUM_DIRECT(type_, name, minv, maxv)                    \
    static bool M_ReadNumDirect_##name(                                        \
        JSON_READ_IO *const io, void *const target)                            \
    {                                                                          \
        if (io->current->type != JSON_TYPE_NUMBER) {                           \
            M_SetError(io, "not a number");                                    \
            return false;                                                      \
        }                                                                      \
        const long long val = JSON_ValueGetInt(io->current, 0);                \
        if (val < (long long)(minv) || val > (long long)(maxv)) {              \
            M_SetError(io, "value out of range: %lld", val);                   \
            return false;                                                      \
        }                                                                      \
        *(type_ *)target = (type_)val;                                         \
        return true;                                                           \
    }
L_DEFINE_M_READ_NUM_DIRECT(int8_t, S8, INT8_MIN, INT8_MAX)
L_DEFINE_M_READ_NUM_DIRECT(int16_t, S16, INT16_MIN, INT16_MAX)
L_DEFINE_M_READ_NUM_DIRECT(int32_t, S32, INT32_MIN, INT32_MAX)
L_DEFINE_M_READ_NUM_DIRECT(uint8_t, U8, 0, UINT8_MAX)
L_DEFINE_M_READ_NUM_DIRECT(uint16_t, U16, 0, UINT16_MAX)
L_DEFINE_M_READ_NUM_DIRECT(uint32_t, U32, 0, UINT32_MAX)
#undef L_DEFINE_M_READ_NUM_DIRECT

static bool M_ReadNumDirect_Double(JSON_READ_IO *const io, double *const target)
{
    if (io->current->type != JSON_TYPE_NUMBER) {
        M_SetError(io, "not a number");
        return false;
    }
    const double val = JSON_ValueGetDouble(io->current, -1.0);
    *(double *)target = val;
    return true;
}

bool JSON_ReadIO_ReadValueDirect(
    JSON_READ_IO *const io, const JSON_READ_TYPE type, void *const target)
{
    switch (type) {
    case JSON_READ_BOOL:
        return M_ReadBoolDirect(io, target);
    case JSON_READ_S8:
        return M_ReadNumDirect_S8(io, target);
    case JSON_READ_U8:
        return M_ReadNumDirect_U8(io, target);
    case JSON_READ_S16:
        return M_ReadNumDirect_S16(io, target);
    case JSON_READ_U16:
        return M_ReadNumDirect_U16(io, target);
    case JSON_READ_S32:
        return M_ReadNumDirect_S32(io, target);
    case JSON_READ_U32:
        return M_ReadNumDirect_U32(io, target);
    case JSON_READ_DOUBLE:
        return M_ReadNumDirect_Double(io, target);
    }
    M_SetError(io, "unsupported read type");
    return false;
}

bool JSON_ReadIO_ReadValue(
    JSON_READ_IO *const io, const char *const key, const JSON_READ_TYPE type,
    void *const target)
{
    if (!JSON_ReadIO_PushObject(io, key)) {
        return false;
    }
    const bool success = JSON_ReadIO_ReadValueDirect(io, type, target);
    JSON_ReadIO_Pop(io);
    return success;
}

const char *JSON_ReadIO_GetError(const JSON_READ_IO *const io)
{
    return io->error_msg;
}

uint16_t JSON_ReadIO_GetVersion(const JSON_READ_IO *const io)
{
    return io->version;
}

void JSON_ReadIO_SetError(JSON_READ_IO *const io, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char body[256];
    vsnprintf(body, sizeof(body), fmt, ap);
    va_end(ap);
    M_SetError(io, "%s", body);
}

bool JSON_ReadIO_PushObject(JSON_READ_IO *const io, const char *const key)
{
    JSON_OBJECT *const current_obj = JSON_ValueAsObject(io->current);
    if (current_obj == nullptr) {
        M_SetError(io, "not an object");
        return false;
    }
    JSON_VALUE *const child = JSON_ObjectGetValue(current_obj, key);
    if (child == nullptr) {
        M_SetError(io, "key does not exist: %s", key);
        return false;
    }
    if (!M_PushPathKey(io, key)) {
        M_SetError(io, "path depth overflow");
        return false;
    }
    return M_PushValue(io, child);
}

bool JSON_ReadIO_PushArrayElem(JSON_READ_IO *const io, const size_t index)
{
    JSON_ARRAY *const current_arr = JSON_ValueAsArray(io->current);
    if (current_arr == nullptr) {
        M_SetError(io, "not an array");
        return false;
    }
    JSON_VALUE *const child = JSON_ArrayGetValue(current_arr, index);
    if (child == nullptr) {
        M_SetError(io, "invalid array index");
        return false;
    }
    if (!M_PushPathIndex(io, index)) {
        M_SetError(io, "path depth overflow");
        return false;
    }
    return M_PushValue(io, child);
}

bool JSON_ReadIO_Pop(JSON_READ_IO *const io)
{
    if (io->current_pos == 0) {
        M_SetError(io, "pop from empty stack");
        return false;
    }
    io->current_pos--;
    io->current = io->stack[io->current_pos];
    M_PopPath(io);
    return true;
}

int32_t JSON_ReadIO_GetArrayLength(JSON_READ_IO *const io)
{
    JSON_ARRAY *const arr = JSON_ValueAsArray(io->current);
    if (arr == nullptr) {
        M_SetError(io, "not an array");
        return false;
    }
    return arr->length;
}

bool JSON_ReadIO_HasKey(JSON_READ_IO *const io, const char *const key)
{
    JSON_OBJECT *const obj = JSON_ValueAsObject(io->current);
    if (obj == nullptr) {
        return false;
    }
    return JSON_ObjectContainsKey(obj, key);
}

JSON_OBJECT *JSON_ReadIO_GetCurrentObject(JSON_READ_IO *const io)
{
    return JSON_ValueAsObject(io->current);
}

JSON_READ_IO *JSON_ReadIO_Create(JSON_VALUE *const root, const uint16_t version)
{
    JSON_READ_IO *const io = Memory_Alloc(sizeof(*io));
    io->stack[0] = root;
    io->current_pos = 0;
    io->current = io->stack[0];
    io->version = version;
    return io;
}

void JSON_ReadIO_Destroy(JSON_READ_IO *const io, const bool success)
{
    if (!success && io->error_msg[0] != '\0') {
        LOG_ERROR("%s", io->error_msg);
    }
    Memory_Free(io);
}
