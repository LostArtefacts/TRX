#pragma once

#include <trx/config/option.h>
#include <trx/core/json.h>

// The settings document and the game flow's, and what they have to say about an
// option.
//
// Both are kept after the read, so an option registered later still finds what
// they had to say about it. They belong to the read rather than to the process:
// a game being swapped for another ends them.

bool ConfigFile_Read(const char *default_path, const char *enforced_path);

// Lets go of both documents, for a set of options being dropped: what one
// game's file said is nothing to the next game's settings.
void ConfigFile_Forget(void);

// The settings root as it was read, for the parts of the file that are not
// options - sections, migrations.
JSON_OBJECT *ConfigFile_GetRoot(void);

// Gives one option the value the settings file carried for it. An option the
// file says nothing about goes back to its default, so a read says what every
// option holds rather than only the ones it mentions.
void ConfigFile_ApplyFileValueTo(CONFIG_OPTION *option);

// Applies what the game flow hides or enforces. Kept apart from the value pass
// because a write on a held option lands on the hold: anything that still has
// the player's own value to set has to have set it by now.
void ConfigFile_ApplyEnforcedTo(CONFIG_OPTION *option);

bool ConfigFile_Write(const char *default_path, void (*action)(JSON_OBJECT *));

// Writes every option under its own key.
void ConfigFile_DumpOptions(JSON_OBJECT *root_obj);
