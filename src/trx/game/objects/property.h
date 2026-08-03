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
    // Where an item keeps its live copy: a member of the object's priv struct,
    // addressed as `field_type`. The engine writes it as the item is
    // initialised and on every later change, so object code reads the member
    // and never the store. A zero `field_size` means the property has no member
    // of its own.
    size_t field_offset;
    TRX_VALUE_TYPE field_type;
    size_t field_size;
    // Whether the member belongs to the item itself rather than to the priv
    // struct its object declares.
    bool on_item;
    // Whether the property will take the value at all. It is asked without an
    // item, so it runs wherever a value is stated - including before the level
    // has items to write it to. Returns nullptr once the value is accepted, or
    // why it was refused.
    const char *(*check)(const TRX_VALUE *value);
    // Where the value goes when the plain member write will not do - a value
    // converted to engine units, one member of several. Runs in place of the
    // member write, at the same moments, and only for a value `check` has
    // already accepted.
    void (*set)(ITEM *item, const TRX_VALUE *value);
} OBJECT_PROPERTY_DECL;

typedef struct OBJECT_PROPERTY_ENTRY OBJECT_PROPERTY_ENTRY;
typedef struct {
    int32_t count;
    OBJECT_PROPERTY_ENTRY *entries;
} OBJECT_PROPERTY_SET;

typedef OBJECT_PROPERTY_SET ITEM_PROPERTY_SET;

// A property an item carries in the named member of the object's priv struct.
// It is named after the member, and takes its type, width, offset and the type
// of its default from the member and the value themselves, so no two of them
// can disagree.
#define OBJECT_PROPERTY(struct_, member_, value_, description_)                \
    {                                                                          \
        .name = #member_,                                                      \
        .description = description_,                                           \
        .value = Value_Of(value_),                                             \
        .field_offset = offsetof(struct_, member_),                            \
        .field_type = Value_TypeOf(((struct_ *)nullptr)->member_),             \
        .field_size = sizeof(((struct_ *)nullptr)->member_),                   \
    }

// A bound property that takes only some of what its member could hold.
#define OBJECT_PROPERTY_CHECKED(                                               \
    struct_, member_, value_, check_, description_)                            \
    {                                                                          \
        .name = #member_,                                                      \
        .description = description_,                                           \
        .value = Value_Of(value_),                                             \
        .field_offset = offsetof(struct_, member_),                            \
        .field_type = Value_TypeOf(((struct_ *)nullptr)->member_),             \
        .field_size = sizeof(((struct_ *)nullptr)->member_),                   \
        .check = check_,                                                       \
    }

// A bound property whose member the store cannot write on its own. `check_` may
// be nullptr where the member takes anything of its width.
#define OBJECT_PROPERTY_SETTER(                                                \
    struct_, member_, value_, check_, set_, description_)                      \
    {                                                                          \
        .name = #member_,                                                      \
        .description = description_,                                           \
        .value = Value_Of(value_),                                             \
        .field_offset = offsetof(struct_, member_),                            \
        .field_type = Value_TypeOf(((struct_ *)nullptr)->member_),             \
        .field_size = sizeof(((struct_ *)nullptr)->member_),                   \
        .check = check_,                                                       \
        .set = set_,                                                           \
    }

// A property an item carries in a member of its own rather than in what its
// object keeps privately: one the engine reads for every object, of any kind.
// `set_` may be nullptr.
#define OBJECT_PROPERTY_ITEM(member_, value_, check_, set_, description_)      \
    {                                                                          \
        .name = #member_,                                                      \
        .description = description_,                                           \
        .value = Value_Of(value_),                                             \
        .field_offset = offsetof(ITEM, member_),                               \
        .field_type = Value_TypeOf(((ITEM *)nullptr)->member_),                \
        .field_size = sizeof(((ITEM *)nullptr)->member_),                      \
        .on_item = true,                                                       \
        .check = check_,                                                       \
        .set = set_,                                                           \
    }

// A property the store alone keeps, read back through ObjectProperty_Get*Value:
// an object-level setting a script tunes, with no item to carry it.
#define OBJECT_PROPERTY_STORED(name_, value_, description_)                    \
    {                                                                          \
        .name = name_,                                                         \
        .description = description_,                                           \
        .value = Value_Of(value_),                                             \
    }

// A property the object keeps somewhere the store cannot address on its own,
// with no member of its own to be held to a width.
#define OBJECT_PROPERTY_STORED_SETTER(                                         \
    name_, value_, check_, set_, description_)                                 \
    {                                                                          \
        .name = name_,                                                         \
        .description = description_,                                           \
        .value = Value_Of(value_),                                             \
        .check = check_,                                                       \
        .set = set_,                                                           \
    }

void ObjectProperty_ResetObject(OBJECT *obj);
void ObjectProperty_ResetItem(ITEM *item);
void ObjectProperty_ApplyDeclarations(
    OBJECT *obj, const OBJECT_PROPERTY_DECL *declarations, size_t count);

// Writes every bound property into the item's priv struct. Called as an item is
// initialised, before the object's own initialiser runs, so the object can read
// what it declared. A later change to a property writes the member again, which
// is why object code never has to look a property up.
void ObjectProperty_ApplyToItem(ITEM *item);

// The setters return nullptr once the value is stored, or why it was refused,
// as Field_Set does for a plain member. A value outside what the property's
// member can hold, or outside what its `check` accepts, is turned away rather
// than brought into range: the store answers every reader, so it may only hold
// what the items will take. The refusal does not wait for an item to take it
// to, so a value stated before the level has items is answered where it is
// stated.
bool ObjectProperty_GetObjectValue(
    const OBJECT *obj, const char *name, TRX_VALUE *out_value);
const char *ObjectProperty_SetObjectValueRaw(
    OBJECT *obj, const char *name, TRX_VALUE value);

bool ObjectProperty_GetItemValue(
    const ITEM *item, const char *name, TRX_VALUE *out_value);
const char *ObjectProperty_SetItemValueRaw(
    ITEM *item, const char *name, TRX_VALUE value);

int32_t ObjectProperty_GetObjectNameCount(const OBJECT *obj);
const char *ObjectProperty_GetObjectName(const OBJECT *obj, int32_t index);

int32_t ObjectProperty_GetItemNameCount(const ITEM *item);
const char *ObjectProperty_GetItemName(const ITEM *item, int32_t index);

void ObjectProperty_WriteItemOverrides(
    JSON_WRITE_IO *io, const ITEM *item, const char *key);
bool ObjectProperty_ReadItemOverrides(JSON_READ_IO *io, ITEM *item);
