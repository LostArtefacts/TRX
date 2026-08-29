#include <trx/game/objects/property.h>

#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/value.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/memory.h>
#include <trx/core/subsystem.h>
#include <trx/debug.h>
#include <trx/game/items.h>
#include <trx/game/objects.h>

#include <string.h>

struct OBJECT_PROPERTY_ENTRY {
    const char *name;
    const char *description;
    TRX_VALUE value;
    // How an item's live copy is reached; see OBJECT_PROPERTY_DECL.
    size_t field_offset;
    TRX_VALUE_TYPE field_type;
    size_t field_size;
    bool on_item;
    const char *(*check)(const TRX_VALUE *value);
    void (*set)(ITEM *item, const TRX_VALUE *value);
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

static void M_Shutdown(void)
{
    for (int32_t i = O_FIRST; i < Object_GetCount(); i++) {
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

// Whether the property will take the value at all, asked without an item to
// take it to: the member's width first, as in Field_Set, since the fit is a
// property of the storage rather than of what the value means, then what
// the property itself says. A property with no member has no width to be held
// to.
//
// Nothing here needs an item, which is what lets a value stated before the
// level has items - a level's own property batch, a script running at setup -
// be answered where it is stated rather than at each item it later fails to
// reach.
static const char *M_CheckValue(
    const OBJECT_PROPERTY_ENTRY *const decl, const TRX_VALUE *const value)
{
    if (decl->field_size != 0) {
        const char *const err = Value_CheckRange(decl->field_type, value);
        if (err != nullptr) {
            return err;
        }
    }
    return decl->check != nullptr ? decl->check(value) : nullptr;
}

// Writes an item's live copy of a property: the member the object declared, or
// the setter standing in for one. A property with neither keeps no copy. The
// value has passed M_CheckValue, so there is nothing left to refuse.
static void M_WriteLiveValue(
    ITEM *const item, const OBJECT_PROPERTY_ENTRY *const decl,
    const TRX_VALUE *const value)
{
    if (item == nullptr || (decl->set == nullptr && decl->field_size == 0)) {
        return;
    }
    // A member of the object's priv, and a setter standing in for one, both
    // live in what the level has not allocated yet; that copy is written as the
    // item is initialised. A member of the item itself is there from the start,
    // and a setter with no member of its own keeps its value elsewhere, so both
    // run either way.
    if (decl->field_size != 0 && !decl->on_item && item->priv == nullptr) {
        return;
    }

    if (decl->set != nullptr) {
        decl->set(item, value);
    } else {
        void *const base = decl->on_item ? (void *)item : item->priv;
        Value_WritePtr(
            decl->field_type, (char *)base + decl->field_offset, value);
    }
}

// Every item of an object holds its own copy of what the object declared, so a
// change to the object's value reaches the items that have not overridden it.
static void M_WriteLiveValueToItems(
    const OBJECT *const obj, const OBJECT_PROPERTY_ENTRY *const decl)
{
    if (decl->set == nullptr && decl->field_size == 0) {
        return;
    }
    for (int32_t i = 0; i < Item_GetTotalCount(); i++) {
        ITEM *const item = Item_Get(i);
        if (Object_TryGet(item->object_id) != obj
            || M_GetEntry(&item->properties, decl->name) != nullptr) {
            continue;
        }
        M_WriteLiveValue(item, decl, &decl->value);
    }
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

// The store is what a reader is answered from, so it may only hold what the
// items will take: the value is put to the property before it is recorded, and
// a refusal leaves the one before it in place.
static const char *M_ApplyObjectValue(
    OBJECT *const obj, const char *const name, const TRX_VALUE *const value)
{
    M_AssertNumeric(value->type);
    OBJECT_PROPERTY_ENTRY *const entry = M_GetEntry(&obj->properties, name);
    if (entry == nullptr) {
        return "no such property";
    }
    const char *const err = M_CheckValue(entry, value);
    if (err != nullptr) {
        return err;
    }
    entry->value = *value;
    M_WriteLiveValueToItems(obj, entry);
    return nullptr;
}

// As in M_ApplyObjectValue: the override is recorded only once the property has
// accepted it, whether or not there is yet an item to write it to.
static const char *M_ApplyItemValue(
    ITEM *const item, const char *const name, const TRX_VALUE *const value)
{
    const OBJECT_PROPERTY_ENTRY *const object_entry =
        M_GetObjectEntry(Object_TryGet(item->object_id), name);
    if (object_entry == nullptr) {
        return "no such property";
    }
    const char *const err = M_CheckValue(object_entry, value);
    if (err != nullptr) {
        return err;
    }
    M_WriteLiveValue(item, object_entry, value);
    M_AddEntry(&item->properties, name, object_entry->description, value);
    return nullptr;
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
        OBJECT_PROPERTY_ENTRY *const entry = M_AddEntry(
            &obj->properties, declaration->name, declaration->description,
            &declaration->value);
        entry->field_offset = declaration->field_offset;
        entry->field_type = declaration->field_type;
        entry->field_size = declaration->field_size;
        entry->on_item = declaration->on_item;
        entry->check = declaration->check;
        entry->set = declaration->set;
        // A bound property is the view of its member: it carries the member's
        // type, so what the member cannot hold is turned away at the write
        // rather than dropped silently on the way to it. A default of another
        // numeric type - an integer written for a double member - is converted
        // once, here.
        const TRX_VALUE_TYPE value_type = entry->value.type;
        TRX_VALUE value = entry->value;
        if (entry->field_size != 0 && entry->field_type != value_type
            && !Value_Coerce(entry->field_type, &entry->value, &value)) {
            // A bool member stated as 0, an XYZ_32 one stated as a number:
            // there is no reading of the default that the member would take.
            ASSERT_FAIL_FMT(
                "%s: default is %s, member holds %s", entry->name,
                Value_TypeName(value_type), Value_TypeName(entry->field_type));
        }
        const char *const err =
            M_ApplyObjectValue(obj, declaration->name, &value);
        if (err != nullptr) {
            ASSERT_FAIL_FMT("%s: %s", declaration->name, err);
        }
    }
}

void ObjectProperty_ApplyToItem(ITEM *const item)
{
    const OBJECT *const obj =
        item == nullptr ? nullptr : Object_TryGet(item->object_id);
    if (obj == nullptr) {
        return;
    }

    // An override recorded before the item had a priv to write it to is only
    // now written; it was held to the property when it was stated, so there is
    // nothing left to refuse here.
    for (int32_t i = 0; i < obj->properties.count; i++) {
        const OBJECT_PROPERTY_ENTRY *const decl = &obj->properties.entries[i];
        const OBJECT_PROPERTY_ENTRY *const own =
            M_GetEntry(&item->properties, decl->name);
        M_WriteLiveValue(
            item, decl, own != nullptr ? &own->value : &decl->value);
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

const char *ObjectProperty_SetObjectValueRaw(
    OBJECT *const obj, const char *const name, const TRX_VALUE value)
{
    OBJECT_PROPERTY_ENTRY *const entry =
        obj == nullptr ? nullptr : M_GetEntry(&obj->properties, name);
    if (entry == nullptr) {
        return "no such property";
    }
    TRX_VALUE coerced_value = {};
    if (!Value_Coerce(entry->value.type, &value, &coerced_value)) {
        return "value is not of the property's type";
    }
    return M_ApplyObjectValue(obj, entry->name, &coerced_value);
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

const char *ObjectProperty_SetItemValueRaw(
    ITEM *const item, const char *const name, const TRX_VALUE value)
{
    if (item == nullptr) {
        return "no such item";
    }

    const OBJECT_PROPERTY_ENTRY *const object_entry =
        M_GetObjectEntry(Object_TryGet(item->object_id), name);
    if (object_entry == nullptr) {
        return "no such property";
    }

    TRX_VALUE coerced_value = {};
    if (!Value_Coerce(object_entry->value.type, &value, &coerced_value)) {
        return "value is not of the property's type";
    }
    return M_ApplyItemValue(item, object_entry->name, &coerced_value);
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

RESULT ObjectProperty_ReadItemOverrides(
    JSON_READ_IO *const io, ITEM *const item)
{
    if (!JSON_ReadIO_HasKey(io, "properties")) {
        return OK;
    }
    MUST(JSON_PUSH(io, "properties"));

    const OBJECT *const obj =
        item == nullptr ? nullptr : Object_TryGet(item->object_id);
    if (obj == nullptr) {
        MUST(JSON_POP(io));
        return JSON_ReadIO_Fail(io, "the item names no object");
    }

    const JSON_OBJECT *const props = JSON_ReadIO_GetCurrentObject(io);
    for (int32_t i = 0; i < obj->properties.count; i++) {
        const OBJECT_PROPERTY_ENTRY *const entry = &obj->properties.entries[i];
        if (!JSON_ReadIO_HasKey(io, entry->name)) {
            continue;
        }

        TRX_VALUE value;
        const RESULT read = JSONValue_Read(
            props, entry->name, entry->value.type, nullptr, &value);
        if (!IS_OK(read)) {
            IGNORE(JSON_ReadIO_Pop(io));
            return read;
        }
        // A save is not worth losing over one value the property no longer
        // takes; the item keeps the object's.
        const char *const err = M_ApplyItemValue(item, entry->name, &value);
        if (err != nullptr) {
            LOG_WARNING("%s: %s", entry->name, err);
        }
    }

    return JSON_POP(io);
}

REGISTER_SUBSYSTEM(.shutdown = M_Shutdown)
