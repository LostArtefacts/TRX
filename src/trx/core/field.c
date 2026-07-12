#include <trx/core/field.h>

#include <trx/debug.h>

#include <string.h>

// Types self-register from constructors, so this must be a plain static array:
// it has to be usable before any engine subsystem is initialised.
#define M_MAX_TYPES 64
static const TYPE_DESC *m_Types[M_MAX_TYPES];
static int32_t m_TypeCount = 0;

void Type_Register(const TYPE_DESC *const type)
{
    ASSERT(m_TypeCount < M_MAX_TYPES);
    m_Types[m_TypeCount++] = type;
}

int32_t Type_GetCount(void)
{
    return m_TypeCount;
}

const TYPE_DESC *Type_GetAt(const int32_t idx)
{
    if (idx < 0 || idx >= m_TypeCount) {
        return nullptr;
    }
    return m_Types[idx];
}

const TYPE_DESC *Type_GetByName(const char *const name)
{
    for (int32_t i = 0; i < m_TypeCount; i++) {
        if (strcmp(m_Types[i]->name, name) == 0) {
            return m_Types[i];
        }
    }
    return nullptr;
}

const char *Field_GetTypeName(const FIELD_TYPE type)
{
    switch (type) {
    case FT_BOOL:
        return "BOOL";
    case FT_INT8:
        return "INT8";
    case FT_UINT8:
        return "UINT8";
    case FT_INT16:
        return "INT16";
    case FT_UINT16:
        return "UINT16";
    case FT_INT32:
        return "INT32";
    case FT_UINT32:
        return "UINT32";
    case FT_FLOAT:
        return "FLOAT";
    case FT_DOUBLE:
        return "DOUBLE";
    case FT_XYZ_16:
        return "XYZ_16";
    case FT_XYZ_32:
        return "XYZ_32";
    case FT_STRING:
        return "STRING";
    }
    return "UNKNOWN";
}

size_t Field_GetTypeSize(const FIELD_TYPE type)
{
    switch (type) {
    case FT_BOOL:
        return sizeof(bool);
    case FT_INT8:
        return sizeof(int8_t);
    case FT_UINT8:
        return sizeof(uint8_t);
    case FT_INT16:
        return sizeof(int16_t);
    case FT_UINT16:
        return sizeof(uint16_t);
    case FT_INT32:
        return sizeof(int32_t);
    case FT_UINT32:
        return sizeof(uint32_t);
    case FT_FLOAT:
        return sizeof(float);
    case FT_DOUBLE:
        return sizeof(double);
    case FT_XYZ_16:
        return sizeof(XYZ_16);
    case FT_XYZ_32:
        return sizeof(XYZ_32);
    case FT_STRING:
        return sizeof(char *);
    }
    return 0;
}

const char *Field_FindDuplicateName(const TYPE_DESC *const type)
{
    for (int32_t i = 0; i < type->field_count; i++) {
        for (int32_t j = i + 1; j < type->field_count; j++) {
            if (strcmp(type->fields[i].name, type->fields[j].name) == 0) {
                return type->fields[i].name;
            }
        }
    }
    return nullptr;
}

void Field_ValidateType(const TYPE_DESC *const type)
{
    const char *const dupe = Field_FindDuplicateName(type);
    if (dupe != nullptr) {
        LOG_ERROR("%s declares '%s' more than once", type->name, dupe);
        ASSERT_FAIL();
    }

    for (int32_t i = 0; i < type->field_count; i++) {
        const FIELD_DESC *const field = &type->fields[i];
        // Only fields that actually address memory carry a backing member to
        // check: Field_Get uses the offset when there is no getter, and
        // Field_Set uses it when there is no setter and the field is writable.
        const bool reads_member = field->get == nullptr;
        const bool writes_member =
            field->set == nullptr && !(field->flags & FF_READONLY);
        if (!reads_member && !writes_member) {
            continue;
        }
        ASSERT(field->size == Field_GetTypeSize(field->type));
    }
}

const FIELD_DESC *Field_Find(
    const TYPE_DESC *const type, const char *const name)
{
    for (int32_t i = 0; i < type->field_count; i++) {
        if (strcmp(type->fields[i].name, name) == 0) {
            return &type->fields[i];
        }
    }
    return nullptr;
}

