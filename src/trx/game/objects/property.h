#pragma once

#include <trx/core/utils.h>
#include <trx/core/value.h>

#include <stddef.h>
#include <stdint.h>

#define OBJECT_PROPERTIES(obj_, ...)                                           \
    do {                                                                       \
        const OBJECT_PROPERTY_DECL _d[] = { __VA_ARGS__ };                     \
        ObjectProperty_ApplyDeclarations((obj_), _d, ARRAY_SIZE(_d));          \
    } while (0)

typedef struct ITEM ITEM;
typedef struct JSON_READ_IO JSON_READ_IO;
typedef struct JSON_WRITE_IO JSON_WRITE_IO;
typedef struct OBJECT OBJECT;

typedef struct {
    const char *name, *description;
    TRX_VALUE value;
} OBJECT_PROPERTY_DECL;

typedef struct OBJECT_PROPERTY_ENTRY OBJECT_PROPERTY_ENTRY;
typedef struct {
    int32_t count;
    OBJECT_PROPERTY_ENTRY *entries;
} OBJECT_PROPERTY_SET;

typedef OBJECT_PROPERTY_SET ITEM_PROPERTY_SET;

#define OBJECT_PROPERTY_INT(name_, value_, description_)                       \
    {                                                                          \
        .name = name_, .description = description_, .value = {                 \
            .type = TVT_S32,                                                   \
            .as_int = value_                                                   \
        }                                                                      \
    }
#define OBJECT_PROPERTY_BOOL(name_, value_, description_)                      \
    {                                                                          \
        .name = name_, .description = description_, .value = {                 \
            .type = TVT_BOOL,                                                  \
            .as_bool = value_                                                  \
        }                                                                      \
    }
#define OBJECT_PROPERTY_XYZ(name_, value_, description_)                       \
    {                                                                          \
        .name = name_, .description = description_, .value = {                 \
            .type = TVT_XYZ_32,                                                \
            .as_xyz = value_                                                   \
        }                                                                      \
    }

#define OBJECT_PROPERTY_RGB(name_, r_, g_, b_, description_)                   \
    {                                                                          \
        .name = name_, .description = description_, .value = {                 \
            .type = TVT_RGB_888,                                               \
            .as_rgb = { .r = r_, .g = g_, .b = b_ }                            \
        }                                                                      \
    }

#define OBJECT_PROPERTY_DOUBLE(name_, value_, description_)                    \
    {                                                                          \
        .name = name_, .description = description_, .value = {                 \
            .type = TVT_DOUBLE,                                                \
            .as_num = value_                                                   \
        }                                                                      \
    }

void ObjectProperty_ResetObject(OBJECT *obj);
void ObjectProperty_ResetItem(ITEM *item);
void ObjectProperty_ApplyDeclarations(
    OBJECT *obj, const OBJECT_PROPERTY_DECL *declarations, size_t count);

bool ObjectProperty_GetObjectValue(
    const OBJECT *obj, const char *name, TRX_VALUE *out_value);
bool ObjectProperty_SetObjectValueRaw(
    OBJECT *obj, const char *name, TRX_VALUE value);

bool ObjectProperty_GetItemValue(
    const ITEM *item, const char *name, TRX_VALUE *out_value);
bool ObjectProperty_SetItemValueRaw(
    ITEM *item, const char *name, TRX_VALUE value);

int32_t ObjectProperty_GetObjectNameCount(const OBJECT *obj);
const char *ObjectProperty_GetObjectName(const OBJECT *obj, int32_t index);

int32_t ObjectProperty_GetItemNameCount(const ITEM *item);
const char *ObjectProperty_GetItemName(const ITEM *item, int32_t index);

void ObjectProperty_WriteItemOverrides(
    JSON_WRITE_IO *io, const ITEM *item, const char *key);
bool ObjectProperty_ReadItemOverrides(JSON_READ_IO *io, ITEM *item);
