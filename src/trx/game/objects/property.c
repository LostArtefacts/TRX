#include <trx/game/objects/property.h>

#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/memory.h>
#include <trx/game/items.h>
#include <trx/game/objects.h>

#include <string.h>

struct OBJECT_PROPERTY_ENTRY {
    const char *name;
    const char *description;
    OBJECT_PROPERTY_VALUE value;
};

__attribute__((destructor)) static void M_Shutdown(void)
{
    for (int32_t i = O_FIRST; i < O_NUMBER_OF; i++) {
        ObjectProperty_ResetObject(Object_Get(i));
    }
}

static OBJECT_PROPERTY_ENTRY *M_GetEntry(
    const OBJECT_PROPERTY_SET *const properties, const char *const name)
{
    if (properties == nullptr || name == nullptr) {
        return nullptr;
    }

    for (int32_t i = 0; i < properties->count; i++) {
        OBJECT_PROPERTY_ENTRY *const entry = &properties->entries[i];
        if (strcmp(entry->name, name) == 0) {
            return entry;
        }
    }
    return nullptr;
}

static const OBJECT_PROPERTY_ENTRY *M_GetObjectEntry(
    const OBJECT *const obj, const char *const name)
{
    if (obj == nullptr) {
        return nullptr;
    }
    return M_GetEntry(&obj->properties, name);
}

static OBJECT_PROPERTY_ENTRY *M_AddEntry(
    OBJECT_PROPERTY_SET *const properties, const char *const name,
    const char *const description, const OBJECT_PROPERTY_VALUE *const value)
{
    OBJECT_PROPERTY_ENTRY *entry = M_GetEntry(properties, name);
    if (entry != nullptr) {
        entry->description = description;
        entry->value = *value;
        return entry;
    }

    properties->entries = Memory_Realloc(
        properties->entries,
        (properties->count + 1) * sizeof(OBJECT_PROPERTY_ENTRY));
    entry = &properties->entries[properties->count];
    properties->count++;
    *entry = (OBJECT_PROPERTY_ENTRY) {
        .name = name,
        .description = description,
        .value = *value,
    };
    return entry;
}

static void M_ApplyObjectValue(
    OBJECT *const obj, const char *const name,
    const OBJECT_PROPERTY_VALUE *const value)
{
    OBJECT_PROPERTY_ENTRY *const entry = M_GetEntry(&obj->properties, name);
    if (entry == nullptr) {
        return;
    }
    entry->value = *value;
}

static void M_ApplyItemValue(
    ITEM *const item, const char *const name,
    const OBJECT_PROPERTY_VALUE *const value)
{
    const OBJECT_PROPERTY_ENTRY *const object_entry =
        M_GetObjectEntry(Object_TryGet(item->object_id), name);
    if (object_entry == nullptr) {
        return;
    }
    M_AddEntry(&item->properties, name, object_entry->description, value);
}

static bool M_CoerceValue(
    const OBJECT_PROPERTY_VALUE target, const OBJECT_PROPERTY_VALUE input,
    OBJECT_PROPERTY_VALUE *const out_value)
{
    *out_value = input;
    if (target.type == input.type) {
        return true;
    }

    switch (target.type) {
    case OBJECT_PROPERTY_TYPE_INT:
        switch (input.type) {
        case OBJECT_PROPERTY_TYPE_FLOAT:
            out_value->type = OBJECT_PROPERTY_TYPE_INT;
            out_value->as_int = input.as_float;
            return true;
        case OBJECT_PROPERTY_TYPE_DOUBLE:
            out_value->type = OBJECT_PROPERTY_TYPE_INT;
            out_value->as_int = input.as_double;
            return true;
        default:
            return false;
        }
    case OBJECT_PROPERTY_TYPE_FLOAT:
        switch (input.type) {
        case OBJECT_PROPERTY_TYPE_INT:
            out_value->type = OBJECT_PROPERTY_TYPE_FLOAT;
            out_value->as_float = input.as_int;
            return true;
        case OBJECT_PROPERTY_TYPE_DOUBLE:
            out_value->type = OBJECT_PROPERTY_TYPE_FLOAT;
            out_value->as_float = input.as_double;
            return true;
        default:
            return false;
        }
    case OBJECT_PROPERTY_TYPE_DOUBLE:
        switch (input.type) {
        case OBJECT_PROPERTY_TYPE_INT:
            out_value->type = OBJECT_PROPERTY_TYPE_DOUBLE;
            out_value->as_double = input.as_int;
            return true;
        case OBJECT_PROPERTY_TYPE_FLOAT:
            out_value->type = OBJECT_PROPERTY_TYPE_DOUBLE;
            out_value->as_double = input.as_float;
            return true;
        default:
            return false;
        }
    case OBJECT_PROPERTY_TYPE_BOOL:
    case OBJECT_PROPERTY_TYPE_XYZ:
        return false;
    }

    return false;
}

