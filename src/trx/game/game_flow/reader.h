#pragma once

#include <trx/game/game_flow/types.h>

// Load the game flow from a file.
void GF_LoadFromFile(const char *path);

// Load the game flow from a file.
// Returns false on I/O or parse/validation failure instead of exiting.
bool GF_TryLoadFromFile(const char *path);

// Load and validate path-backed gameflow references for a mod.
// Returns false if parsing fails or any required resolved path is missing,
// leaving why in error_out where one is given. Caller must free it with
// Memory_FreePointer().
bool GF_ValidateMod(const char *mod_name, const char *path, char **error_out);

// Quick-parse a gameflow file to extract only the mod metadata fields
// ("name", "engine", and "extends"). Returns true on success. Caller must
// free meta->name and meta->extends with Memory_FreePointer().
//
// Where error_out is given, a failed read leaves the reason in it, for a
// caller that has to say why the mod was passed over. Caller must free it
// with Memory_FreePointer().
bool GF_ReadModMeta(const char *path, GF_MOD_META *meta, char **error_out);
