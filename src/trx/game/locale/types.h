#pragma once

#include <stdint.h>

#define LOCALE_CHAIN_MAX 4
#define LOCALE_CODE_SIZE 16

// What a language is chosen for. Each follows a setting of its own, so the
// voices can be heard in one language while the text reads in another.
typedef enum {
    // The UI and the game strings.
    LOCALE_ROLE_TEXT,
    // The dub: FMVs, music and speech.
    LOCALE_ROLE_AUDIO,
    // Follows the text until subtitles get a setting of their own.
    LOCALE_ROLE_SUBTITLES,
    LOCALE_ROLE_NUMBER_OF,
} LOCALE_ROLE;

// The names one language gives a file, tried in order.
typedef struct {
    // nullptr for the names every language falls back to.
    char *lang;
    int32_t name_count;
    char **names;
} LOCALE_PATH_ENTRY;

// A file that may go by other names in other languages.
typedef struct {
    int32_t entry_count;
    LOCALE_PATH_ENTRY *entries;
} LOCALE_PATH;

// Language codes to try, most wanted first, in lower case.
typedef struct {
    int32_t count;
    char codes[LOCALE_CHAIN_MAX][LOCALE_CODE_SIZE];
} LOCALE_CHAIN;
