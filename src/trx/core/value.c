#include <trx/core/value.h>

#include <trx/core/dynamic_enum.h>
#include <trx/core/enum_map.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

// TAG, machine name, storage C type. Every type has a name and a storage width.
#define M_VALUE_TYPES(_)                                                       \
    _(TVT_BOOL, "BOOL", bool)                                                  \
    _(TVT_S8, "S8", int8_t)                                                    \
    _(TVT_U8, "U8", uint8_t)                                                   \
    _(TVT_S16, "S16", int16_t)                                                 \
    _(TVT_U16, "U16", uint16_t)                                                \
    _(TVT_S32, "S32", int32_t)                                                 \
    _(TVT_U32, "U32", uint32_t)                                                \
    _(TVT_FLOAT, "FLOAT", float)                                               \
    _(TVT_DOUBLE, "DOUBLE", double)                                            \
    _(TVT_XYZ_16, "XYZ_16", XYZ_16)                                            \
    _(TVT_XYZ_32, "XYZ_32", XYZ_32)                                            \
    _(TVT_RGB_888, "RGB_888", RGB_888)                                         \
    _(TVT_RGB_F, "RGB_F", RGB_F)                                               \
    _(TVT_STRING, "STRING", char *)                                            \
    _(TVT_ENUM, "ENUM", int32_t)                                               \
    _(TVT_DYNAMIC_ENUM, "DYNAMIC_ENUM", char *)

// TAG, storage C type, carrier member. The types a plain load or store moves
// without narrowing; XYZ_16 narrows and strings are caller-managed, so both are
// handled apart from this table.
#define M_VALUE_COPY_TYPES(_)                                                  \
    _(TVT_BOOL, bool, as_bool)                                                 \
    _(TVT_S8, int8_t, as_int)                                                  \
    _(TVT_U8, uint8_t, as_int)                                                 \
    _(TVT_S16, int16_t, as_int)                                                \
    _(TVT_U16, uint16_t, as_int)                                               \
    _(TVT_S32, int32_t, as_int)                                                \
    _(TVT_U32, uint32_t, as_int)                                               \
    _(TVT_ENUM, int32_t, as_int)                                               \
    _(TVT_FLOAT, float, as_num)                                                \
    _(TVT_DOUBLE, double, as_num)                                              \
    _(TVT_RGB_888, RGB_888, as_rgb)                                            \
    _(TVT_RGB_F, RGB_F, as_rgb_f)                                              \
    _(TVT_XYZ_32, XYZ_32, as_xyz)

// TAG, min, max. The integer widths a store range-checks. ENUM rides in an
// int32, so it checks the same bounds.
#define M_VALUE_INT_TYPES(_)                                                   \
    _(TVT_S8, INT8_MIN, INT8_MAX)                                              \
    _(TVT_U8, 0, UINT8_MAX)                                                    \
    _(TVT_S16, INT16_MIN, INT16_MAX)                                           \
    _(TVT_U16, 0, UINT16_MAX)                                                  \
    _(TVT_S32, INT32_MIN, INT32_MAX)                                           \
    _(TVT_ENUM, INT32_MIN, INT32_MAX)                                          \
    _(TVT_U32, 0, UINT32_MAX)

static bool M_IsInteger(const TRX_VALUE_TYPE type)
{
    switch (type) {
    case TVT_S8:
    case TVT_U8:
    case TVT_S16:
    case TVT_U16:
    case TVT_S32:
    case TVT_U32:
        return true;
    default:
        return false;
    }
}

static bool M_EqualNoCase(const char *a, const char *b)
{
    for (; *a != '\0' && *b != '\0'; a++, b++) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return false;
        }
    }
    return *a == *b;
}

static bool M_ParseBool(const char *const str, bool *const out)
{
    static const char *const truthy[] = { "on", "true", "1" };
    static const char *const falsy[] = { "off", "false", "0" };
    for (size_t i = 0; i < 3; i++) {
        if (M_EqualNoCase(str, truthy[i])) {
            *out = true;
            return true;
        }
        if (M_EqualNoCase(str, falsy[i])) {
            *out = false;
            return true;
        }
    }
    return false;
}

