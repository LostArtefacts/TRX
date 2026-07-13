// The four things anyone does with a config value: copy it out of an option,
// put it back, reach it as a raw pointer, and let it go. Nothing here knows
// what an override is, or that the game exists.

#include <trx/config/value.h>

#include <trx/core/memory.h>
#include <trx/debug.h>

void ConfigValue_Copy(
    const CONFIG_OPTION *const option, CONFIG_VALUE *const dst,
    const void *const src)
{
    ASSERT(option != nullptr);
    ASSERT(dst != nullptr);
    ASSERT(src != nullptr);

    switch (option->type) {
    case COT_BOOL:
        dst->bool_value = *(const bool *)src;
        break;
    case COT_INT32:
        dst->int32_value = *(const int32_t *)src;
        break;
    case COT_ENUM:
        dst->int32_value = *(const int *)src;
        break;
    case COT_FLOAT:
    case COT_FLOAT_PERCENT:
        dst->float_value = *(const float *)src;
        break;
    case COT_DOUBLE:
        dst->double_value = *(const double *)src;
        break;
    case COT_RGB888:
        dst->rgb888_value = *(const RGB_888 *)src;
        break;
    case COT_STRING:
    case COT_DYNAMIC_ENUM: {
        const char *const src_value = *(const char *const *)src;
        dst->string_value =
            src_value != nullptr ? Memory_DupStr(src_value) : nullptr;
        break;
    }
    }
}

void ConfigValue_Free(
    const CONFIG_OPTION *const option, CONFIG_VALUE *const value)
{
    ASSERT(option != nullptr);
    ASSERT(value != nullptr);

    if (option->type == COT_STRING || option->type == COT_DYNAMIC_ENUM) {
        Memory_FreePointer(&value->string_value);
    }
}

const void *ConfigValue_GetPtr(
    const CONFIG_OPTION *const option, const CONFIG_VALUE *const value)
{
    ASSERT(option != nullptr);
    ASSERT(value != nullptr);

    switch (option->type) {
    case COT_BOOL:
        return &value->bool_value;
    case COT_INT32:
    case COT_ENUM:
        return &value->int32_value;
    case COT_FLOAT:
    case COT_FLOAT_PERCENT:
        return &value->float_value;
    case COT_DOUBLE:
        return &value->double_value;
    case COT_RGB888:
        return &value->rgb888_value;
    case COT_STRING:
    case COT_DYNAMIC_ENUM:
        return &value->string_value;
    }
    return nullptr;
}

void ConfigValue_Apply(
    const CONFIG_OPTION *const option, const CONFIG_VALUE *const value)
{
    ASSERT(option != nullptr);
    ASSERT(value != nullptr);

    switch (option->type) {
    case COT_BOOL:
        *(bool *)option->target = value->bool_value;
        break;
    case COT_INT32:
        *(int32_t *)option->target = value->int32_value;
        break;
    case COT_ENUM:
        *(int *)option->target = value->int32_value;
        break;
    case COT_FLOAT:
    case COT_FLOAT_PERCENT:
        *(float *)option->target = value->float_value;
        break;
    case COT_DOUBLE:
        *(double *)option->target = value->double_value;
        break;
    case COT_RGB888:
        *(RGB_888 *)option->target = value->rgb888_value;
        break;
    case COT_STRING:
    case COT_DYNAMIC_ENUM: {
        char **const p = (char **)option->target;
        char *const old = *p;
        *p = value->string_value != nullptr ? Memory_DupStr(value->string_value)
                                            : nullptr;
        Memory_Free(old);
        break;
    }
    }
}
