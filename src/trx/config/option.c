// One option: what it holds, and what may hold it away from that. Nothing here
// knows that other options exist; how an option reads and writes as a string is
// option_text.c.

#include <trx/config/option.h>

#include <trx/config/priv.h>
#include <trx/core/dynamic_enum.h>
#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/debug.h>

static bool M_IsStringLike(const TRX_VALUE_TYPE type)
{
    return type == TVT_STRING || type == TVT_DYNAMIC_ENUM;
}

// A string in a TRX_VALUE is borrowed by the type's own contract, so an option
// holding one keeps its own copy and these say where that copy is made and
// released. Everything else rides in the carrier and owns nothing.
static void M_FreeValue(TRX_VALUE *const value)
{
    if (M_IsStringLike(value->type)) {
        Memory_Free((char *)value->as_str);
        value->as_str = nullptr;
    }
}

static void M_CopyValue(TRX_VALUE *const dst, const TRX_VALUE *const src)
{
    *dst = *src;
    if (M_IsStringLike(src->type) && src->as_str != nullptr) {
        dst->as_str = Memory_DupStr(src->as_str);
    }
}

// Brings the carrier down to what the option's storage can hold, so that a 0.8
// read from the file and a 0.8f written in map.def are the same value
// afterwards. Without it every float option would read as changed from its
// default the moment the file was read.
//
// False where the storage cannot represent the value at all. The buffer is
// aligned as the carrier is, since the write goes through the storage type's
// own pointer.
static bool M_NarrowValue(TRX_VALUE *const value)
{
    if (M_IsStringLike(value->type)) {
        return true;
    }
    alignas(TRX_VALUE) char storage[sizeof(TRX_VALUE)] = {};
    if (Value_WritePtr(value->type, storage, value) != nullptr) {
        return false;
    }
    Value_ReadPtr(value->type, storage, value);
    return true;
}

static void M_ClearMirror(const CONFIG_OPTION *const option)
{
    if (option->mirror == nullptr) {
        return;
    }
    if (M_IsStringLike(option->default_value.type)) {
        Memory_FreePointer((char **)option->mirror);
        return;
    }
    const TRX_VALUE zero = { .type = option->default_value.type };
    Value_WritePtr(zero.type, option->mirror, &zero);
}

// Narrows the carrier into the field g_Config keeps for this option.
static void M_WriteMirror(const CONFIG_OPTION *const option)
{
    if (option->mirror == nullptr) {
        return;
    }
    if (M_IsStringLike(option->value.type)) {
        // g_Config holds a `char *` of its own rather than aliasing the
        // option's, so a reader that keeps the pointer is not left holding a
        // freed one the next time the setting moves.
        char **const slot = option->mirror;
        char *const old = *slot;
        *slot = option->value.as_str != nullptr
            ? Memory_DupStr(option->value.as_str)
            : nullptr;
        Memory_Free(old);
        return;
    }
    Value_WritePtr(option->value.type, option->mirror, &option->value);
}

// Puts a value into the option itself, no matter what asked for it. Every write
// lands here, so this is the one place that keeps g_Config in step and the one
// that says an option moved.
static void M_Apply(
    CONFIG_OPTION *const option, const TRX_VALUE *const value,
    const CONFIG_WRITE_KIND kind)
{
    TRX_VALUE copy;
    M_CopyValue(&copy, value);
    if (!M_NarrowValue(&copy)) {
        // g_Config could not follow a value the storage cannot hold, and an
        // option whose copy says something else is worse than a write that did
        // not happen. Everything that produces a value checks its range first,
        // so this is a guard rather than a path.
        M_FreeValue(&copy);
        return;
    }
    const bool moved = !Value_Equal(&copy, &option->value);
    M_FreeValue(&option->value);
    option->value = copy;
    M_WriteMirror(option);
    if (moved && kind != CONFIG_WRITE_SILENT) {
        Config_ReportChange(option, kind == CONFIG_WRITE_PERSIST);
    }
}

const void *Config_Option_GetEnumKey(const CONFIG_OPTION *const option)
{
    // A dynamic enum's values are keyed on the option itself, which is an
    // address that does not move for as long as the option exists.
    return option->value.type == TVT_DYNAMIC_ENUM ? (const void *)option
                                                  : option->enum_map;
}

void Config_Option_Init(
    CONFIG_OPTION *const option, const CONFIG_OPTION_DESC *const desc)
{
    option->name = Memory_DupStr(desc->name);
    option->mirror = desc->mirror;
    option->enum_map = desc->enum_map;
    option->bounds = desc->bounds != nullptr
        ? Memory_Dup(desc->bounds, sizeof(CONFIG_OPTION_BOUNDS))
        : nullptr;
    option->flags = desc->percent ? CONFIG_OPTION_PERCENT : 0;
    M_CopyValue(&option->default_value, &desc->default_value);
    M_NarrowValue(&option->default_value);
    // The value starts as the default, written properly so that g_Config comes
    // up holding it too.
    option->value.type = desc->default_value.type;
    M_Apply(option, &option->default_value, CONFIG_WRITE_SILENT);
}

void Config_Option_Free(CONFIG_OPTION *const option)
{
    // A dynamic enum's values are keyed on the option's own address, which the
    // allocator is free to hand out again. Dropping them here keeps the next
    // option to land on this address from finding this one's.
    if (option->value.type == TVT_DYNAMIC_ENUM) {
        DynamicEnum_ResetValues(Config_Option_GetEnumKey(option));
    }
    Config_Option_ReleaseHolds(option);
    M_FreeValue(&option->value);
    M_ClearMirror(option);
    M_FreeValue(&option->default_value);
    Memory_Free((CONFIG_OPTION_BOUNDS *)option->bounds);
    Memory_Free((char *)option->name);
}

