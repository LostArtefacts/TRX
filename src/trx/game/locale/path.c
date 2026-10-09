#include <trx/game/locale/path.h>

#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/game/locale/chain.h>

static char *M_ResolveEntry(
    const LOCALE_PATH_ENTRY *const entry, const LOCALE_CHAIN *const chain,
    const GAME_DYNAMIC_PATH kind)
{
    for (int32_t i = 0; i < entry->name_count; i++) {
        char *const resolved =
            LocaleChain_Resolve(chain, kind, entry->names[i]);
        if (resolved != nullptr) {
            return resolved;
        }
    }
    return nullptr;
}

char *LocalePath_Resolve(
    const LOCALE_PATH *const path, const LOCALE_CHAIN *const chain,
    const GAME_DYNAMIC_PATH kind)
{
    for (int32_t i = 0; i < chain->count; i++) {
        for (int32_t j = 0; j < path->entry_count; j++) {
            const LOCALE_PATH_ENTRY *const entry = &path->entries[j];
            if (entry->lang == nullptr
                || !String_Equivalent(entry->lang, chain->codes[i])) {
                continue;
            }
            char *const resolved = M_ResolveEntry(entry, chain, kind);
            if (resolved != nullptr) {
                return resolved;
            }
        }
    }

    for (int32_t i = 0; i < path->entry_count; i++) {
        if (path->entries[i].lang != nullptr) {
            continue;
        }
        char *const resolved = M_ResolveEntry(&path->entries[i], chain, kind);
        if (resolved != nullptr) {
            return resolved;
        }
    }

    const LOCALE_CHAIN none = {};
    for (int32_t i = 0; i < path->entry_count; i++) {
        char *const resolved = M_ResolveEntry(&path->entries[i], &none, kind);
        if (resolved != nullptr) {
            return resolved;
        }
    }
    return nullptr;
}

void LocalePath_Free(LOCALE_PATH *const path)
{
    for (int32_t i = 0; i < path->entry_count; i++) {
        LOCALE_PATH_ENTRY *const entry = &path->entries[i];
        for (int32_t j = 0; j < entry->name_count; j++) {
            Memory_FreePointer(&entry->names[j]);
        }
        Memory_FreePointer(&entry->names);
        Memory_FreePointer(&entry->lang);
    }
    Memory_FreePointer(&path->entries);
    path->entry_count = 0;
}