bool Field_Get(
    const FIELD_DESC *const field, const void *const self,
    FIELD_VALUE *const out)
{
    if (field->get != nullptr) {
        return field->get(self, out);
    }

    const void *const p = (const char *)self + field->offset;
    out->type = field->type;
    switch (field->type) {
    case FT_BOOL:
        out->as_bool = *(const bool *)p;
        break;
    case FT_INT8:
        out->as_int = *(const int8_t *)p;
        break;
    case FT_UINT8:
        out->as_int = *(const uint8_t *)p;
        break;
    case FT_INT16:
        out->as_int = *(const int16_t *)p;
        break;
    case FT_UINT16:
        out->as_int = *(const uint16_t *)p;
        break;
    case FT_INT32:
        out->as_int = *(const int32_t *)p;
        break;
    case FT_UINT32:
        out->as_int = *(const uint32_t *)p;
        break;
    case FT_FLOAT:
        out->as_num = *(const float *)p;
        break;
    case FT_DOUBLE:
        out->as_num = *(const double *)p;
        break;
    case FT_XYZ_32:
        out->as_xyz = *(const XYZ_32 *)p;
        break;
    case FT_XYZ_16: {
        const XYZ_16 *const v = p;
        out->as_xyz = (XYZ_32) { .x = v->x, .y = v->y, .z = v->z };
        break;
    }
    case FT_STRING:
        out->as_str = *(const char *const *)p;
        break;
    }
    return true;
}

// Returns nullptr if `value` fits the integer `type`, else an error message.
// The reflection layer is the one place that knows every field's exact width,
// so it is where an out-of-range store must be rejected rather than silently
// truncated (e.g. writing 99999 into an int16 member).
static const char *M_CheckIntRange(const FIELD_TYPE type, const int64_t value)
{
    int64_t min = 0;
    int64_t max = 0;
    switch (type) {
    case FT_INT8:
        min = INT8_MIN;
        max = INT8_MAX;
        break;
    case FT_UINT8:
        min = 0;
        max = UINT8_MAX;
        break;
    case FT_INT16:
        min = INT16_MIN;
        max = INT16_MAX;
        break;
    case FT_UINT16:
        min = 0;
        max = UINT16_MAX;
        break;
    case FT_INT32:
        min = INT32_MIN;
        max = INT32_MAX;
        break;
    case FT_UINT32:
        min = 0;
        max = UINT32_MAX;
        break;
    default:
        return nullptr;
    }
    if (value < min || value > max) {
        return "value out of range for field";
    }
    return nullptr;
}

const char *Field_Set(
    const FIELD_DESC *const field, void *const self,
    const FIELD_VALUE *const in)
{
    if (field->flags & FF_READONLY) {
        return "field is read-only";
    }
    if (field->set != nullptr) {
        return field->set(self, in);
    }

    switch (field->type) {
    case FT_INT8:
    case FT_UINT8:
    case FT_INT16:
    case FT_UINT16:
    case FT_INT32:
    case FT_UINT32: {
        const char *const err = M_CheckIntRange(field->type, in->as_int);
        if (err != nullptr) {
            return err;
        }
        break;
    }
    case FT_XYZ_16:
        if (M_CheckIntRange(FT_INT16, in->as_xyz.x) != nullptr
            || M_CheckIntRange(FT_INT16, in->as_xyz.y) != nullptr
            || M_CheckIntRange(FT_INT16, in->as_xyz.z) != nullptr) {
            return "vector component out of range for field";
        }
        break;
    default:
        break;
    }

    void *const p = (char *)self + field->offset;
    switch (field->type) {
    case FT_BOOL:
        *(bool *)p = in->as_bool;
        break;
    case FT_INT8:
        *(int8_t *)p = in->as_int;
        break;
    case FT_UINT8:
        *(uint8_t *)p = in->as_int;
        break;
    case FT_INT16:
        *(int16_t *)p = in->as_int;
        break;
    case FT_UINT16:
        *(uint16_t *)p = in->as_int;
        break;
    case FT_INT32:
        *(int32_t *)p = in->as_int;
        break;
    case FT_UINT32:
        *(uint32_t *)p = in->as_int;
        break;
    case FT_FLOAT:
        *(float *)p = in->as_num;
        break;
    case FT_DOUBLE:
        *(double *)p = in->as_num;
        break;
    case FT_XYZ_32:
        *(XYZ_32 *)p = in->as_xyz;
        break;
    case FT_XYZ_16:
        *(XYZ_16 *)p = (XYZ_16) {
            .x = in->as_xyz.x,
            .y = in->as_xyz.y,
            .z = in->as_xyz.z,
        };
        break;
    case FT_STRING:
        return "string fields require a custom setter";
    }
    return nullptr;
}
