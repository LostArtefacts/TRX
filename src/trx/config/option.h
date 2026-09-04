#pragma once

#include <trx/core/value.h>

#include <stdint.h>

// A setting.
//
// Every option is registered at runtime, from a description the game's map*.def
// names. There is one kind of option and one way to make one, so nothing
// downstream - the settings file, the console, the settings dialogs - has to
// ask where an option came from.
//
// An option's address is its identity and does not move for its lifetime, so
// holding one stays safe across another being registered.

typedef enum {
    // A float presented as a percentage: stored 0..1, shown and entered as a
    // 0..100 percentage. Only meaningful for TVT_FLOAT.
    CONFIG_OPTION_PERCENT = 1 << 0,
    // The game flow asked for this option to be absent from the settings
    // dialogs. Not the same as being held, which leaves the row in place.
    CONFIG_OPTION_HIDDEN = 1 << 1,
} CONFIG_OPTION_FLAGS;

// What an option accepts, for the numeric types. Held in the units the value is
// stored in, so a scale that runs 0.5 to 2.0 says so.
typedef struct {
    double min;
    double max;
} CONFIG_OPTION_BOUNDS;

// Who is holding an option's value away from the player's own.
typedef enum {
    // enforced_config in the game flow: the player may not change it, the
    // settings dialogs mark it, and the file never carries it.
    CONFIG_HOLD_GAME_FLOW,
    // trx.config.override, for a script dressing a level.
    CONFIG_HOLD_SCRIPT,
    // The demo's own settings, put back when it ends.
    CONFIG_HOLD_DEMO,
    // What photo mode keeps out of the picture, put back when it closes.
    CONFIG_HOLD_PHOTO_MODE,
    // A key the player is holding, put back when they let go.
    CONFIG_HOLD_INPUT,
} CONFIG_HOLD_SOURCE;

#define CONFIG_HOLD_MAX_DEPTH 3

typedef struct {
    TRX_VALUE value;
    CONFIG_HOLD_SOURCE source;
} CONFIG_HOLD;

typedef struct CONFIG_OPTION {
    // Both the dotted path the option answers to and, by its last segment, the
    // key the settings file carries it under.
    const char *name;
    uint32_t flags;

    // The value in force. Its type is the option's type; there is no separate
    // member for it.
    TRX_VALUE value;
    // What the option holds until the player changes it. Always of the same
    // type as `value`.
    TRX_VALUE default_value;

    // Where g_Config keeps a copy, written whenever the value is. Not where the
    // value lives - g_Config is a convenience for reading, and every write
    // comes through the option.
    void *mirror;

    // The EnumMap an option's values are named by, for TVT_ENUM. Null for
    // every other type, TVT_DYNAMIC_ENUM included: its values are keyed on the
    // option itself rather than on a map declared up front.
    const char *enum_map;

    // Null where the option takes anything its storage can hold. The option's
    // own copy: what a declaration pointed at is the caller's, and a script's
    // declaration is read off a Lua table that is gone by the next call.
    const CONFIG_OPTION_BOUNDS *bounds;

    // What the player chose, kept while something holds the option away from
    // it. This is what the settings file carries.
    TRX_VALUE base_value;
    CONFIG_HOLD holds[CONFIG_HOLD_MAX_DEPTH];
    int32_t hold_depth;
} CONFIG_OPTION;

// What a registration says. Everything an option needs to exist; the rest -
// what is holding it, what the file had for it - registration works out.
typedef struct {
    const char *name;
    TRX_VALUE default_value;
    void *mirror;
    const char *enum_map;
    const CONFIG_OPTION_BOUNDS *bounds;
    bool percent;
} CONFIG_OPTION_DESC;

// What moved, reported to whoever asked to hear about it.
//
// The options are named rather than described, so a subscriber that cares about
// one asks whether it is in here. Nothing has to diff the settings to find out
// what happened, because the write said so as it went.
typedef struct {
    // Whether this is a change to remember. A hold pushed over an option, or
    // lifted off it, moves the live value without the player having chosen
    // anything, so the settings file has nothing new to say.
    bool persist;
    const CONFIG_OPTION *const *options;
    int32_t count;
} CONFIG_CHANGE;

