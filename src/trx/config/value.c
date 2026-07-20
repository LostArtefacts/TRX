// Copying a config value out of an option, putting it back, reaching it as a
// raw pointer, and freeing it. This layer knows nothing about overrides.

#include <trx/config/value.h>

#include <trx/core/memory.h>
#include <trx/core/value.h>
#include <trx/debug.h>

static bool M_IsString(const CONFIG_OPTION *const option)
{
    return option->type == TVT_STRING || option->type == TVT_DYNAMIC_ENUM;
}

void ConfigValue_Copy(
    const CONFIG_OPTION *const option, CONFIG_VALUE *const dst,
    const void *const src)
{
    ASSERT(option != nullptr);
    ASSERT(dst != nullptr);
    ASSERT(src != nullptr);

    // A CONFIG_VALUE is a fresh copy, so the string is duplicated rather than
    // replaced - there is no prior owner to free, unlike Value_CopyPtr.
    if (M_IsString(option)) {
        const char *const src_value = *(const char *const *)src;
        dst->string_value =
            src_value != nullptr ? Memory_DupStr(src_value) : nullptr;
        return;
    }
    TRX_VALUE value;
    Value_ReadPtr(option->type, src, &value);
    Value_WritePtr(option->type, dst, &value);
}

void ConfigValue_Free(
    const CONFIG_OPTION *const option, CONFIG_VALUE *const value)
{
    ASSERT(option != nullptr);
    ASSERT(value != nullptr);

    if (M_IsString(option)) {
        Memory_FreePointer(&value->string_value);
    }
}

const void *ConfigValue_GetPtr(
    const CONFIG_OPTION *const option, const CONFIG_VALUE *const value)
{
    ASSERT(option != nullptr);
    ASSERT(value != nullptr);
    // A CONFIG_VALUE is a union, so its address is the value of whichever type
    // the option holds - the raw pointer the rest of the config layer expects.
    return value;
}

void ConfigValue_Apply(
    const CONFIG_OPTION *const option, const CONFIG_VALUE *const value)
{
    ASSERT(option != nullptr);
    ASSERT(value != nullptr);
    Value_CopyPtr(option->type, (void *)option->target, value);
}