void ObjectProperty_ResetObject(OBJECT *const obj)
{
    Memory_FreePointer(&obj->properties.entries);
    obj->properties.count = 0;
}

void ObjectProperty_ResetItem(ITEM *const item)
{
    Memory_FreePointer(&item->properties.entries);
    item->properties.count = 0;
}

void ObjectProperty_ApplyDeclarations(
    OBJECT *const obj, const OBJECT_PROPERTY_DECL *const declarations,
    const size_t count)
{
    for (size_t i = 0; i < count; i++) {
        const OBJECT_PROPERTY_DECL *const declaration = &declarations[i];
        M_AddEntry(
            &obj->properties, declaration->name, declaration->description,
            &declaration->value);
        M_ApplyObjectValue(obj, declaration->name, &declaration->value);
    }
}

int32_t ObjectProperty_GetObjectNameCount(const OBJECT *const obj)
{
    return obj == nullptr ? 0 : obj->properties.count;
}

const char *ObjectProperty_GetObjectName(
    const OBJECT *const obj, const int32_t index)
{
    if (obj == nullptr || index < 0 || index >= obj->properties.count) {
        return nullptr;
    }
    return obj->properties.entries[index].name;
}

int32_t ObjectProperty_GetItemNameCount(const ITEM *const item)
{
    const OBJECT *const obj =
        item == nullptr ? nullptr : Object_TryGet(item->object_id);
    return ObjectProperty_GetObjectNameCount(obj);
}

const char *ObjectProperty_GetItemName(
    const ITEM *const item, const int32_t index)
{
    const OBJECT *const obj =
        item == nullptr ? nullptr : Object_TryGet(item->object_id);
    return ObjectProperty_GetObjectName(obj, index);
}

bool ObjectProperty_GetObjectValue(
    const OBJECT *const obj, const char *const name,
    OBJECT_PROPERTY_VALUE *const out_value)
{
    const OBJECT_PROPERTY_ENTRY *const entry = M_GetObjectEntry(obj, name);
    if (entry == nullptr) {
        return false;
    }
    *out_value = entry->value;
    return true;
}

bool ObjectProperty_SetObjectValueRaw(
    OBJECT *const obj, const char *const name,
    const OBJECT_PROPERTY_VALUE value)
{
    OBJECT_PROPERTY_ENTRY *const entry =
        obj == nullptr ? nullptr : M_GetEntry(&obj->properties, name);
    if (entry == nullptr) {
        return false;
    }
    OBJECT_PROPERTY_VALUE coerced_value = {};
    if (!M_CoerceValue(entry->value, value, &coerced_value)) {
        return false;
    }
    M_ApplyObjectValue(obj, name, &coerced_value);
    return true;
}

bool ObjectProperty_GetItemValue(
    const ITEM *const item, const char *const name,
    OBJECT_PROPERTY_VALUE *const out_value)
{
    if (item == nullptr) {
        return false;
    }

    const OBJECT_PROPERTY_ENTRY *entry = M_GetEntry(&item->properties, name);
    if (entry != nullptr) {
        *out_value = entry->value;
        return true;
    }
    return ObjectProperty_GetObjectValue(
        Object_TryGet(item->object_id), name, out_value);
}

