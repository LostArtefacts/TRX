#pragma once

#include <trx/config/option.h>
#include <trx/core/event_manager.h>

#include <stdint.h>

bool Config_Read(const char *default_path, const char *enforced_path);

// Whether the settings file has been read. Until it has, what the options hold
// is their defaults rather than the player's own choices.
bool Config_IsLoaded(void);
bool Config_Write(void);

// Holds every option to its bounds, then announces which have moved since the
// last call. Announcing is what makes a change take effect: the settings file
// is written from it, and so is everything that has to be told - the mixer, the
// renderer, the window. Call it once a batch of writes is finished rather than
// once per write. Returns whether anything had moved.
//
// Nothing diffs the settings to find that out: every write named the option it
// moved as it went.
bool Config_Update(void);

// Drops the record of what has moved, so the next Config_Update says nothing of
// it. For writes that are not a setting changing: what the settings file held,
// or what a replay recording asked for, is where the settings start rather than
// something that moved.
void Config_DiscardPendingChanges(void);

int32_t Config_SubscribeChanges(EVENT_LISTENER listener, void *user_data);
void Config_UnsubscribeChanges(int32_t listener_id);

// Whether the change reported names the option g_Config keeps at `mirror`,
// which is how a caller reading a setting out of g_Config asks whether that
// setting is one of the ones that moved.
bool Config_Change_HasMirror(const CONFIG_CHANGE *change, const void *mirror);

// The last segment of a dotted option name, which is what the settings file
// keys the option on.
const char *Config_ResolveOptionName(const char *option_name);

// Writes the setting g_Config keeps at `mirror`, which is the only way to write
// one: g_Config itself is const. The value is taken in whatever shape the
// caller's own expression had, and the option says what it means.
bool Config_SetValue(const void *mirror, TRX_VALUE value);

// Holds and releases the setting g_Config keeps at `mirror`. The value is taken
// as it is for Config_SetValue; see Config_Option_PushHold for what a hold is.
bool Config_PushHold(
    const void *mirror, TRX_VALUE value, CONFIG_HOLD_SOURCE source);
bool Config_PopHold(const void *mirror);

// The same three, naming the setting as a member of g_Config so that the value
// is tagged by the C type of the expression itself.
#define CONFIG_SET(member_, value_)                                            \
    Config_SetValue(&(member_), Value_Of(value_))
#define CONFIG_PUSH_HOLD(member_, value_, source_)                             \
    Config_PushHold(&(member_), Value_Of((typeof(member_))(value_)), source_)
#define CONFIG_POP_HOLD(member_) Config_PopHold(&(member_))

// TOGGLE and CYCLE, for a setting: they read the member and write it back, so
// they cannot be used on g_Config as they stand. TOGGLE casts because `!x` is
// an int in C, and an int carrier does not reach a bool setting.
#define CONFIG_TOGGLE(member_) CONFIG_SET(member_, (typeof(member_))!(member_))
#define CONFIG_CYCLE(member_, rate_, number_of_)                               \
    CONFIG_SET(member_, ((member_) + (rate_) + (number_of_)) % (number_of_))