static uint8_t M_ChannelToByte(const float channel)
{
    const float scaled = channel * 255.0f;
    if (scaled <= 0.0f) {
        return 0;
    }
    if (scaled >= 255.0f) {
        return 255;
    }
    return (uint8_t)(scaled + 0.5f);
}

static bool M_ParseRGB(const char *str, RGB_888 *const out)
{
    if (str[0] == '#') {
        str++;
    }
    return sscanf(str, "%02hhX%02hhX%02hhX", &out->r, &out->g, &out->b) == 3;
}

// Resolves an EnumMap value from a string, false on a miss.
static bool M_LookUpEnum(
    const char *const name, const char *const str, int32_t *const out)
{
    // EnumMap_Get returns its default on a miss, and any value could be a
    // genuine result, so probe with two distinct defaults: a miss returns both,
    // a hit returns the mapped value twice.
    const int32_t r1 = EnumMap_Get(name, str, 0);
    const int32_t r2 = EnumMap_Get(name, str, 1);
    if (r1 != r2) {
        return false;
    }
    *out = r1;
    return true;
}

const char *Value_TypeName(const TRX_VALUE_TYPE type)
{
    switch (type) {
#define M_CASE(TAG, NAME, CTYPE)                                               \
    case TAG:                                                                  \
        return NAME;
        M_VALUE_TYPES(M_CASE)
#undef M_CASE
    }
    return "UNKNOWN";
}

size_t Value_TypeSize(const TRX_VALUE_TYPE type)
{
    switch (type) {
#define M_CASE(TAG, NAME, CTYPE)                                               \
    case TAG:                                                                  \
        return sizeof(CTYPE);
        M_VALUE_TYPES(M_CASE)
#undef M_CASE
    }
    return 0;
}

void Value_ReadPtr(
    const TRX_VALUE_TYPE type, const void *const src, TRX_VALUE *const out)
{
    out->type = type;
    switch (type) {
#define M_CASE(TAG, CTYPE, LANE)                                               \
    case TAG:                                                                  \
        out->LANE = *(const CTYPE *)src;                                       \
        break;
        M_VALUE_COPY_TYPES(M_CASE)
#undef M_CASE
    case TVT_XYZ_16: {
        const XYZ_16 *const v = src;
        out->as_xyz = (XYZ_32) { .x = v->x, .y = v->y, .z = v->z };
        break;
    }
    case TVT_STRING:
    case TVT_DYNAMIC_ENUM:
        out->as_str = *(const char *const *)src;
        break;
    }
}

const char *Value_CheckRange(
    const TRX_VALUE_TYPE type, const TRX_VALUE *const in)
{
    int64_t min = 0;
    int64_t max = 0;
    switch (type) {
#define M_CASE(TAG, MIN, MAX)                                                  \
    case TAG:                                                                  \
        min = (MIN);                                                           \
        max = (MAX);                                                           \
        break;
        M_VALUE_INT_TYPES(M_CASE)
#undef M_CASE
    case TVT_XYZ_16: {
        const int32_t comps[] = { in->as_xyz.x, in->as_xyz.y, in->as_xyz.z };
        for (size_t i = 0; i < 3; i++) {
            const TRX_VALUE c = { .type = TVT_S16, .as_int = comps[i] };
            if (Value_CheckRange(TVT_S16, &c) != nullptr) {
                return "vector component out of range";
            }
        }
        return nullptr;
    }
    default:
        return nullptr;
    }
    if (in->as_int < min || in->as_int > max) {
        return "value out of range";
    }
    return nullptr;
}

void Value_Wrap(const TRX_VALUE_TYPE type, TRX_VALUE *const value)
{
    int64_t min = 0;
    int64_t max = 0;
    switch (type) {
#define M_CASE(TAG, MIN, MAX)                                                  \
    case TAG:                                                                  \
        min = (MIN);                                                           \
        max = (MAX);                                                           \
        break;
        M_VALUE_INT_TYPES(M_CASE)
#undef M_CASE
    case TVT_XYZ_16: {
        int32_t *const comps[] = { &value->as_xyz.x, &value->as_xyz.y,
                                   &value->as_xyz.z };
        for (size_t i = 0; i < 3; i++) {
            TRX_VALUE comp = { .type = TVT_S16, .as_int = *comps[i] };
            Value_Wrap(TVT_S16, &comp);
            *comps[i] = (int32_t)comp.as_int;
        }
        return;
    }
    default:
        return;
    }

    const int64_t span = max - min + 1;
    value->as_int = (value->as_int - min) % span;
    if (value->as_int < 0) {
        value->as_int += span;
    }
    value->as_int += min;
}

