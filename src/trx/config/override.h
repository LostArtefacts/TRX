#pragma once

#include <trx/config/option.h>

#include <stdbool.h>

// A stack of runtime overrides, one per option.
//
// The point is that the player's own value survives underneath. Pushing records
// it and applies the new one; popping puts it back. Overrides stack, so a demo
// can override what a level already overrode, and each pop lifts one layer.
//
// Overrides are runtime-only: they never reach the settings file. What gets
// saved is the value underneath, which is what ConfigOverride_GetBaseValuePtr
// is for.

// Applies `value` to the option, remembering whatever is currently there.
bool ConfigOverride_Push(const CONFIG_OPTION *option, const void *value);

// Lifts one layer off. False if the option was not overridden.
bool ConfigOverride_Pop(const CONFIG_OPTION *option);

bool ConfigOverride_IsOverridden(const CONFIG_OPTION *option);

// The value the option would have had if nobody had overridden it - what the
// settings file must be written from. Returns the option's own target when
// there is no override.
const void *ConfigOverride_GetBaseValuePtr(const CONFIG_OPTION *option);

// Drops every override without restoring it. For a config being reloaded from
// scratch, where the values underneath are about to be replaced anyway.
void ConfigOverride_Clear(void);

void ConfigOverride_Shutdown(void);