// The key naming which enum an option's values come from: for TVT_ENUM the
// EnumMap name, and for TVT_DYNAMIC_ENUM the option's own address, which is the
// token its values are registered under - so the code that seeds those values
// has to key them on this too. Null for a type that is not an enum.
//
// This is what the value parse, format and JSON calls take alongside the type.
const void *Config_Option_GetEnumKey(const CONFIG_OPTION *option);

// Writes the value in force, and the copy g_Config keeps. Every write to a
// setting comes through here: g_Config is const, so there is no way around it.
//
// An option something is holding takes the value into the topmost hold, so the
// player's own value underneath survives the hold being lifted, and the
// settings file still saves what the player chose. Whether a held option may be
// written at all is the caller's to decide - see Config_Option_IsHeld.
//
// The value is taken by copy - a string is duplicated and the option owns it -
// so the caller keeps the value it passed.
void Config_Option_Write(CONFIG_OPTION *option, const TRX_VALUE *value);

// Holds the option to the bounds it declared. A value outside them is brought
// to the nearest one it accepts. An option with no bounds takes anything its
// storage can hold.
//
// A dynamic enum is left alone, whether or not its values are registered yet.
// Its values arrive from whoever owns them - the outfits from the skin module,
// the bar looks from the UI - and a value they do not name is a mod's that is
// not loaded rather than a value to refuse: the player chose it, the settings
// file carries it, and it works again the next time that mod is. What reads
// the setting falls back on its own until then, and cycling the row lands on a
// value that is registered.
void Config_Option_Sanitize(CONFIG_OPTION *option);

// Whether the option still holds what it was declared with.
bool Config_Option_IsAtDefault(const CONFIG_OPTION *option);

// Puts the default back. Refused on a held option unless forced, in which case
// the default replaces what the topmost hold applies.
bool Config_Option_RestoreDefault(CONFIG_OPTION *option, bool force);

// The value the settings file carries: the player's own, from underneath
// anything holding the option.
const TRX_VALUE *Config_Option_GetBaseValue(const CONFIG_OPTION *option);

// Holds the option at `value` until it is released, remembering what it held.
// False once the stack is full.
bool Config_Option_PushHold(
    CONFIG_OPTION *option, const TRX_VALUE *value, CONFIG_HOLD_SOURCE source);

// Lifts one hold off, putting back what was underneath. False if nothing was
// holding the option.
bool Config_Option_PopHold(CONFIG_OPTION *option);

// Drops every hold without putting anything back, for an option about to be
// given its value from scratch.
void Config_Option_ReleaseHolds(CONFIG_OPTION *option);

// Whether anything is holding the option away from the player's own value. A
// held option is not the player's to change, whoever is holding it.
bool Config_Option_IsHeld(const CONFIG_OPTION *option);

// Whether the game flow is what is holding it. A script may hold a setting over
// another script's hold, but not over the one a level asked for.
bool Config_Option_IsEnforced(const CONFIG_OPTION *option);

// Whether the game flow asked for the option to be absent from the dialogs.
bool Config_Option_IsHidden(const CONFIG_OPTION *option);

// Whether the option can take this value as it is spelled. A setting a game
// declares takes only the values that game offers, so a caller naming one from
// elsewhere is asking for something this option has no answer to.
bool Config_Option_AcceptsString(
    const CONFIG_OPTION *option, const char *value);

// Updates the value from a string. Refused on a held option unless forced, in
// which case the value replaces what the topmost hold applies - so releasing
// the hold still restores the player's own.
bool Config_Option_SetFromString(
    CONFIG_OPTION *option, const char *new_value, bool force);

// Holds the option at a value parsed from a string. Fails on a value that will
// not parse, once the stack is full, and on an option the game flow enforces -
// which is a level's own decision, not a script's to talk over.
bool Config_Option_PushHoldFromString(
    CONFIG_OPTION *option, const char *value, CONFIG_HOLD_SOURCE source);

// The current value as a static string. The string must not be freed and is
// short lived.
const char *Config_Option_GetValueAsString(
    const CONFIG_OPTION *option, bool human_readable);

// Puts a value string through the same parser and formatter a live value goes
// through. Returns an allocated string the caller owns.
char *Config_Option_NormalizeValueString(
    const CONFIG_OPTION *option, const char *value, bool human_readable);

// The translated title and description a settings dialog shows.
const char *Config_Option_GetTitle(const CONFIG_OPTION *option);
const char *Config_Option_GetDescription(const CONFIG_OPTION *option);
