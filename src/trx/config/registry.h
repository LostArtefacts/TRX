#pragma once

// The options that exist, and the only way to make one.

#include <trx/config/option.h>

// Adds an option and gives it its starting value: its default, then whatever
// the settings file held for it, then whatever the game flow enforces or hides,
// then held to its own bounds.
//
// The settings file is kept after the read, so this is the only thing that has
// to consult it, and an option that arrives after the read still finds what the
// file had to say about it. One registered before the read holds its default
// until the read comes.
//
// Returns nullptr if an option already answers to the name - either the whole
// of it, or the last segment alone, which is what a lookup by path accepts and
// what the settings file keys an option on. A script declaring one is told so;
// a game's own map*.def saying it twice is a mistake and asserts.
CONFIG_OPTION *Config_Register(const CONFIG_OPTION_DESC *desc);

// Registers the options this game's map*.def names, each holding its default,
// and drops whatever was registered before: a different game is a different set
// of settings.
void Config_RegisterBuiltInOptions(void);

// Every option, terminated by a null entry. An option's address does not move,
// so one taken from here stays good after another is registered.
CONFIG_OPTION *const *Config_GetOptions(void);

// The option a dotted path names, by the whole path or by its last segment.
CONFIG_OPTION *Config_FindOption(const char *path);

// The option whose value g_Config keeps at `mirror`, which is how a caller
// holding a pointer into g_Config - a settings dialog row, a hard-coded
// setting - names the option that owns it.
CONFIG_OPTION *Config_FindOptionByMirror(const void *mirror);

// Drops every hold and the player's value underneath it, leaving whatever the
// topmost hold last applied as the value in force. Nothing is put back: this is
// for a config about to be read, where what the file says is what the option
// ends up holding.
//
// An option the file turns out to say nothing about goes back to its default,
// so nothing a hold applied outlives the read. The config is read once per
// game rather than once per process: switching mod restarts the game in place.
void Config_ClearHolds(void);
