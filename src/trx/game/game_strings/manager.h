// Manages autodiscovery and runtime reload of string bundles per language and
// mod/OG fallback.
#pragma once

#include <trx/core/event_manager.h>
#include <trx/core/vector.h>
#include <trx/game/shell/mod.h>

// Point the manager at the files the given mod takes its strings from - the
// common file, the base mod's where it has one, and the mod's own - and load
// the configured language.
void GameStringManager_LoadForMod(const SHELL_MOD *mod);

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
bool GameStringManager_ReloadLanguage(const char *lang);

// Subscribe to be notified when the game strings language is reloaded.
// The listener will receive an EVENT with name "reload_language",
// and .data pointing to the language code (const char *).
int32_t GameStringManager_SubscribeReload(
    EVENT_LISTENER listener, void *user_data);

// Unsubscribe from language reload events.
void GameStringManager_UnsubscribeReload(int32_t listener_id);
