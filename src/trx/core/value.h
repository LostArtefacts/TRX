#pragma once

#include <trx/core/colors.h>
#include <trx/core/math.h>

#include <stddef.h>
#include <stdint.h>

// The primitive value taxonomy shared by the config, reflection and object
// property layers. A type says how to address a stored value; the carrier holds
// one, widened.
typedef enum {
    TVT_BOOL,
    TVT_S8,
    TVT_U8,
    TVT_S16,
    TVT_U16,
    TVT_S32,
    TVT_U32,
    TVT_FLOAT,
    TVT_DOUBLE,
    TVT_XYZ_16,
    TVT_XYZ_32,
    TVT_RGB_888,
    TVT_STRING,
    // int32 storage, resolved to and from a string through an EnumMap named by
    // the `param` argument.
    TVT_ENUM,
    // string storage, validated and labelled through the dynamic enum registry
    // keyed on the token passed as `param`.
    TVT_DYNAMIC_ENUM,
} TRX_VALUE_TYPE;

// Widened carrier: every integer width rides in as_int and XYZ_16 in as_xyz,
// so a caller narrows on store by the type alone and never needs a second tag.
// as_str is borrowed - ownership stays with whoever supplied the string.
typedef struct {
    TRX_VALUE_TYPE type;
    union {
        bool as_bool;
        int64_t as_int;
        double as_num;
        XYZ_32 as_xyz;
        RGB_888 as_rgb;
        const char *as_str;
    };
} TRX_VALUE;

// The TRX_VALUE_TYPE that addresses a member of a plain C type, resolved from
// the member itself so a plain member cannot be mistagged. Listing the types
// rather than accepting anything means a member of a type nobody planned for -
// a pointer, a 64-bit integer - fails to compile instead of being addressed as
// something it is not. Enum members resolve to their integer storage type;
// enum-string mapping is a caller concern, not a member's.
#define Value_TypeOf(member_)                                                  \
    _Generic(                                                                  \
        (member_),                                                             \
        bool: TVT_BOOL,                                                        \
        int8_t: TVT_S8,                                                        \
        uint8_t: TVT_U8,                                                       \
        int16_t: TVT_S16,                                                      \
        uint16_t: TVT_U16,                                                     \
        int32_t: TVT_S32,                                                      \
        uint32_t: TVT_U32,                                                     \
        float: TVT_FLOAT,                                                      \
        double: TVT_DOUBLE,                                                    \
        XYZ_16: TVT_XYZ_16,                                                    \
        XYZ_32: TVT_XYZ_32,                                                    \
        RGB_888: TVT_RGB_888,                                                  \
        char *: TVT_STRING,                                                    \
        const char *: TVT_STRING)

// Stable machine-readable name, e.g. "S16", "XYZ_32", "ENUM".
const char *Value_TypeName(TRX_VALUE_TYPE type);

// Byte width a given type expects to address.
size_t Value_TypeSize(TRX_VALUE_TYPE type);

// Reads the value at `src` into `out`, widening integers into as_int and XYZ_16
// into as_xyz. `out->type` is set to `type`.
void Value_ReadPtr(TRX_VALUE_TYPE type, const void *src, TRX_VALUE *out);

// Returns nullptr if `in` fits `type`, else why not. XYZ_16 checks each
// component. Callers that write a member through a custom setter run this
// first, since the fit is a property of the storage, not of the setter.
const char *Value_CheckRange(TRX_VALUE_TYPE type, const TRX_VALUE *in);

// Brings `in` into the range `type` can hold by wrapping it, for a member that
// counts in cycles rather than along a line: an angle past the end of the turn
// names the same direction. Types with no range to speak of are left alone.
void Value_Wrap(TRX_VALUE_TYPE type, TRX_VALUE *value);

// Narrows `in` into the storage at `dst`. Returns nullptr on success or an
// error message; the range check runs first. String storage is caller-managed,
// so a string type reports an error rather than writing a borrowed pointer.
const char *Value_WritePtr(TRX_VALUE_TYPE type, void *dst, const TRX_VALUE *in);

// Resolves a string into `out`. `param` is the EnumMap name for TVT_ENUM and
// the dynamic enum token for TVT_DYNAMIC_ENUM; unused otherwise. A parsed
// string value borrows `str`. Returns false if the string does not fit.
bool Value_Parse(
    TRX_VALUE_TYPE type, const void *param, const char *str, TRX_VALUE *out);

// Serializes `value` to a static string (see String_FormatStatic). `human`
// selects a localized enum label over its machine name; `param` is as in
// Value_Parse.
const char *Value_Format(
    TRX_VALUE_TYPE type, const void *param, const TRX_VALUE *value, bool human);

// Converts a numeric value to `target`, among the integer and floating types.
// Returns false when no numeric conversion applies (bool, XYZ, string, enum,
// or a cross that would lose meaning). On an exact type match `in` is copied.
bool Value_Coerce(TRX_VALUE_TYPE target, const TRX_VALUE *in, TRX_VALUE *out);

// Whether the values stored at `a` and `b` are equal when read as `type`.
bool Value_EqualPtr(TRX_VALUE_TYPE type, const void *a, const void *b);

// Copies the value at `src` into the storage at `dst`, both addressed as
// `type`. String storage is owned: `dst` holds a `char *` that is freed and
// replaced with a duplicate of the string `src` points to, so `src` must be a
// `char **`.
void Value_CopyPtr(TRX_VALUE_TYPE type, void *dst, const void *src);
