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

static bool M_ReadBoolCurrent(JSON_READ_IO *const io, bool *const target)
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

#define L_DEFINE_M_READ_NUM_CURRENT(type_, name, minv, maxv)                   \
    static bool M_ReadNumCurrent_##name(                                       \
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
L_DEFINE_M_READ_NUM_CURRENT(int8_t, S8, INT8_MIN, INT8_MAX)
L_DEFINE_M_READ_NUM_CURRENT(int16_t, S16, INT16_MIN, INT16_MAX)
L_DEFINE_M_READ_NUM_CURRENT(int32_t, S32, INT32_MIN, INT32_MAX)
L_DEFINE_M_READ_NUM_CURRENT(uint8_t, U8, 0, UINT8_MAX)
L_DEFINE_M_READ_NUM_CURRENT(uint16_t, U16, 0, UINT16_MAX)
L_DEFINE_M_READ_NUM_CURRENT(uint32_t, U32, 0, UINT32_MAX)
#undef L_DEFINE_M_READ_NUM_CURRENT

static bool M_ReadNumCurrent_Double(
    JSON_READ_IO *const io, double *const target)
{
    if (io->current->type != JSON_TYPE_NUMBER) {
        M_SetError(io, "not a number");
        return false;
    }
    const double val = JSON_ValueGetDouble(io->current, -1.0);
    *(double *)target = val;
    return true;
}

static bool M_ReadNumCurrent_Float(JSON_READ_IO *const io, float *const target)
{
    if (io->current->type != JSON_TYPE_NUMBER) {
        M_SetError(io, "not a number");
        return false;
    }
    const double val = JSON_ValueGetDouble(io->current, -1.0);
    *target = (float)val;
    return true;
}

static bool M_ReadStringCurrent(
    JSON_READ_IO *const io, const char **const target)
{
    if (io->current->type != JSON_TYPE_STRING) {
        M_SetError(io, "not a string");
        return false;
    }
    *target = JSON_ValueGetString(io->current, nullptr);
    return *target != nullptr;
}

bool JSON_ReadIO_ReadXYZ32Current(
    JSON_READ_IO *const io, void *const target_void)
{
    XYZ_32 *const target = target_void;
    JSON_ARRAY *const tuple = JSON_ValueAsArray(io->current);
    if (tuple != nullptr) {
        const int32_t tuple_len = tuple->length;
        if (tuple_len != 3) {
            M_SetError(io, "XYZ tuple must have exactly 3 values");
            JSON_FAIL();
        }
        JSON_MUST(JSON_READ_A(io, 0, &target->x));
        JSON_MUST(JSON_READ_A(io, 1, &target->y));
        JSON_MUST(JSON_READ_A(io, 2, &target->z));
    } else {
        JSON_MUST(JSON_READ(io, "x", &target->x));
        JSON_MUST(JSON_READ(io, "y", &target->y));
        JSON_MUST(JSON_READ(io, "z", &target->z));
    }
    JSON_FINISH();
}

bool JSON_ReadIO_ReadXYZ16Current(
    JSON_READ_IO *const io, void *const target_void)
{
    XYZ_32 tmp;
    JSON_MUST(JSON_ReadIO_ReadXYZ32Current(io, &tmp));
    if (tmp.x < INT16_MIN || tmp.x > INT16_MAX || tmp.y < INT16_MIN
        || tmp.y > INT16_MAX || tmp.z < INT16_MIN || tmp.z > INT16_MAX) {
        M_SetError(io, "XYZ16 value out of range");
        JSON_FAIL();
    }
    XYZ_16 *const target = target_void;
    target->x = tmp.x;
    target->y = tmp.y;
    target->z = tmp.z;
    JSON_FINISH();
}

static bool M_ReadRGB888Current(JSON_READ_IO *const io, RGB_888 *const target)
{
    JSON_ARRAY *const tuple = JSON_ValueAsArray(io->current);
    if (tuple != nullptr) {
        const int32_t tuple_len = tuple->length;
        if (tuple_len != 3) {
            M_SetError(io, "RGB array must have exactly 3 values");
            JSON_FAIL();
        }

        RGB_F color = { -1.0f, -1.0f, -1.0f };
        JSON_MUST(JSON_READ_A(io, 0, &color.r));
        JSON_MUST(JSON_READ_A(io, 1, &color.g));
        JSON_MUST(JSON_READ_A(io, 2, &color.b));
        if (color.r < 0.0f || color.g < 0.0f || color.b < 0.0f || color.r > 1.0f
            || color.g > 1.0f || color.b > 1.0f) {
            M_SetError(io, "RGB array values must be in range 0.0..1.0");
            JSON_FAIL();
        }

        *target = (RGB_888) {
            (uint8_t)(color.r * 255.0f),
            (uint8_t)(color.g * 255.0f),
            (uint8_t)(color.b * 255.0f),
        };
    } else {
        const char *str = nullptr;
        JSON_MUST(JSON_READ_CURRENT(io, &str));
        if (!String_ParseRGB888(str, target)) {
            M_SetError(io, "invalid RGB color string");
            JSON_FAIL();
        }
    }

    JSON_FINISH();
}

#define L_DEFINE_JSON_READ_IO_TYPE(name, ctype, impl_func)                     \
    bool JSON_ReadIO_Read##name##Current(                                      \
        JSON_READ_IO *const io, void *const target)                            \
    {                                                                          \
        return impl_func(io, (ctype *)target);                                 \
    }
L_DEFINE_JSON_READ_IO_TYPE(Bool, bool, M_ReadBoolCurrent)
L_DEFINE_JSON_READ_IO_TYPE(S8, int8_t, M_ReadNumCurrent_S8)
L_DEFINE_JSON_READ_IO_TYPE(U8, uint8_t, M_ReadNumCurrent_U8)
L_DEFINE_JSON_READ_IO_TYPE(S16, int16_t, M_ReadNumCurrent_S16)
L_DEFINE_JSON_READ_IO_TYPE(U16, uint16_t, M_ReadNumCurrent_U16)
L_DEFINE_JSON_READ_IO_TYPE(S32, int32_t, M_ReadNumCurrent_S32)
L_DEFINE_JSON_READ_IO_TYPE(U32, uint32_t, M_ReadNumCurrent_U32)
L_DEFINE_JSON_READ_IO_TYPE(Float, float, M_ReadNumCurrent_Float)
L_DEFINE_JSON_READ_IO_TYPE(Double, double, M_ReadNumCurrent_Double)
L_DEFINE_JSON_READ_IO_TYPE(String, const char *, M_ReadStringCurrent)
L_DEFINE_JSON_READ_IO_TYPE(RGB888, RGB_888, M_ReadRGB888Current)
#undef L_DEFINE_JSON_READ_IO_TYPE

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
        return -1;
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

JSON_VALUE *JSON_ReadIO_GetCurrentValue(JSON_READ_IO *const io)
{
    return io->current;
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
