#include <trx/core/file.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/vector.h>
#include <trx/game/inject.h>
#include <trx/game/objects.h>

typedef enum {
    M_TARGET_OBJECT,
    M_TARGET_ITEM,
} M_TARGET_TYPE;

typedef struct {
    const char *name;
    TRX_VALUE value;
} M_PROPERTY;

typedef struct {
    M_TARGET_TYPE target_type;
    int32_t target_id;
    VECTOR *properties;
} M_PROPERTY_BATCH;

// On-disk property type tags, fixed by the injection file format.
// TRX_VALUE_TYPE orders its own constants independently, so the tag read from
// the file is mapped to a TRX_VALUE_TYPE explicitly rather than cast.
typedef enum {
    M_DISK_INT = 0,
    M_DISK_FLOAT = 1,
    M_DISK_DOUBLE = 2,
    M_DISK_BOOL = 3,
    M_DISK_XYZ = 4,
    M_DISK_RGB = 5,
} M_DISK_PROPERTY_TYPE;

static VECTOR *m_Batches = nullptr;

static const char *M_ReadName(const INJECTION *const injection)
{
    const int32_t length = File_ReadS32(injection->fp);
    if (length <= 0) {
        return nullptr;
    }

    char *const name = Memory_Alloc((size_t)(length + 1));
    File_ReadData(injection->fp, name, length);
    name[length] = '\0';

    return name;
}

static TRX_VALUE M_ReadValue(const INJECTION *const injection)
{
    TRX_VALUE value = {};
    const int32_t disk_type = File_ReadS32(injection->fp);

    switch (disk_type) {
    case M_DISK_INT:
        value.type = TVT_S32;
        value.as_int = File_ReadS32(injection->fp);
        break;
    case M_DISK_FLOAT:
        value.type = TVT_FLOAT;
        value.as_num = File_ReadFloat(injection->fp);
        break;
    case M_DISK_DOUBLE:
        value.type = TVT_DOUBLE;
        value.as_num = File_ReadDouble(injection->fp);
        break;
    case M_DISK_BOOL:
        value.type = TVT_BOOL;
        value.as_bool = File_ReadS32(injection->fp) != 0;
        break;
    case M_DISK_XYZ:
        value.type = TVT_XYZ_32;
        value.as_xyz = (XYZ_32) {
            .x = File_ReadS32(injection->fp),
            .y = File_ReadS32(injection->fp),
            .z = File_ReadS32(injection->fp),
        };
        break;
    case M_DISK_RGB:
        value.type = TVT_RGB_888;
        value.as_rgb = (RGB_888) {
            .r = File_ReadU8(injection->fp),
            .g = File_ReadU8(injection->fp),
            .b = File_ReadU8(injection->fp),
        };
        break;
    default:
        LOG_WARNING("Unknown property type %d", disk_type);
        break;
    }

    return value;
}

static void *M_GetBatchTarget(const M_PROPERTY_BATCH *const batch)
{
    void *target = nullptr;
    switch (batch->target_type) {
    case M_TARGET_OBJECT:
        OBJECT *const obj = Object_TryGet(batch->target_id);
        if (obj != nullptr && obj->loaded) {
            target = (void *)obj;
        }
        break;

    case M_TARGET_ITEM:
        target = (void *)Item_Get(batch->target_id);
        break;

    default:
        break;
    }

    return target;
}

static void M_ApplyBatch(const M_PROPERTY_BATCH *const batch)
{
    void *target = M_GetBatchTarget(batch);
    if (target == nullptr) {
        LOG_WARNING(
            "Invalid property target type %d, id %d", batch->target_type,
            batch->target_id);
        return;
    }

    for (int32_t i = 0; i < batch->properties->count; i++) {
        const M_PROPERTY *const property = Vector_Get(batch->properties, i);

        const char *err;
        switch (batch->target_type) {
        case M_TARGET_OBJECT:
            err = ObjectProperty_SetObjectValueRaw(
                (OBJECT *)target, property->name, property->value);
            break;

        case M_TARGET_ITEM:
            err = ObjectProperty_SetItemValueRaw(
                (ITEM *)target, property->name, property->value);
            break;

        default:
            err = "no such target type";
            break;
        }

        if (err != nullptr) {
            LOG_WARNING(
                "Failed to set property %s on target type %d, id %d: %s",
                property->name, batch->target_type, batch->target_id, err);
        }
    }
}

static void M_PropertyEdits(
    const INJECTION_CONTEXT *const ctx, const INJECTION *const injection,
    const int32_t data_count)
{
    if (data_count <= 0) {
        return;
    }

    if (m_Batches == nullptr) {
        m_Batches =
            Vector_CreateAtCapacity(sizeof(M_PROPERTY_BATCH), data_count);
    }

    for (int32_t i = 0; i < data_count; i++) {
        M_PROPERTY_BATCH batch = {};
        batch.target_type = File_ReadS32(injection->fp);

        switch (batch.target_type) {
        case M_TARGET_OBJECT:
            const INJECTION_OBJECT_INFO obj_info =
                Inject_ReadObjectPtr(injection);
            batch.target_id = obj_info.id;
            break;

        case M_TARGET_ITEM:
            batch.target_id = File_ReadS32(injection->fp);
            break;

        default:
            LOG_WARNING("Unknown property target type %d", batch.target_type);
            break;
        }

        const int32_t property_count = File_ReadS32(injection->fp);
        if (property_count > 0) {
            batch.properties =
                Vector_CreateAtCapacity(sizeof(M_PROPERTY), property_count);
            for (int32_t j = 0; j < property_count; j++) {
                const M_PROPERTY property = {
                    .name = M_ReadName(injection),
                    .value = M_ReadValue(injection),
                };
                Vector_Add(batch.properties, &property);
            }
        }

        Vector_Add(m_Batches, &batch);
    }
}

static void M_FreeBatch(M_PROPERTY_BATCH *const batch)
{
    if (batch->properties == nullptr) {
        return;
    }

    for (int32_t i = 0; i < batch->properties->count; i++) {
        M_PROPERTY *const property = Vector_Get(batch->properties, i);
        Memory_FreePointer(&property->name);
    }

    Vector_Free(batch->properties);
    batch->properties = nullptr;
}

static void M_Cleanup(void)
{
    if (m_Batches == nullptr) {
        return;
    }

    for (int32_t i = 0; i < m_Batches->count; i++) {
        M_FreeBatch(Vector_Get(m_Batches, i));
    }

    Vector_Free(m_Batches);
    m_Batches = nullptr;
}

void Inject_ApplyProperties(void)
{
    if (m_Batches == nullptr) {
        return;
    }

    for (int32_t i = 0; i < m_Batches->count; i++) {
        M_ApplyBatch(Vector_Get(m_Batches, i));
    }

    M_Cleanup();
}

REGISTER_INJECT_EDITOR(IDT_PROPERTY_EDITS, M_PropertyEdits)