void Config_Option_WriteAs(
    CONFIG_OPTION *const option, const TRX_VALUE *const value,
    const CONFIG_WRITE_KIND kind)
{
    ASSERT(option != nullptr);
    ASSERT(value != nullptr);
    ASSERT(value->type == option->value.type);

    // Something holding the option is what the player sees, so a write lands on
    // the hold rather than under it; the value underneath is the player's own
    // and comes back when the hold is lifted.
    if (option->hold_depth > 0) {
        CONFIG_HOLD *const hold = &option->holds[option->hold_depth - 1];
        TRX_VALUE copy;
        M_CopyValue(&copy, value);
        M_FreeValue(&hold->value);
        hold->value = copy;
        // A hold lives for as long as the game flow, script or demo that put it
        // there, so what lands on one is never the file's to keep: base_value,
        // which is what gets written, has not moved.
        M_Apply(
            option, value,
            kind == CONFIG_WRITE_PERSIST ? CONFIG_WRITE_TRANSIENT : kind);
        return;
    }
    M_Apply(option, value, kind);
}

void Config_Option_Write(
    CONFIG_OPTION *const option, const TRX_VALUE *const value)
{
    Config_Option_WriteAs(option, value, CONFIG_WRITE_PERSIST);
}

void Config_Option_Sanitize(CONFIG_OPTION *const option)
{
    ASSERT(option != nullptr);
    if (option->bounds == nullptr) {
        return;
    }

    TRX_VALUE clamped = option->value;
    switch (option->value.type) {
    case TVT_S8:
    case TVT_U8:
    case TVT_S16:
    case TVT_U16:
    case TVT_S32:
    case TVT_U32:
    case TVT_ENUM:
        CLAMP(
            clamped.as_int, (int64_t)option->bounds->min,
            (int64_t)option->bounds->max);
        break;
    case TVT_FLOAT:
    case TVT_DOUBLE:
        CLAMP(clamped.as_num, option->bounds->min, option->bounds->max);
        break;
    default:
        return;
    }
    if (!Value_Equal(&clamped, &option->value)) {
        M_Apply(option, &clamped, CONFIG_WRITE_SILENT);
    }
}

bool Config_Option_IsAtDefault(const CONFIG_OPTION *const option)
{
    ASSERT(option != nullptr);
    return Value_Equal(&option->value, &option->default_value);
}

bool Config_Option_RestoreDefault(CONFIG_OPTION *const option, const bool force)
{
    ASSERT(option != nullptr);
    if (option->hold_depth > 0 && !force) {
        return false;
    }
    Config_Option_Write(option, &option->default_value);
    return true;
}

const TRX_VALUE *Config_Option_GetBaseValue(const CONFIG_OPTION *const option)
{
    ASSERT(option != nullptr);
    return option->hold_depth > 0 ? &option->base_value : &option->value;
}

bool Config_Option_PushHold(
    CONFIG_OPTION *const option, const TRX_VALUE *const value,
    const CONFIG_HOLD_SOURCE source)
{
    ASSERT(option != nullptr);
    ASSERT(value != nullptr);
    ASSERT(value->type == option->value.type);

    if (option->hold_depth >= CONFIG_HOLD_MAX_DEPTH) {
        return false;
    }
    if (option->hold_depth == 0) {
        // Nothing is holding this option yet, so what is in it now is the
        // player's own value. That is the one to put back at the end.
        M_CopyValue(&option->base_value, &option->value);
    }

    CONFIG_HOLD *const hold = &option->holds[option->hold_depth];
    M_CopyValue(&hold->value, value);
    hold->source = source;
    option->hold_depth++;
    M_Apply(option, &hold->value, CONFIG_WRITE_TRANSIENT);
    return true;
}

bool Config_Option_PopHold(CONFIG_OPTION *const option)
{
    ASSERT(option != nullptr);
    if (option->hold_depth == 0) {
        return false;
    }

    option->hold_depth--;
    M_FreeValue(&option->holds[option->hold_depth].value);
    if (option->hold_depth == 0) {
        M_Apply(option, &option->base_value, CONFIG_WRITE_TRANSIENT);
        M_FreeValue(&option->base_value);
    } else {
        M_Apply(
            option, &option->holds[option->hold_depth - 1].value,
            CONFIG_WRITE_TRANSIENT);
    }
    return true;
}

void Config_Option_ReleaseHolds(CONFIG_OPTION *const option)
{
    ASSERT(option != nullptr);
    for (int32_t i = 0; i < option->hold_depth; i++) {
        M_FreeValue(&option->holds[i].value);
    }
    if (option->hold_depth > 0) {
        M_FreeValue(&option->base_value);
    }
    option->hold_depth = 0;
}

bool Config_Option_IsHeld(const CONFIG_OPTION *const option)
{
    ASSERT(option != nullptr);
    return option->hold_depth > 0;
}

bool Config_Option_IsEnforced(const CONFIG_OPTION *const option)
{
    ASSERT(option != nullptr);
    for (int32_t i = 0; i < option->hold_depth; i++) {
        if (option->holds[i].source == CONFIG_HOLD_GAME_FLOW) {
            return true;
        }
    }
    return false;
}

bool Config_Option_IsHidden(const CONFIG_OPTION *const option)
{
    ASSERT(option != nullptr);
    return (option->flags & CONFIG_OPTION_HIDDEN) != 0;
}
