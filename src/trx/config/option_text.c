// An option as a string: what the settings dialogs and the console show, and
// what they read back. Kept apart from the option's own state because this is
// the half that needs the game's own strings.

#include <trx/config/option.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/debug.h>
#include <trx/game/game_strings/entries.h>

// The two presentations Value_Format does not carry, because each reaches past
// the stored value: a bool as a localized on/off, and a float as a percentage.
static const char *M_FormatBoolHuman(const bool value)
{
    return value ? GS("general/misc/on") : GS("general/misc/off");
}

static const char *M_FormatFloatPercent(const float value)
{
    return String_FormatStatic("%.0f%%", value);
}

// Reads a string as the option's own value. A percentage is entered the way it
// is shown, so it comes back to the 0..1 the option stores.
static bool M_ParseValue(
    const CONFIG_OPTION *const option, const char *const str,
    TRX_VALUE *const out)
{
    if (!Value_Parse(
            option->value.type, Config_Option_GetEnumKey(option), str, out)) {
        return false;
    }
    if ((option->flags & CONFIG_OPTION_PERCENT) != 0) {
        out->as_num /= 100.0;
    }
    return Value_CheckRange(option->value.type, out) == nullptr;
}

bool Config_Option_SetFromString(
    CONFIG_OPTION *const option, const char *const new_value, const bool force)
{
    ASSERT(option != nullptr);
    if (!force && Config_Option_IsHeld(option)) {
        return false;
    }
    TRX_VALUE parsed;
    if (!M_ParseValue(option, new_value, &parsed)) {
        return false;
    }
    Config_Option_Write(option, &parsed);
    return true;
}

bool Config_Option_PushHoldFromString(
    CONFIG_OPTION *const option, const char *const value,
    const CONFIG_HOLD_SOURCE source)
{
    ASSERT(option != nullptr);
    if (Config_Option_IsEnforced(option)) {
        return false;
    }
    TRX_VALUE parsed;
    if (!M_ParseValue(option, value, &parsed)) {
        return false;
    }
    return Config_Option_PushHold(option, &parsed, source);
}

const char *Config_Option_GetValueAsString(
    const CONFIG_OPTION *const option, const bool human_readable)
{
    if (option == nullptr) {
        return nullptr;
    }
    if (human_readable && option->value.type == TVT_BOOL) {
        return M_FormatBoolHuman(option->value.as_bool);
    }
    if ((option->flags & CONFIG_OPTION_PERCENT) != 0) {
        return M_FormatFloatPercent(option->value.as_num * 100.0);
    }
    return Value_Format(
        option->value.type, Config_Option_GetEnumKey(option), &option->value,
        human_readable);
}

char *Config_Option_NormalizeValueString(
    const CONFIG_OPTION *const option, const char *const value,
    const bool human_readable)
{
    if (option == nullptr) {
        return Memory_DupStr(value != nullptr ? value : "");
    }

    const char *const input = value != nullptr ? value : "";
    const void *const enum_key = Config_Option_GetEnumKey(option);

    TRX_VALUE parsed;
    if (!Value_Parse(option->value.type, enum_key, input, &parsed)) {
        return Memory_DupStr(input);
    }
    if (human_readable && option->value.type == TVT_BOOL) {
        return Memory_DupStr(M_FormatBoolHuman(parsed.as_bool));
    }
    if ((option->flags & CONFIG_OPTION_PERCENT) != 0) {
        return Memory_DupStr(M_FormatFloatPercent(parsed.as_num));
    }
    return Memory_DupStr(
        Value_Format(option->value.type, enum_key, &parsed, human_readable));
}

const char *Config_Option_GetTitle(const CONFIG_OPTION *const option)
{
    if (option == nullptr || option->name == nullptr) {
        return nullptr;
    }
    return GameString_Get(
        String_FormatStatic("settings/%s/title", option->name));
}

const char *Config_Option_GetDescription(const CONFIG_OPTION *const option)
{
    if (option == nullptr || option->name == nullptr) {
        return nullptr;
    }
    return GameString_Get(
        String_FormatStatic("settings/%s/description", option->name));
}
