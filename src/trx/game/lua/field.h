#pragma once

#include <trx/core/math.h>
#include <trx/core/utils.h>

#include <stddef.h>
#include <stdint.h>

typedef enum {
    FT_BOOL,
    FT_INT8,
    FT_UINT8,
    FT_INT16,
    FT_UINT16,
    FT_INT32,
    FT_UINT32,
    FT_FLOAT,
    FT_DOUBLE,
    FT_XYZ_16,
    FT_XYZ_32,
    FT_STRING,
} FIELD_TYPE;

// Widened carrier: every integer type rides in as_int, and XYZ_16 rides in
// as_xyz. FIELD_TYPE says how to narrow on store, so callers never need to.
typedef struct {
    FIELD_TYPE type;
    union {
        bool as_bool;
        int64_t as_int;
        double as_num;
        XYZ_32 as_xyz;
        const char *as_str;
    };
} FIELD_VALUE;

typedef enum {
    FF_NONE = 0,
    // A hard constraint, not a style choice: this member must never be written
    // through the reflection layer. Lua may narrow a field to read-only, but it
    // cannot widen one that is marked here.
    FF_READONLY = 1 << 0,
} FIELD_FLAGS;

typedef struct {
    const char *name; // C member name; an internal key, not the public API name
    FIELD_TYPE type;
    size_t offset; // used when get/set are null
    size_t size; // sizeof the member; validated against type at startup
    uint32_t flags;
    // Escape hatch for members that are computed, validated, or have side
    // effects. set() returns nullptr on success, or an error message.
    bool (*get)(const void *self, FIELD_VALUE *out);
    const char *(*set)(void *self, const FIELD_VALUE *in);
} FIELD_DESC;

typedef struct {
    const char *name;
    const FIELD_DESC *fields;
    int32_t field_count;
} TYPE_DESC;

// The FIELD_TYPE that addresses a member of this C type, resolved from the
// member itself. A plain member therefore cannot be mistagged: there is no
// second declaration of its type to disagree with it.
//
// This matters because sizeof alone cannot tell INT32 from UINT32 or FLOAT, nor
// INT16 from UINT16, so a hand-written tag that was wrong within its own width
// would read the right bytes as the wrong value - and Field_ValidateType, which
// only compares sizeof, would pass it. Hand-written accessors got this from the
// compiler for free; a table of offsets has to ask for it.
//
// An enum member resolves to whichever integer type the implementation made it
// compatible with, which is exactly how its storage must be addressed. Listing
// the types rather than accepting anything means a member of a type nobody
// thought about - a pointer, a 64-bit integer - fails to compile instead of
// being addressed as something it is not.
#define M_FIELD_TYPE_OF(member_)                                               \
    _Generic(                                                                  \
        (member_),                                                             \
        bool: FT_BOOL,                                                         \
        int8_t: FT_INT8,                                                       \
        uint8_t: FT_UINT8,                                                     \
        int16_t: FT_INT16,                                                     \
        uint16_t: FT_UINT16,                                                   \
        int32_t: FT_INT32,                                                     \
        uint32_t: FT_UINT32,                                                   \
        float: FT_FLOAT,                                                       \
        double: FT_DOUBLE,                                                     \
        XYZ_16: FT_XYZ_16,                                                     \
        XYZ_32: FT_XYZ_32,                                                     \
        char *: FT_STRING,                                                     \
        const char *: FT_STRING)

#define M_FIELD(struct_, member_, flags_)                                      \
    {                                                                          \
        .name = #member_,                                                      \
        .type = M_FIELD_TYPE_OF(((struct_ *)nullptr)->member_),                \
        .offset = offsetof(struct_, member_),                                  \
        .size = sizeof(((struct_ *)nullptr)->member_),                         \
        .flags = flags_,                                                       \
    }

// Plain struct member.
#define FIELD(struct_, member_) M_FIELD(struct_, member_, FF_NONE)

// Plain struct member that must never be written through reflection.
#define FIELD_RO(struct_, member_) M_FIELD(struct_, member_, FF_READONLY)

// Plain struct member whose write has side effects or needs validation.
#define FIELD_SET(struct_, member_, set_)                                      \
    {                                                                          \
        .name = #member_,                                                      \
        .type = M_FIELD_TYPE_OF(((struct_ *)nullptr)->member_),                \
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

// Defines a TYPE_DESC and adds it to the global type registry, so consumers
// that enumerate types (the API binder, the docs dump) need not know which
// types exist.
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

// Byte width a given FIELD_TYPE expects to address.
size_t Field_GetTypeSize(FIELD_TYPE type);

// Stable machine-readable name for a FIELD_TYPE, e.g. "INT16", "XYZ_32".
const char *Field_GetTypeName(FIELD_TYPE type);

// Returns the first name declared more than once in the table, or nullptr.
// Field_Find returns the first match, so a duplicate silently shadows every
// later entry - including a validating setter that then never runs.
const char *Field_FindDuplicateName(const TYPE_DESC *type);

// Assert every plain member's sizeof matches its declared FIELD_TYPE. Call once
// per type at startup.
//
// A member declared with FIELD/FIELD_RO/FIELD_SET cannot fail this: its type is
// resolved from the member itself. It remains the backstop for the entries the
// macros cannot see - a FIELD_FN whose declared type disagrees with what its
// getter returns, or a table built by hand.
void Field_ValidateType(const TYPE_DESC *type);

const FIELD_DESC *Field_Find(const TYPE_DESC *type, const char *name);
bool Field_Get(const FIELD_DESC *field, const void *self, FIELD_VALUE *out);
const char *Field_Set(
    const FIELD_DESC *field, void *self, const FIELD_VALUE *in);
