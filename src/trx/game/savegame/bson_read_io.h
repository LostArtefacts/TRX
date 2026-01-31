#pragma once

#include <trx/json.h>
#include <trx/log.h>

#include <stddef.h>
#include <stdint.h>

typedef struct SG_READ_IO SG_READ_IO;

typedef enum SG_READ_TYPE {
    SG_TYPE_BOOL = 0,
    SG_TYPE_S8,
    SG_TYPE_U8,
    SG_TYPE_S16,
    SG_TYPE_U16,
    SG_TYPE_S32,
    SG_TYPE_U32,
    SG_TYPE_DOUBLE,
} SG_READ_TYPE;

SG_READ_IO *SG_ReadIO_Create(JSON_VALUE *root, uint16_t sg_version);
void SG_ReadIO_Destroy(SG_READ_IO *io, bool success);

const char *SG_ReadIO_GetError(const SG_READ_IO *io);
uint16_t SG_ReadIO_GetVersion(const SG_READ_IO *io);

void SG_ReadIO_SetError(SG_READ_IO *io, const char *fmt, ...);
bool SG_ReadIO_PushObject(SG_READ_IO *io, const char *key);
bool SG_ReadIO_PushArrayElem(SG_READ_IO *io, size_t index);
bool SG_ReadIO_Pop(SG_READ_IO *io);
int32_t SG_ReadIO_GetArrayLength(SG_READ_IO *io);
bool SG_ReadIO_HasKey(SG_READ_IO *io, const char *key);
JSON_OBJECT *SG_ReadIO_GetCurrentObject(SG_READ_IO *io);

bool SG_ReadIO_ReadValue(
    SG_READ_IO *io, const char *key, SG_READ_TYPE type, void *target);
bool SG_ReadIO_ReadValueDirect(SG_READ_IO *io, SG_READ_TYPE type, void *target);

#define SG_PUSH(io, key) SG_ReadIO_PushObject((io), (key))
#define SG_PUSH_INDEX(io, idx) SG_ReadIO_PushArrayElem((io), (idx))
#define SG_POP(io) SG_ReadIO_Pop((io))
#define SG_ARRAY_LEN(io) SG_ReadIO_GetArrayLength((io))

#define SG_READ_VALUE_DIRECT(io, target_ptr)                                   \
    SG_ReadIO_ReadValueDirect(                                                 \
        (io),                                                                  \
        _Generic(                                                              \
            *(target_ptr),                                                     \
            bool: SG_TYPE_BOOL,                                                \
            int8_t: SG_TYPE_S8,                                                \
            uint8_t: SG_TYPE_U8,                                               \
            int16_t: SG_TYPE_S16,                                              \
            uint16_t: SG_TYPE_U16,                                             \
            int32_t: SG_TYPE_S32,                                              \
            uint32_t: SG_TYPE_U32,                                             \
            double: SG_TYPE_DOUBLE),                                           \
        (void *)(target_ptr))

#define SG_READ_VALUE(io, key, target_ptr)                                     \
    SG_ReadIO_ReadValue(                                                       \
        (io), (key),                                                           \
        _Generic(                                                              \
            *(target_ptr),                                                     \
            bool: SG_TYPE_BOOL,                                                \
            int8_t: SG_TYPE_S8,                                                \
            uint8_t: SG_TYPE_U8,                                               \
            int16_t: SG_TYPE_S16,                                              \
            uint16_t: SG_TYPE_U16,                                             \
            int32_t: SG_TYPE_S32,                                              \
            uint32_t: SG_TYPE_U32,                                             \
            double: SG_TYPE_DOUBLE),                                           \
        (void *)(target_ptr))

#define SG_SHOULD(x) ((x) ? 1 : (LOG_WARNING("%s", SG_ReadIO_GetError(io)), 0))
#define SG_OPTIONAL(x) (x)
#define SG_MUST(x)                                                             \
    if (!(x)) {                                                                \
        goto fail;                                                             \
    }
#define SG_FAIL() goto fail;
#define SG_FINISH()                                                            \
    do {                                                                       \
    success:                                                                   \
        return true;                                                           \
    fail:                                                                      \
        return false;                                                          \
    } while (0);