bool ObjectProperty_SetItemValueRaw(
    ITEM *const item, const char *const name, const OBJECT_PROPERTY_VALUE value)
{
    if (item == nullptr) {
        return false;
    }

    const OBJECT_PROPERTY_ENTRY *const object_entry =
        M_GetObjectEntry(Object_TryGet(item->object_id), name);
    if (object_entry == nullptr) {
        return false;
    }

    OBJECT_PROPERTY_VALUE coerced_value = {};
    if (!M_CoerceValue(object_entry->value, value, &coerced_value)) {
        return false;
    }
    M_ApplyItemValue(item, name, &coerced_value);
    return true;
}

void ObjectProperty_WriteItemOverrides(
    JSON_WRITE_IO *const io, const ITEM *const item, const char *const key)
{
    if (item == nullptr || item->properties.count == 0) {
        return;
    }

    JSONW_PUSH_OBJECT(io);
    for (int32_t i = 0; i < item->properties.count; i++) {
        const OBJECT_PROPERTY_ENTRY *const entry = &item->properties.entries[i];
        const OBJECT_PROPERTY_VALUE *const value = &entry->value;
        switch (value->type) {
        case OBJECT_PROPERTY_TYPE_INT:
            JSONW_WRITE(io, entry->name, value->as_int);
            break;
        case OBJECT_PROPERTY_TYPE_FLOAT:
            JSONW_WRITE(io, entry->name, value->as_float);
            break;
        case OBJECT_PROPERTY_TYPE_DOUBLE:
            JSONW_WRITE(io, entry->name, value->as_double);
            break;
        case OBJECT_PROPERTY_TYPE_BOOL:
            JSONW_WRITE(io, entry->name, value->as_bool);
            break;
        case OBJECT_PROPERTY_TYPE_XYZ:
            JSONW_PUSH_OBJECT(io);
            JSONW_WRITE(io, "x", value->as_xyz.x);
            JSONW_WRITE(io, "y", value->as_xyz.y);
            JSONW_WRITE(io, "z", value->as_xyz.z);
            JSONW_POP_AND_SET(io, entry->name);
            break;
        }
    }
    JSONW_POP_AND_SET(io, key);
}

bool ObjectProperty_ReadItemOverrides(JSON_READ_IO *const io, ITEM *const item)
{
    if (!JSON_PUSH(io, "properties")) {
        return true;
    }

    const OBJECT *const obj =
        item == nullptr ? nullptr : Object_TryGet(item->object_id);
    if (obj == nullptr) {
        goto fail;
    }

    for (int32_t i = 0; i < obj->properties.count; i++) {
        const OBJECT_PROPERTY_ENTRY *const entry = &obj->properties.entries[i];
        if (!JSON_ReadIO_HasKey(io, entry->name)) {
            continue;
        }
        OBJECT_PROPERTY_VALUE value = {
            .type = entry->value.type,
        };
        switch (value.type) {
        case OBJECT_PROPERTY_TYPE_INT:
            if (!JSON_READ(io, entry->name, &value.as_int)) {
                goto fail;
            }
            break;
        case OBJECT_PROPERTY_TYPE_FLOAT:
            if (!JSON_READ(io, entry->name, &value.as_float)) {
                goto fail;
            }
            break;
        case OBJECT_PROPERTY_TYPE_DOUBLE:
            if (!JSON_READ(io, entry->name, &value.as_double)) {
                goto fail;
            }
            break;
        case OBJECT_PROPERTY_TYPE_BOOL:
            if (!JSON_READ(io, entry->name, &value.as_bool)) {
                goto fail;
            }
            break;
        case OBJECT_PROPERTY_TYPE_XYZ:
            if (!JSON_PUSH(io, entry->name)) {
                goto fail;
            }
            if (!JSON_READ(io, "x", &value.as_xyz.x)) {
                goto fail;
            }
            if (!JSON_READ(io, "y", &value.as_xyz.y)) {
                goto fail;
            }
            if (!JSON_READ(io, "z", &value.as_xyz.z)) {
                goto fail;
            }
            JSON_POP(io);
            break;
        }
        M_ApplyItemValue(item, entry->name, &value);
    }

    return JSON_POP(io);

fail:
    JSON_POP(io);
    return false;
}
