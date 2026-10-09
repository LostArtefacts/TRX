#pragma once

#include <trx/core/event_manager.h>
#include <trx/core/vector.h>
#include <trx/game/locale/chain.h>
#include <trx/game/locale/path.h>
#include <trx/game/paths.h>

// Which language the game speaks for each role, and where it looks for what
// that language needs. Everything that depends on a language asks here rather
// than reading the settings.

#define LOCALE_AUDIO_AUTO "auto"
#define LOCALE_AUDIO_RETAIL "retail"

// The languages a role asks for. Text follows the language setting. Audio
// follows the voice language setting, where Auto means the text language and
// Retail the plain retail files, which is an empty chain.
LOCALE_CHAIN Locale_GetChain(LOCALE_ROLE role);

// The language a role asks for, as the settings name it, or nullptr where it
// asks for none.
const char *Locale_GetCode(LOCALE_ROLE role);

// The languages a role can be set to, as a vector of owning strings. Text
// offers the languages with strings files. Audio offers the dubs installed,
// from the language folders beside the retail files and from the game flow's
// own names for its FMVs. The caller frees each string and the vector.
VECTOR *Locale_GetAvailable(LOCALE_ROLE role);

// The name a language gives itself in its strings file, or nullptr.
const char *Locale_GetLanguageName(const char *code);

// LocaleChain_Find along the chain of the role.
char *Locale_Find(
    LOCALE_ROLE role, const char *path, LOCALE_FIND_FUNC func, void *user_data);

// Resolves rel as GamePath_PeekResolve does, along the chain of the role. The
// caller frees the result.
char *Locale_Resolve(LOCALE_ROLE role, GAME_DYNAMIC_PATH path, const char *rel);

// LocalePath_Resolve along the chain of the role. The caller frees the result.
char *Locale_ResolvePath(
    LOCALE_ROLE role, GAME_DYNAMIC_PATH kind, const LOCALE_PATH *path);

// The listener is told when the language of a role changes. The event's data
// is the LOCALE_ROLE, cast to a pointer.
int32_t Locale_SubscribeChanges(EVENT_LISTENER listener, void *user_data);
void Locale_UnsubscribeChanges(int32_t listener_id);

// A player who has never launched the game has never said what language they
// want, so the one their system prefers stands in for the answer, from among
// the text languages available. preferred holds the system's codes as char *,
// most wanted first.
void Locale_ApplySystemLanguage(const VECTOR *preferred);
