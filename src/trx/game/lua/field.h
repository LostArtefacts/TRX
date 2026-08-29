#pragma once

#include <trx/core/utils.h>
#include <trx/core/value.h>

#include <stddef.h>
#include <stdint.h>

typedef enum {
    FF_NONE = 0,
    // A hard constraint, not a style choice: this member must never be written
    // through the reflection layer. Lua may narrow a field to read-only, but it
    // cannot widen one that is marked here.
    FF_READONLY = 1 << 0,
    // The member counts in cycles rather than along a line - an angle - so a
    // value outside its width names a value inside it, and wraps instead of
    // being rejected.
    FF_MODULAR = 1 << 1,
    // The member has a state that no value names, so nil reaches its setter as
    // a null value rather than being turned away as the wrong type. Only a
    // member with a setter can carry this.
    FF_NULLABLE = 1 << 2,
} FIELD_FLAGS;

typedef struct {
    const char *name; // C member name; an internal key, not the public API name
    TRX_VALUE_TYPE type;
    size_t offset; // used when get/set are null
    size_t size; // sizeof the member; validated against type at startup
    uint32_t flags;
    // Escape hatch for members that are computed, validated, or have side
    // effects. set() returns nullptr on success, or an error message.
    bool (*get)(const void *self, TRX_VALUE *out);
    const char *(*set)(void *self, const TRX_VALUE *in);
} FIELD_DESC;

typedef struct {
    const char *name;
    const FIELD_DESC *fields;
    int32_t field_count;
} TYPE_DESC;

#define M_FIELD(struct_, member_, flags_)                                      \
    {                                                                          \
        .name = #member_,                                                      \
        .type = Value_TypeOf(((struct_ *)nullptr)->member_),                   \
        .offset = offsetof(struct_, member_),                                  \
        .size = sizeof(((struct_ *)nullptr)->member_),                         \
        .flags = flags_,                                                       \
    }

// Plain struct member.
#define FIELD(struct_, member_) M_FIELD(struct_, member_, FF_NONE)

// Plain struct member that must never be written through reflection.
#define FIELD_RO(struct_, member_) M_FIELD(struct_, member_, FF_READONLY)

// Plain struct member holding a cyclic quantity, such as an angle.
#define FIELD_MODULAR(struct_, member_) M_FIELD(struct_, member_, FF_MODULAR)

// Plain struct member whose write has side effects or needs validation.
#define FIELD_SET(struct_, member_, set_)                                      \
    {                                                                          \
        .name = #member_,                                                      \
        .type = Value_TypeOf(((struct_ *)nullptr)->member_),                   \
        .offset = offsetof(struct_, member_),                                  \
        .size = sizeof(((struct_ *)nullptr)->member_),                         \
        .set = set_,                                                           \
    }

// Computed member with no backing field. Pass nullptr for set_ to make it
// read-only. `name_` is still a C-side key, not an API name.
#define FIELD_FN(name_, type_, get_, set_)                                     \
    {                                                                          \
        .name = name_,                                                         \
        .type = type_,                                                         \
        .get = get_,                                                           \
        .set = set_,                                                           \
        .flags = (set_) == nullptr ? FF_READONLY : FF_NONE,                    \
    }

// Computed member whose setter also takes nil, for a member with a state that
// no value names. The setter is handed a null value where nil is written.
#define FIELD_FN_NULLABLE(name_, type_, get_, set_)                            \
    {                                                                          \
        .name = name_,                                                         \
        .type = type_,                                                         \
        .get = get_,                                                           \
        .set = set_,                                                           \
        .flags = FF_NULLABLE,                                                  \
    }

// Defines a TYPE_DESC and adds it to the global type registry, so consumers
// that enumerate types (the API binder, the docs dump) need not know which
// types exist.
// A type whose whole surface is methods. The fields are what a script reads off
// the struct itself, and a type that answers everything through calls has none.
#define TYPE_DEFINE_METHODS_ONLY(struct_)                                      \
    const TYPE_DESC TYPE_##struct_ = {                                         \
        .name = #struct_,                                                      \
        .fields = nullptr,                                                     \
        .field_count = 0,                                                      \
    };                                                                         \
    __attribute__((constructor)) static void M_RegisterType##struct_(void)     \
    {                                                                          \
        Type_Register(&TYPE_##struct_);                                        \
    }

#define TYPE_DEFINE(struct_, fields_)                                          \
    const TYPE_DESC TYPE_##struct_ = {                                         \
        .name = #struct_,                                                      \
        .fields = fields_,                                                     \
        .field_count = ARRAY_SIZE(fields_),                                    \
    };                                                                         \
    __attribute__((constructor)) static void M_RegisterType##struct_(void)     \
    {                                                                          \
        Type_Register(&TYPE_##struct_);                                        \
    }

void Type_Register(const TYPE_DESC *type);
int32_t Type_GetCount(void);
const TYPE_DESC *Type_GetAt(int32_t idx);
const TYPE_DESC *Type_GetByName(const char *name);

// Returns the first name declared more than once in the table, or nullptr.
// Field_Find returns the first match, so a duplicate silently shadows every
// later entry - including a validating setter that then never runs.
const char *Field_FindDuplicateName(const TYPE_DESC *type);

// Assert every plain member's sizeof matches its declared type. Call once per
// type at startup.
//
// A member declared with FIELD/FIELD_RO/FIELD_SET cannot fail this: its type is
// resolved from the member itself. It remains the backstop for the entries the
// macros cannot see - a FIELD_FN whose declared type disagrees with what its
// getter returns, or a table built by hand.
void Field_ValidateType(const TYPE_DESC *type);

const FIELD_DESC *Field_Find(const TYPE_DESC *type, const char *name);
bool Field_Get(const FIELD_DESC *field, const void *self, TRX_VALUE *out);
const char *Field_Set(const FIELD_DESC *field, void *self, const TRX_VALUE *in);
