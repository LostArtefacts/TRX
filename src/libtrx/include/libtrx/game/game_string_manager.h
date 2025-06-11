// Manages autodiscovery and runtime reload of string bundles per language and
// mod/OG fallback.
#pragma once

#include "../vector.h"

#include <stdbool.h>

// Initialize the string bundle manager.
void GameStringManager_Init(void);

// Shutdown the string bundle manager.
void GameStringManager_Shutdown(void);

// Set the OG and active mod base strings file paths.
// og_base_path: path to vanilla (OG) base strings file.
// mod_base_path: path to current mod's base strings file.
// After calling this, call GameStringManager_ReloadLanguage to load the
// strings.
void GameStringManager_SetBaseFiles(
    const char *og_base_path, const char *mod_base_path);

// Returns a vector of char* of available language codes discovered.
// Caller owns the returned VECTOR and must free it via Vector_Free and free
// each string.
VECTOR *GameStringManager_GetAvailableLanguages(void);

// Get the display name for a language code as defined by "language_name"
// in the strings JSON file. Returns nullptr if unavailable.
// The returned pointer is owned by the manager; do not free.
const char *GameStringManager_GetLanguageName(const char *code);

// Reload all game strings for the given language code.
// Clears any previously loaded strings, loads OG fallback and mod overrides.
// lang: language code (e.g. "en", "fr"). Must be one returned by
// GetAvailableLanguages.
void GameStringManager_ReloadLanguage(const char *lang);
