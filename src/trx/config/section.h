#pragma once

// The parts of the settings file that are not options.
//
// An option is the config module's own: it knows the option's name, its type
// and where its value lives, so it can read and write the key itself. The rest
// of the file is not like that. Input layouts are per-backend, per-layout,
// per-slot arrays; the gym track records are stat blobs. Whoever owns that data
// is the only one who knows how it is spelled.
//
// So the config module owns the document and drives the passes, and a module
// names the key it owns and says how to read and write it. Nothing here has to
// know what is inside.

#include <trx/core/json.h>
#include <trx/core/utils.h>

typedef struct {
    // The top-level key in the settings file. Also what tells the writer that
    // this key is spoken for, so a key belonging to nothing is carried over
    // rather than dropped - see ConfigFile_Write.
    const char *key;
    // Called with the key's object, or nullptr where the file had nothing under
    // it. A section that is asked for and finds nothing keeps what it has.
    void (*load)(const JSON_OBJECT *obj);
    // Called with a fresh object to fill. Leaving it empty is allowed; the
    // writer drops a section that wrote nothing rather than leaving a bare key.
    void (*save)(JSON_OBJECT *obj);
} CONFIG_SECTION;

void Config_Section_Add(const CONFIG_SECTION *section);

// The sections, terminated by a null entry. Constructors run in an order the
// linker chooses, so this one is not meaningful and nothing may depend on it:
// each section owns its own key and reads none of the others.
const CONFIG_SECTION *const *Config_Section_GetAll(void);

// Whether any top-level key belongs to a section.
bool Config_Section_OwnsKey(const char *key);

// Says a section's data moved, so the settings file follows.
//
// An option reports itself as it is written. A section cannot: the config
// module never sees inside one, so it has no way to tell that a binding was
// rebound. The module that did it says so here.
void Config_SectionChanged(void);

// Whether a section has reported a change since this was last called, clearing
// the report as it goes.
bool Config_Section_TakeChanged(void);

// Registers a section as the file is linked, so nothing has to drive a list and
// the order modules initialize in does not matter.
#define REGISTER_CONFIG_SECTION(...)                                           \
    __attribute__((__constructor__)) static void CONCAT(                       \
        M_RegisterConfigSection_, __LINE__)(void)                              \
    {                                                                          \
        static const CONFIG_SECTION m_Section = { __VA_ARGS__ };               \
        Config_Section_Add(&m_Section);                                        \
    }
