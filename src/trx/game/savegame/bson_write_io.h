#pragma once

#include <trx/json.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct SG_WRITE_IO SG_WRITE_IO;

typedef enum SG_WRITE_TYPE {
    SG_WRITE_BOOL = 0,
    SG_WRITE_INT,
    SG_WRITE_DOUBLE,
    SG_WRITE_STRING,
} SG_WRITE_TYPE;

SG_WRITE_IO *SG_WriteIO_Create(void);
void SG_WriteIO_Destroy(SG_WRITE_IO *io);

JSON_VALUE *SG_WriteIO_GetRoot(SG_WRITE_IO *io);

void SG_WriteIO_PushObject(SG_WRITE_IO *io);
void SG_WriteIO_PushArray(SG_WRITE_IO *io);
void SG_WriteIO_PopAndSet(SG_WRITE_IO *io, const char *key);
void SG_WriteIO_PopAndAppend(SG_WRITE_IO *io);
JSON_OBJECT *SG_WriteIO_GetCurrentObject(SG_WRITE_IO *io);

void SG_WriteIO_PushBool(SG_WRITE_IO *io, bool value);
void SG_WriteIO_PushInt(SG_WRITE_IO *io, int32_t value);
void SG_WriteIO_PushDouble(SG_WRITE_IO *io, double value);
void SG_WriteIO_PushString(SG_WRITE_IO *io, const char *value);

#define SGW_PUSH_OBJECT(io) SG_WriteIO_PushObject((io))
#define SGW_PUSH_ARRAY(io) SG_WriteIO_PushArray((io))
#define SGW_POP_AND_SET(io, key) SG_WriteIO_PopAndSet((io), (key))
#define SGW_POP_AND_APPEND(io) SG_WriteIO_PopAndAppend((io))

#define SGW_PUSH_VALUE(io, value)                                              \
    _Generic(                                                                  \
        (value),                                                               \
        bool: SG_WriteIO_PushBool,                                             \
        int8_t: SG_WriteIO_PushInt,                                            \
        uint8_t: SG_WriteIO_PushInt,                                           \
        int16_t: SG_WriteIO_PushInt,                                           \
        uint16_t: SG_WriteIO_PushInt,                                          \
        int32_t: SG_WriteIO_PushInt,                                           \
        uint32_t: SG_WriteIO_PushInt,                                          \
        float: SG_WriteIO_PushDouble,                                          \
        double: SG_WriteIO_PushDouble,                                         \
        const char *: SG_WriteIO_PushString,                                   \
        char *: SG_WriteIO_PushString)(io, value)

#define SGW_WRITE_VALUE(io, key, value)                                        \
    do {                                                                       \
        SGW_PUSH_VALUE((io), (value));                                         \
        SGW_POP_AND_SET((io), (key));                                          \
    } while (0)

#define SGW_WRITE_VALUE_NZ(io, key, value)                                     \
    do {                                                                       \
        typeof(value) _tmp = (value);                                          \
        unsigned char _zero[sizeof(_tmp)] = { 0 };                             \
        if (memcmp(&_tmp, _zero, sizeof(_tmp)) != 0) {                         \
            SGW_WRITE_VALUE(io, key, _tmp);                                    \
        }                                                                      \
    } while (0)
