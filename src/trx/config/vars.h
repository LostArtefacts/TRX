#pragma once

#include <trx/config/types.h>

// The settings, for reading.
//
// This is a view of what the option registry holds, kept up to date as each
// setting is written, so reading a setting stays a plain struct access. It is
// not where a setting lives, and writing one here would go around the option
// that owns it - past the hold on it, past the bounds it is held to, and past
// the change that has to be reported. So it reads as const, and a write is a
// compile error rather than a value that quietly fails to stick.
//
// Spelled as a cast rather than a second const object: the object itself is not
// const, since the config module writes it, and this way `&g_Config.member` is
// still the address constant that a static table can be built from.
extern CONFIG g_ConfigStorage;
#define g_Config (*(const CONFIG *)&g_ConfigStorage)
