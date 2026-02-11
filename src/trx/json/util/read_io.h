#pragma once

#include <trx/json.h>
#include <trx/log.h>

#include <stddef.h>
#include <stdint.h>

typedef struct JSON_READ_IO JSON_READ_IO;

typedef enum JSON_READ_TYPE {
    JSON_READ_TYPE_BOOL = 0,
    JSON_READ_TYPE_S8,
    JSON_READ_TYPE_U8,
    JSON_READ_TYPE_S16,
    JSON_READ_TYPE_U16,
    JSON_READ_TYPE_S32,
    JSON_READ_TYPE_U32,
    JSON_READ_TYPE_DOUBLE,
} JSON_READ_TYPE;

JSON_READ_IO *JSON_ReadIO_Create(JSON_VALUE *root, uint16_t version);
void JSON_ReadIO_Destroy(JSON_READ_IO *io, bool success);

const char *JSON_ReadIO_GetError(const JSON_READ_IO *io);
uint16_t JSON_ReadIO_GetVersion(const JSON_READ_IO *io);

void JSON_ReadIO_SetError(JSON_READ_IO *io, const char *fmt, ...);
bool JSON_ReadIO_PushObject(JSON_READ_IO *io, const char *key);
bool JSON_ReadIO_PushArrayElem(JSON_READ_IO *io, size_t index);
bool JSON_ReadIO_Pop(JSON_READ_IO *io);
int32_t JSON_ReadIO_GetArrayLength(JSON_READ_IO *io);
bool JSON_ReadIO_HasKey(JSON_READ_IO *io, const char *key);
JSON_OBJECT *JSON_ReadIO_GetCurrentObject(JSON_READ_IO *io);

bool JSON_ReadIO_Read(
    JSON_READ_IO *io, const char *key, JSON_READ_TYPE type, void *target);
bool JSON_ReadIO_ReadDirect(
    JSON_READ_IO *io, JSON_READ_TYPE type, void *target);

#define JSON_PUSH(io, key) JSON_ReadIO_PushObject((io), (key))
#define JSON_PUSH_INDEX(io, idx) JSON_ReadIO_PushArrayElem((io), (idx))
#define JSON_POP(io) JSON_ReadIO_Pop((io))
#define JSON_ARRAY_LEN(io) JSON_ReadIO_GetArrayLength((io))

#define JSON_READ_DIRECT(io, target_ptr)                                       \
    JSON_ReadIO_ReadDirect(                                                    \
        (io),                                                                  \
        _Generic(                                                              \
            *(target_ptr),                                                     \
            bool: JSON_READ_TYPE_BOOL,                                         \
            int8_t: JSON_READ_TYPE_S8,                                         \
            uint8_t: JSON_READ_TYPE_U8,                                        \
            int16_t: JSON_READ_TYPE_S16,                                       \
            uint16_t: JSON_READ_TYPE_U16,                                      \
            int32_t: JSON_READ_TYPE_S32,                                       \
            uint32_t: JSON_READ_TYPE_U32,                                      \
            double: JSON_READ_TYPE_DOUBLE),                                    \
        (void *)(target_ptr))

#define JSON_READ(io, key, target_ptr)                                         \
    JSON_ReadIO_Read(                                                          \
        (io), (key),                                                           \
        _Generic(                                                              \
            *(target_ptr),                                                     \
            bool: JSON_READ_TYPE_BOOL,                                         \
            int8_t: JSON_READ_TYPE_S8,                                         \
            uint8_t: JSON_READ_TYPE_U8,                                        \
            int16_t: JSON_READ_TYPE_S16,                                       \
            uint16_t: JSON_READ_TYPE_U16,                                      \
            int32_t: JSON_READ_TYPE_S32,                                       \
            uint32_t: JSON_READ_TYPE_U32,                                      \
            double: JSON_READ_TYPE_DOUBLE),                                    \
        (void *)(target_ptr))

#define JSON_SHOULD(x)                                                         \
    ((x) ? 1 : (LOG_WARNING("%s", JSON_ReadIO_GetError(io)), 0))
#define JSON_OPTIONAL(x) (x)
#define JSON_MUST(x)                                                           \
    if (!(x)) {                                                                \
        goto fail;                                                             \
    }
#define JSON_FAIL() goto fail;
#define JSON_FINISH()                                                          \
    do {                                                                       \
    success:                                                                   \
        return true;                                                           \
    fail:                                                                      \
        return false;                                                          \
    } while (0);
