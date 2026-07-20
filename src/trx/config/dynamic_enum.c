#include <trx/config/dynamic_enum.h>

#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/game_strings/entries.h>

#include <string.h>

typedef struct {
    char *value;
    char *label;
} M_DYNAMIC_ENUM_VALUE;

typedef struct M_DYNAMIC_ENUM_REGISTRY_ENTRY {
    const void *token;
    VECTOR *values;
    struct M_DYNAMIC_ENUM_REGISTRY_ENTRY *next;
} M_DYNAMIC_ENUM_REGISTRY_ENTRY;

static M_DYNAMIC_ENUM_REGISTRY_ENTRY *m_Registry = nullptr;

static bool M_IsSameValue(const char *const left, const char *const right)
{
    if (left == nullptr && right == nullptr) {
        return true;
    }
    if (left == nullptr || right == nullptr) {
        return false;
    }
    return strcmp(left, right) == 0;
}

static M_DYNAMIC_ENUM_REGISTRY_ENTRY *M_GetRegistryEntry(
    const void *const token, const bool create)
{
    if (token == nullptr) {
        return nullptr;
    }

    for (M_DYNAMIC_ENUM_REGISTRY_ENTRY *entry = m_Registry; entry != nullptr;
         entry = entry->next) {
        if (entry->token == token) {
            return entry;
        }
    }

    if (!create) {
        return nullptr;
    }

    M_DYNAMIC_ENUM_REGISTRY_ENTRY *const entry = Memory_Alloc(sizeof(*entry));
    entry->token = token;
    entry->values = Vector_Create(sizeof(M_DYNAMIC_ENUM_VALUE));
    entry->next = m_Registry;
    m_Registry = entry;
    return entry;
}

static void M_FreeValues(VECTOR **const values_ptr)
{
    if (values_ptr == nullptr || *values_ptr == nullptr) {
        return;
    }

    VECTOR *const values = *values_ptr;
    for (int32_t i = 0; i < values->count; i++) {
        M_DYNAMIC_ENUM_VALUE *const dyn_value = Vector_Get(values, i);
        Memory_FreePointer(&dyn_value->value);
        Memory_FreePointer(&dyn_value->label);
    }
    Vector_Free(values);
    *values_ptr = nullptr;
}

static int32_t M_FindValueIndex(
    const void *const token, const char *const value)
{
    const M_DYNAMIC_ENUM_REGISTRY_ENTRY *const entry =
        M_GetRegistryEntry(token, false);
    if (entry == nullptr || entry->values == nullptr) {
        return -1;
    }

    for (int32_t i = 0; i < entry->values->count; i++) {
        const M_DYNAMIC_ENUM_VALUE *const dyn_value =
            Vector_Get(entry->values, i);
        if (M_IsSameValue(dyn_value->value, value)) {
            return i;
        }
    }

    return -1;
}

static const M_DYNAMIC_ENUM_VALUE *M_GetValueEntry(
    const void *const token, const int32_t index)
{
    const M_DYNAMIC_ENUM_REGISTRY_ENTRY *const entry =
        M_GetRegistryEntry(token, false);
    if (entry == nullptr || entry->values == nullptr) {
        return nullptr;
    }
    if (index < 0 || index >= entry->values->count) {
        return nullptr;
    }
    return Vector_Get(entry->values, index);
}

static const char *M_GetDisplayLabel(
    const M_DYNAMIC_ENUM_VALUE *const dyn_value)
{
    if (dyn_value == nullptr) {
        return "(null)";
    }
    if (!String_IsEmpty(dyn_value->label)) {
        const char *const resolved = GameString_Get(dyn_value->label);
        if (!String_IsEmpty(resolved)) {
            return resolved;
        }
    }
    if (dyn_value->value != nullptr) {
        return dyn_value->value;
    }
    return "(null)";
}

static void M_Shutdown(void)
{
    M_DYNAMIC_ENUM_REGISTRY_ENTRY *entry = m_Registry;
    while (entry != nullptr) {
        M_DYNAMIC_ENUM_REGISTRY_ENTRY *const next = entry->next;
        M_FreeValues(&entry->values);
        Memory_FreePointer(&entry);
        entry = next;
    }
    m_Registry = nullptr;
}

