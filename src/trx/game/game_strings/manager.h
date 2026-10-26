// Manages autodiscovery and runtime reload of string bundles per language and
// mod/OG fallback.
#pragma once

#include <trx/core/event_manager.h>
#include <trx/core/result.h>
#include <trx/core/vector.h>
#include <trx/game/game_flow/types.h>
#include <trx/game/shell/mod.h>

// Point the manager at the files the given mod takes its strings from - the
// common file, the base mod's where it has one, and the mod's own - and load
// the configured language.
RESULT GameStringManager_LoadForMod(const SHELL_MOD *mod);

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
RESULT GameStringManager_ReloadLanguage(const char *lang);

// Applies the strings that are in force for a level, or for no level where
// level is nullptr, and notifies the change subscribers.
void GameStringManager_Apply(const GF_LEVEL *level);

// Subscribes to changes of the game strings: a reload of the language, and the
// strings of a level being applied. The listener receives an EVENT with name
// "change".
int32_t GameStringManager_SubscribeChanges(
    EVENT_LISTENER listener, void *user_data);

void GameStringManager_UnsubscribeChanges(int32_t listener_id);
