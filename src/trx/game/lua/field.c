#include <trx/game/lua/field.h>

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
        // Only fields that address memory carry a backing member to check:
        // Field_Get uses the offset when there is no getter, and Field_Set uses
        // it when there is no setter and the field is writable.
        ASSERT((field->flags & FF_NULLABLE) == 0 || field->set != nullptr);
        const bool reads_member = field->get == nullptr;
        const bool writes_member =
            field->set == nullptr && !(field->flags & FF_READONLY);
        if (!reads_member && !writes_member) {
            continue;
        }
        ASSERT(field->size == Value_TypeSize(field->type));
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
    const FIELD_DESC *const field, const void *const self, TRX_VALUE *const out)
{
    if (field->get != nullptr) {
        return field->get(self, out);
    }
    Value_ReadPtr(field->type, (const char *)self + field->offset, out);
    return true;
}

const char *Field_Set(
    const FIELD_DESC *const field, void *const self, const TRX_VALUE *in)
{
    if (field->flags & FF_READONLY) {
        return "field is read-only";
    }

    // A null value names the state the member has outside its type, so it goes
    // to the setter as it is, with no width to check it against.
    if (in == nullptr) {
        ASSERT((field->flags & FF_NULLABLE) != 0);
        return field->set(self, nullptr);
    }

    // A cyclic member has no value outside its width: one is spelled the long
    // way round, so it comes back in rather than being turned away.
    TRX_VALUE wrapped;
    if ((field->flags & FF_MODULAR) != 0) {
        wrapped = *in;
        Value_Wrap(field->type, &wrapped);
        in = &wrapped;
    }

    // Whether the value fits is a property of the member's storage, not of the
    // setter, so it is checked before a custom setter runs too. A validating
    // setter guards its own semantics - a legal animation number, a room that
    // exists - and then assigns the widened carrier straight into the member;
    // it has no width of its own to check against.
    const char *const err = Value_CheckRange(field->type, in);
    if (err != nullptr) {
        return err;
    }

    if (field->set != nullptr) {
        return field->set(self, in);
    }

    return Value_WritePtr(field->type, (char *)self + field->offset, in);
}
