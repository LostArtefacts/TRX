#pragma once

#include <trx/core/colors.h>
#include <trx/core/json.h>
#include <trx/core/log.h>
#include <trx/core/math/types.h>
#include <trx/core/result.h>

#include <stddef.h>
#include <stdint.h>

typedef struct JSON_READ_IO JSON_READ_IO;

typedef struct {
    void *tmp;
} JSON_READ_IO_DUMMY;

#define JSON_READ_IO_TYPE_LIST_BASE(X)                                         \
    X(Bool, bool)                                                              \
    X(S8, int8_t)                                                              \
    X(U8, uint8_t)                                                             \
    X(S16, int16_t)                                                            \
    X(U16, uint16_t)                                                           \
    X(S32, int32_t)                                                            \
    X(U32, uint32_t)                                                           \
    X(S64, int64_t)                                                            \
    X(U64, uint64_t)                                                           \
    X(Float, float)                                                            \
    X(Double, double)                                                          \
    X(XYZ16, XYZ_16)                                                           \
    X(XYZ32, XYZ_32)                                                           \
    X(RGB888, RGB_888)                                                         \
    X(RGBA8888, RGBA_8888)                                                     \
    X(String, const char *)
#define JSON_READ_IO_TYPE_LIST JSON_READ_IO_TYPE_LIST_BASE

#define JSON_READ_IO_TYPE_TO_CURRENT_FN(name, ctype)                           \
    ctype:                                                                     \
    JSON_ReadIO_Read##name##Current,

// ============================================================================
// Public APIs

JSON_READ_IO *JSON_ReadIO_Create(
    JSON_VALUE *root, uint16_t version, const char *source_path);
void JSON_ReadIO_Destroy(JSON_READ_IO *io);

const char *JSON_ReadIO_GetError(const JSON_READ_IO *io);
const char *JSON_ReadIO_GetErrorPath(const JSON_READ_IO *io);
const char *JSON_ReadIO_GetErrorBody(const JSON_READ_IO *io);
int32_t JSON_ReadIO_GetErrorLine(const JSON_READ_IO *io);
int32_t JSON_ReadIO_GetErrorCol(const JSON_READ_IO *io);
uint16_t JSON_ReadIO_GetVersion(const JSON_READ_IO *io);
void JSON_ReadIO_FormatError(
    const JSON_READ_IO *io, bool multiline, char *buffer, size_t buffer_size);

void JSON_ReadIO_SetError(JSON_READ_IO *io, const char *fmt, ...);
void JSON_ReadIO_SetErrorAt(
    JSON_READ_IO *io, int32_t line, int32_t col, const char *fmt, ...);
RESULT JSON_ReadIO_PushObject(JSON_READ_IO *io, const char *key);
RESULT JSON_ReadIO_PushArrayElem(JSON_READ_IO *io, size_t index);
RESULT JSON_ReadIO_Pop(JSON_READ_IO *io);
int32_t JSON_ReadIO_GetArrayLength(JSON_READ_IO *io);
bool JSON_ReadIO_HasKey(JSON_READ_IO *io, const char *key);
JSON_OBJECT *JSON_ReadIO_GetCurrentObject(JSON_READ_IO *io);
JSON_VALUE *JSON_ReadIO_GetCurrentValue(JSON_READ_IO *io);

#define L_DECLARE_JSON_READ_IO_TYPE(name, ctype)                               \
    RESULT JSON_ReadIO_Read##name##Current(JSON_READ_IO *io, void *target);
JSON_READ_IO_TYPE_LIST(L_DECLARE_JSON_READ_IO_TYPE)
#undef L_DECLARE_JSON_READ_IO_TYPE

#define JSON_PUSH(io, key) JSON_ReadIO_PushObject((io), (key))
#define JSON_PUSH_INDEX(io, idx) JSON_ReadIO_PushArrayElem((io), (idx))
#define JSON_POP(io) JSON_ReadIO_Pop((io))
#define JSON_ARRAY_LEN(io) JSON_ReadIO_GetArrayLength((io))

// Reads the value into target_ptr from the value the stack is on.
#define JSON_READ_CURRENT(io, target_ptr)                                      \
    _Generic(                                                                  \
        *(target_ptr),                                                         \
        JSON_READ_IO_TYPE_LIST(JSON_READ_IO_TYPE_TO_CURRENT_FN)                \
            JSON_READ_IO_DUMMY: JSON_ReadIO_ReadS32Current)(                   \
        (io), (target_ptr))

typedef RESULT (*JSON_READ_IO_READ_FUNC)(JSON_READ_IO *io, void *target);

// Reads the value at the given key, or at the given index. Reports a key that
// is not there as well as one holding something other than what was asked
// for. The optional read reports only the latter, leaving target_ptr alone
// where the key is absent.
RESULT JSON_ReadIO_ReadKey(
    JSON_READ_IO *io, const char *key, void *target,
    JSON_READ_IO_READ_FUNC read_func);
RESULT JSON_ReadIO_ReadKeyOptional(
    JSON_READ_IO *io, const char *key, void *target,
    JSON_READ_IO_READ_FUNC read_func);
RESULT JSON_ReadIO_ReadIndex(
    JSON_READ_IO *io, size_t index, void *target,
    JSON_READ_IO_READ_FUNC read_func);

#define JSON_READ_IO_READ_FN_FOR(target_ptr)                                   \
    _Generic(                                                                  \
        *(target_ptr),                                                         \
        JSON_READ_IO_TYPE_LIST(JSON_READ_IO_TYPE_TO_CURRENT_FN)                \
            JSON_READ_IO_DUMMY: JSON_ReadIO_ReadS32Current)

// The key has to be there and hold what was asked for.
#define JSON_READ(io, key, target_ptr)                                         \
    JSON_ReadIO_ReadKey(                                                       \
        (io), (key), (target_ptr), JSON_READ_IO_READ_FN_FOR(target_ptr))

// The key may be absent, in which case the default stands. A key that is
// there still has to hold what was asked for.
#define JSON_READ_D(io, key, target_ptr, default_value)                        \
    (*(target_ptr) = (default_value),                                          \
     JSON_ReadIO_ReadKeyOptional(                                              \
         (io), (key), (target_ptr), JSON_READ_IO_READ_FN_FOR(target_ptr)))

// The key may be absent, in which case target_ptr is left alone. A key that
// is there still has to hold what was asked for.
#define JSON_READ_OPT(io, key, target_ptr)                                     \
    JSON_ReadIO_ReadKeyOptional(                                               \
        (io), (key), (target_ptr), JSON_READ_IO_READ_FN_FOR(target_ptr))

// Like JSON_READ(), except an array index instead of an object key.
#define JSON_READ_A(io, idx, target_ptr)                                       \
    JSON_ReadIO_ReadIndex(                                                     \
        (io), (idx), (target_ptr), JSON_READ_IO_READ_FN_FOR(target_ptr))

// Records the trouble and hands it back as a failure, naming the file and the
// place in it that the read had reached.
RESULT JSON_ReadIO_Fail(JSON_READ_IO *io, const char *fmt, ...);
