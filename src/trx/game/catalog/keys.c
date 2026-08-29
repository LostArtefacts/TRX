#include <trx/debug.h>
#include <trx/game/catalog/manager.h>

#include <ctype.h>
#include <string.h>

// Map each context to the prefix used before names in its C spelling.
static const char *const m_ContextPrefixes[CATALOG_CONTEXT_MAX] = {
    [CATALOG_OBJECTS] = "O_",     [CATALOG_MUSIC] = "MX_",
    [CATALOG_SAMPLES] = "SFX_",   [CATALOG_LARA_STATES] = "LS_",
    [CATALOG_LARA_ANIMS] = "LA_", [CATALOG_ITEM_ACTIONS] = "ITEM_ACTION_",
    [CATALOG_WEAPONS] = "LGT_",
};

const char *Catalog_KeyForEnum(
    const CATALOG_CONTEXT context, const char *const enum_name)
{
    const char *const prefix = m_ContextPrefixes[context];
    const char *name = enum_name;
    if (strncmp(name, prefix, strlen(prefix)) == 0) {
        name += strlen(prefix);
    }

    static char key[64];
    ASSERT(strlen(name) < sizeof(key));
    size_t i = 0;
    for (; name[i] != '\0' && i < sizeof(key) - 1; i++) {
        key[i] = (char)tolower((unsigned char)name[i]);
    }
    key[i] = '\0';
    return key;
}

bool Catalog_IsValidKey(const char *const key)
{
    if (key == nullptr || key[0] == '\0') {
        return false;
    }
    for (const char *c = key; *c != '\0'; c++) {
        const bool ok = (*c >= 'a' && *c <= 'z') || (*c >= '0' && *c <= '9')
            || *c == ':' || *c == '_' || *c == '-';
        if (!ok) {
            return false;
        }
    }
    return true;
}