const char *Value_WritePtr(
    const TRX_VALUE_TYPE type, void *const dst, const TRX_VALUE *const in)
{
    const char *const err = Value_CheckRange(type, in);
    if (err != nullptr) {
        return err;
    }

    switch (type) {
#define M_CASE(TAG, CTYPE, LANE)                                               \
    case TAG:                                                                  \
        *(CTYPE *)dst = in->LANE;                                              \
        break;
        M_VALUE_COPY_TYPES(M_CASE)
#undef M_CASE
    case TVT_XYZ_16:
        *(XYZ_16 *)dst = (XYZ_16) {
            .x = in->as_xyz.x,
            .y = in->as_xyz.y,
            .z = in->as_xyz.z,
        };
        break;
    case TVT_STRING:
    case TVT_DYNAMIC_ENUM:
        return "string storage is caller-managed";
    }
    return nullptr;
}

bool Value_Parse(
    const TRX_VALUE_TYPE type, const void *const param, const char *const str,
    TRX_VALUE *const out)
{
    out->type = type;
    if (str == nullptr) {
        // A null string only means anything for the string-storage types.
        switch (type) {
        case TVT_STRING:
            out->as_str = nullptr;
            return true;
        case TVT_DYNAMIC_ENUM:
            out->as_str = nullptr;
            return DynamicEnum_IsValidValue(param, nullptr);
        default:
            return false;
        }
    }
    switch (type) {
    case TVT_BOOL:
        return M_ParseBool(str, &out->as_bool);

    case TVT_S8:
    case TVT_U8:
    case TVT_S16:
    case TVT_U16:
    case TVT_S32:
    case TVT_U32:
        return sscanf(str, "%" SCNd64, &out->as_int) == 1;

    case TVT_FLOAT:
    case TVT_DOUBLE:
        return sscanf(str, "%lf", &out->as_num) == 1;

    case TVT_XYZ_16:
    case TVT_XYZ_32: {
        XYZ_32 v;
        if (sscanf(str, "%d %d %d", &v.x, &v.y, &v.z) != 3) {
            return false;
        }
        out->as_xyz = v;
        return true;
    }

    case TVT_RGB_888:
        return M_ParseRGB(str, &out->as_rgb);

    case TVT_RGB_F: {
        RGB_888 bytes;
        if (!M_ParseRGB(str, &bytes)) {
            return false;
        }
        out->as_rgb_f = (RGB_F) {
            .r = bytes.r / 255.0f,
            .g = bytes.g / 255.0f,
            .b = bytes.b / 255.0f,
        };
        return true;
    }

    case TVT_ENUM: {
        int32_t mapped;
        if (!M_LookUpEnum((const char *)param, str, &mapped)) {
            return false;
        }
        out->as_int = mapped;
        return true;
    }

    case TVT_DYNAMIC_ENUM:
        if (!DynamicEnum_IsValidValue(param, str)) {
            return false;
        }
        out->as_str = str;
        return true;

    case TVT_STRING:
        out->as_str = str;
        return true;
    }
    return false;
}

const char *Value_Format(
    const TRX_VALUE_TYPE type, const void *const param,
    const TRX_VALUE *const value, const bool human)
{
    switch (type) {
    case TVT_BOOL:
        return String_FormatStatic("%d", value->as_bool);
    case TVT_S8:
    case TVT_U8:
    case TVT_S16:
    case TVT_U16:
    case TVT_S32:
    case TVT_U32:
        return String_FormatStatic("%" PRId64, value->as_int);
    case TVT_FLOAT:
    case TVT_DOUBLE:
        return String_FormatStatic("%.2f", value->as_num);
    case TVT_XYZ_16:
    case TVT_XYZ_32:
        return String_FormatStatic(
            "%d %d %d", value->as_xyz.x, value->as_xyz.y, value->as_xyz.z);
    case TVT_RGB_888:
        return String_FormatStatic(
            "%02x%02x%02x", value->as_rgb.r, value->as_rgb.g, value->as_rgb.b);
    case TVT_RGB_F:
        return String_FormatStatic(
            "%02x%02x%02x", M_ChannelToByte(value->as_rgb_f.r),
            M_ChannelToByte(value->as_rgb_f.g),
            M_ChannelToByte(value->as_rgb_f.b));
    case TVT_ENUM: {
        if (human) {
            const char *const label =
                EnumMap_GetLabel((const char *)param, value->as_int);
            if (label != nullptr) {
                return label;
            }
        }
        return EnumMap_ToString((const char *)param, value->as_int);
    }
    case TVT_DYNAMIC_ENUM:
        if (human) {
            return DynamicEnum_GetLabelForValue(param, value->as_str);
        }
        return value->as_str != nullptr ? value->as_str : "";
    case TVT_STRING:
        return value->as_str != nullptr ? value->as_str : "";
    }
    return "";
}

