#pragma once

// The keys older releases wrote, and what became of them.
//
// Every migration lives here: the config layer above knows only that a file may
// need reading twice, once for the options it has now and once for the names
// they used to go by.
//
// We try to support up to 5 major stable releases back.

#include <trx/core/json.h>

// Applies every migration to a config file that has already been read for the
// options this build has. What it finds it writes straight to g_Config.
void ConfigLegacy_Load(JSON_OBJECT *root_obj);

// Whether a key is one a migration reads, and so one no build writes any more.
// config/file.c carries a key it cannot place over to the new file, and this is
// what it asks to tell a setting belonging to another mod from one a migration
// has already had.
bool ConfigLegacy_IsKey(const char *name);
