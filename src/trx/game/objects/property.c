#include <trx/game/objects/property.h>

#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/value.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/memory.h>
#include <trx/debug.h>
#include <trx/game/items.h>
#include <trx/game/objects.h>

#include <string.h>

struct OBJECT_PROPERTY_ENTRY {
    const char *name;
    const char *description;
    TRX_VALUE value;
};

// Object properties are numeric: the declaration macros produce only S32, BOOL,
// XYZ_32, RGB_888 and DOUBLE, none of which own memory. A text-valued property
// would need string ownership the set does not carry, so it is refused at the
// door.
static void M_AssertNumeric(const TRX_VALUE_TYPE type)
{
    ASSERT(type != TVT_STRING && type != TVT_DYNAMIC_ENUM);
}

static void M_FreeSet(OBJECT_PROPERTY_SET *const set)
{
    Memory_FreePointer(&set->entries);
    set->count = 0;
}

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
    const char *const description, const TRX_VALUE *const value)
{
    M_AssertNumeric(value->type);
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
    OBJECT *const obj, const char *const name, const TRX_VALUE *const value)
{
    M_AssertNumeric(value->type);
    OBJECT_PROPERTY_ENTRY *const entry = M_GetEntry(&obj->properties, name);
    if (entry == nullptr) {
        return;
    }
    entry->value = *value;
}

static void M_ApplyItemValue(
    ITEM *const item, const char *const name, const TRX_VALUE *const value)
{
    const OBJECT_PROPERTY_ENTRY *const object_entry =
        M_GetObjectEntry(Object_TryGet(item->object_id), name);
    if (object_entry == nullptr) {
        return;
    }
    M_AddEntry(&item->properties, name, object_entry->description, value);
}

void ObjectProperty_ResetObject(OBJECT *const obj)
{
    M_FreeSet(&obj->properties);
}

void ObjectProperty_ResetItem(ITEM *const item)
{
    M_FreeSet(&item->properties);
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
    const OBJECT *const obj, const char *const name, TRX_VALUE *const out_value)
{
    const OBJECT_PROPERTY_ENTRY *const entry = M_GetObjectEntry(obj, name);
    if (entry == nullptr) {
        return false;
    }
    *out_value = entry->value;
    return true;
}

bool ObjectProperty_SetObjectValueRaw(
    OBJECT *const obj, const char *const name, const TRX_VALUE value)
{
    OBJECT_PROPERTY_ENTRY *const entry =
        obj == nullptr ? nullptr : M_GetEntry(&obj->properties, name);
    if (entry == nullptr) {
        return false;
    }
    TRX_VALUE coerced_value = {};
    if (!Value_Coerce(entry->value.type, &value, &coerced_value)) {
        return false;
    }
    M_ApplyObjectValue(obj, entry->name, &coerced_value);
    return true;
}

bool ObjectProperty_GetItemValue(
    const ITEM *const item, const char *const name, TRX_VALUE *const out_value)
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
    ITEM *const item, const char *const name, const TRX_VALUE value)
{
    if (item == nullptr) {
        return false;
    }

    const OBJECT_PROPERTY_ENTRY *const object_entry =
        M_GetObjectEntry(Object_TryGet(item->object_id), name);
    if (object_entry == nullptr) {
        return false;
    }

    TRX_VALUE coerced_value = {};
    if (!Value_Coerce(object_entry->value.type, &value, &coerced_value)) {
        return false;
    }
    M_ApplyItemValue(item, object_entry->name, &coerced_value);
    return true;
}

void ObjectProperty_WriteItemOverrides(
    JSON_WRITE_IO *const io, const ITEM *const item, const char *const key)
{
    if (item == nullptr || item->properties.count == 0) {
        return;
    }

    JSONW_PUSH_OBJECT(io);
    JSON_OBJECT *const props = JSON_WriteIO_GetCurrentObject(io);
    for (int32_t i = 0; i < item->properties.count; i++) {
        const OBJECT_PROPERTY_ENTRY *const entry = &item->properties.entries[i];
        // Object properties are numeric, so no enum-map name is in play;
        // nullptr is the right param.
        JSONValue_Write(
            props, entry->name, entry->value.type, nullptr, &entry->value);
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

    const JSON_OBJECT *const props = JSON_ReadIO_GetCurrentObject(io);
    for (int32_t i = 0; i < obj->properties.count; i++) {
        const OBJECT_PROPERTY_ENTRY *const entry = &obj->properties.entries[i];
        if (!JSON_ReadIO_HasKey(io, entry->name)) {
            continue;
        }

        TRX_VALUE value;
        if (!JSONValue_Read(
                props, entry->name, entry->value.type, nullptr, &value)) {
            goto fail;
        }
        M_ApplyItemValue(item, entry->name, &value);
    }

    return JSON_POP(io);

fail:
    JSON_POP(io);
    return false;
}