__attribute__((destructor)) static void M_AtShutdown(void)
{
    M_Shutdown();
}

void DynamicEnum_ResetValues(const void *const token)
{
    if (token == nullptr) {
        return;
    }

    M_DYNAMIC_ENUM_REGISTRY_ENTRY *const entry =
        M_GetRegistryEntry(token, true);
    ASSERT(entry != nullptr);
    M_FreeValues(&entry->values);
    entry->values = Vector_Create(sizeof(M_DYNAMIC_ENUM_VALUE));
}

bool DynamicEnum_AddValue(
    const void *const token, const char *const value, const char *const label)
{
    if (token == nullptr) {
        return false;
    }

    M_DYNAMIC_ENUM_REGISTRY_ENTRY *const entry =
        M_GetRegistryEntry(token, true);
    ASSERT(entry != nullptr);
    if (entry->values == nullptr) {
        entry->values = Vector_Create(sizeof(M_DYNAMIC_ENUM_VALUE));
    }

    M_DYNAMIC_ENUM_VALUE dyn_value = {
        .value = value != nullptr ? Memory_DupStr(value) : nullptr,
        .label = label != nullptr ? Memory_DupStr(label) : nullptr,
    };
    Vector_Add(entry->values, &dyn_value);
    return true;
}

bool DynamicEnum_IsValidValue(const void *const token, const char *const value)
{
    return M_FindValueIndex(token, value) >= 0;
}

int32_t DynamicEnum_GetValueCount(const void *const token)
{
    const M_DYNAMIC_ENUM_REGISTRY_ENTRY *const entry =
        M_GetRegistryEntry(token, false);
    if (entry == nullptr || entry->values == nullptr) {
        return 0;
    }
    return entry->values->count;
}

const char *DynamicEnum_GetValueAt(const void *const token, const int32_t index)
{
    const M_DYNAMIC_ENUM_VALUE *const dyn_value = M_GetValueEntry(token, index);
    if (dyn_value == nullptr) {
        return nullptr;
    }
    return dyn_value->value;
}

const char *DynamicEnum_GetLabelAt(const void *const token, const int32_t index)
{
    const M_DYNAMIC_ENUM_VALUE *const dyn_value = M_GetValueEntry(token, index);
    return M_GetDisplayLabel(dyn_value);
}

const char *DynamicEnum_GetLabelForValue(
    const void *const token, const char *const value)
{
    const int32_t idx = M_FindValueIndex(token, value);
    if (idx < 0) {
        return value != nullptr ? value : "(null)";
    }
    return DynamicEnum_GetLabelAt(token, idx);
}

bool DynamicEnum_CanCycle(
    const void *const token, const char *const current, const int32_t dir)
{
    if (token == nullptr || dir == 0) {
        return false;
    }

    const int32_t value_count = DynamicEnum_GetValueCount(token);
    if (value_count <= 0) {
        return false;
    }

    const int32_t cur_idx = M_FindValueIndex(token, current);
    if (cur_idx < 0) {
        return true;
    }

    const int32_t step = dir < 0 ? -1 : 1;
    const int32_t next_idx = cur_idx + step;
    return next_idx >= 0 && next_idx < value_count;
}

const char *DynamicEnum_GetNext(
    const void *const token, const char *const current, const int32_t dir)
{
    if (token == nullptr || dir == 0) {
        return nullptr;
    }

    const int32_t value_count = DynamicEnum_GetValueCount(token);
    if (value_count <= 0) {
        return nullptr;
    }

    const int32_t cur_idx = M_FindValueIndex(token, current);
    if (cur_idx < 0) {
        return DynamicEnum_GetValueAt(token, 0);
    }

    const int32_t step = dir < 0 ? -1 : 1;
    const int32_t next_idx = cur_idx + step;
    if (next_idx < 0 || next_idx >= value_count) {
        return nullptr;
    }

    return DynamicEnum_GetValueAt(token, next_idx);
}