bool Value_Coerce(
    const TRX_VALUE_TYPE target, const TRX_VALUE *const in,
    TRX_VALUE *const out)
{
    if (target == in->type) {
        *out = *in;
        out->type = target;
        return true;
    }

    const bool target_int = M_IsInteger(target);
    const bool target_num = target == TVT_FLOAT || target == TVT_DOUBLE;
    const bool in_int = M_IsInteger(in->type);
    const bool in_num = in->type == TVT_FLOAT || in->type == TVT_DOUBLE;

    out->type = target;
    if (target_int && in_int) {
        out->as_int = in->as_int;
        return true;
    }
    if (target_int && in_num) {
        out->as_int = (int64_t)in->as_num;
        return true;
    }
    if (target_num && in_int) {
        out->as_num = in->as_int;
        return true;
    }
    if (target_num && in_num) {
        out->as_num = in->as_num;
        return true;
    }
    return false;
}

void Value_CopyPtr(
    const TRX_VALUE_TYPE type, void *const dst, const void *const src)
{
    if (type == TVT_STRING || type == TVT_DYNAMIC_ENUM) {
        const char *const new_str = *(const char *const *)src;
        char *const old = *(char **)dst;
        *(char **)dst = new_str != nullptr ? Memory_DupStr(new_str) : nullptr;
        // Free after allocating: the new string lands on a different pointer,
        // and change subscribers compare pointers alone to tell it moved.
        Memory_Free(old);
        return;
    }
    TRX_VALUE v;
    Value_ReadPtr(type, src, &v);
    Value_WritePtr(type, dst, &v);
}

bool Value_EqualPtr(
    const TRX_VALUE_TYPE type, const void *const a, const void *const b)
{
    TRX_VALUE va;
    TRX_VALUE vb;
    Value_ReadPtr(type, a, &va);
    Value_ReadPtr(type, b, &vb);
    return Value_Equal(&va, &vb);
}

bool Value_Equal(const TRX_VALUE *const a, const TRX_VALUE *const b)
{
    if (a->type != b->type) {
        return false;
    }
    switch (a->type) {
    case TVT_BOOL:
        return a->as_bool == b->as_bool;
    case TVT_S8:
    case TVT_U8:
    case TVT_S16:
    case TVT_U16:
    case TVT_S32:
    case TVT_U32:
    case TVT_ENUM:
        return a->as_int == b->as_int;
    case TVT_FLOAT:
    case TVT_DOUBLE:
        return a->as_num == b->as_num;
    case TVT_XYZ_16:
    case TVT_XYZ_32:
        return a->as_xyz.x == b->as_xyz.x && a->as_xyz.y == b->as_xyz.y
            && a->as_xyz.z == b->as_xyz.z;
    case TVT_RGB_888:
        return a->as_rgb.r == b->as_rgb.r && a->as_rgb.g == b->as_rgb.g
            && a->as_rgb.b == b->as_rgb.b;
    case TVT_RGB_F:
        return a->as_rgb_f.r == b->as_rgb_f.r && a->as_rgb_f.g == b->as_rgb_f.g
            && a->as_rgb_f.b == b->as_rgb_f.b;
    case TVT_STRING:
    case TVT_DYNAMIC_ENUM:
        if (a->as_str == nullptr || b->as_str == nullptr) {
            return a->as_str == b->as_str;
        }
        return strcmp(a->as_str, b->as_str) == 0;
    }
    return false;
}
