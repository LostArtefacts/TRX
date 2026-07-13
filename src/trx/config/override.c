// The runtime override stack. It knows how to hold a value away from an option
// and put it back; it does not know where options come from, or how a string
// becomes a value. Both of those it asks the config layer for.

#include <trx/config/override.h>

#include <trx/config/common.h>
#include <trx/config/value.h>
#include <trx/core/memory.h>
#include <trx/core/vector.h>
#include <trx/debug.h>

#define CONFIG_OVERRIDE_MAX_DEPTH 3

typedef struct {
    const CONFIG_OPTION *option;
    CONFIG_VALUE base_value;
    CONFIG_VALUE override_values[CONFIG_OVERRIDE_MAX_DEPTH];
    int32_t depth;
} M_OVERRIDE;

static VECTOR *m_Overrides = nullptr;

static int32_t M_GetIndex(const CONFIG_OPTION *const option)
{
    if (m_Overrides == nullptr) {
        return -1;
    }

    for (int32_t i = 0; i < m_Overrides->count; i++) {
        const M_OVERRIDE *const override = Vector_Get(m_Overrides, i);
        if (override->option->target == option->target) {
            return i;
        }
    }
    return -1;
}

static void M_Free(M_OVERRIDE *const override)
{
    ASSERT(override != nullptr);

    ConfigValue_Free(override->option, &override->base_value);
    for (int32_t i = 0; i < override->depth; i++) {
        ConfigValue_Free(override->option, &override->override_values[i]);
    }
}

bool ConfigOverride_Push(
    const CONFIG_OPTION *const option, const void *const value)
{
    ASSERT(option != nullptr);
    ASSERT(value != nullptr);

    if (m_Overrides == nullptr) {
        m_Overrides = Vector_Create(sizeof(M_OVERRIDE));
    }

    int32_t override_idx = M_GetIndex(option);
    if (override_idx == -1) {
        // Nothing is holding this option yet, so what is in it now is the
        // player's own value. That is the one to put back at the end.
        M_OVERRIDE override = {
            .option = option,
            .depth = 0,
        };
        ConfigValue_Copy(option, &override.base_value, option->target);
        Vector_Add(m_Overrides, &override);
        override_idx = m_Overrides->count - 1;
    }

    M_OVERRIDE *const override = Vector_Get(m_Overrides, override_idx);
    if (override->depth >= CONFIG_OVERRIDE_MAX_DEPTH) {
        return false;
    }

    CONFIG_VALUE *const override_value =
        &override->override_values[override->depth];
    ConfigValue_Copy(option, override_value, value);
    override->depth++;
    ConfigValue_Apply(option, override_value);
    return true;
}

bool ConfigOverride_Pop(const CONFIG_OPTION *const option)
{
    ASSERT(option != nullptr);

    const int32_t override_idx = M_GetIndex(option);
    if (override_idx == -1) {
        return false;
    }

    M_OVERRIDE *const override = Vector_Get(m_Overrides, override_idx);
    ASSERT(override->depth > 0);

    override->depth--;
    ConfigValue_Free(
        override->option, &override->override_values[override->depth]);

    if (override->depth == 0) {
        ConfigValue_Apply(override->option, &override->base_value);
        M_Free(override);
        Vector_RemoveAt(m_Overrides, override_idx);
    } else {
        ConfigValue_Apply(
            override->option, &override->override_values[override->depth - 1]);
    }

    return true;
}

bool ConfigOverride_IsOverridden(const CONFIG_OPTION *const option)
{
    ASSERT(option != nullptr);
    return M_GetIndex(option) != -1;
}

const void *ConfigOverride_GetBaseValuePtr(const CONFIG_OPTION *const option)
{
    ASSERT(option != nullptr);

    const int32_t override_idx = M_GetIndex(option);
    if (override_idx == -1) {
        return option->target;
    }

    const M_OVERRIDE *const override = Vector_Get(m_Overrides, override_idx);
    return ConfigValue_GetPtr(option, &override->base_value);
}

void ConfigOverride_Clear(void)
{
    if (m_Overrides == nullptr) {
        return;
    }

    for (int32_t i = 0; i < m_Overrides->count; i++) {
        M_Free(Vector_Get(m_Overrides, i));
    }
    Vector_Clear(m_Overrides);
}

void ConfigOverride_Shutdown(void)
{
    if (m_Overrides == nullptr) {
        return;
    }
    ConfigOverride_Clear();
    Vector_Free(m_Overrides);
    m_Overrides = nullptr;
}
