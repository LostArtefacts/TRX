#include <trx/game/ui/dialogs/settings_handlers.h>

#include <trx/config/registry.h>
#include <trx/core/vector.h>
#include <trx/debug.h>

#include <string.h>

typedef struct {
    const CONFIG_OPTION *option;
    const UI_SETTING_HANDLER *handler;
} M_ENTRY;

// Handlers register themselves as the file is linked, when no option exists
// yet, so they arrive naming a key. The option is what a row asks with, so the
// keys are resolved once and the lookup after that is by address.
static VECTOR *m_Handlers = nullptr;
static VECTOR *m_Resolved = nullptr;
// What the resolved table was built against; the options are dropped and made
// again when the game changes, and every address in it goes with them.
static int32_t m_ResolvedGeneration = -1;
// Bumped as handlers arrive, for a caller keeping a handler of its own.
static int32_t m_Generation = 0;

// What a row with no handler of its own reads.
static const UI_SETTING_HANDLER m_Empty = {};

static void M_Resolve(void)
{
    if (m_Resolved == nullptr) {
        m_Resolved = Vector_Create(sizeof(M_ENTRY));
    }
    Vector_Clear(m_Resolved);
    m_ResolvedGeneration = Config_GetGeneration();
    for (int32_t i = 0; m_Handlers != nullptr && i < m_Handlers->count; i++) {
        const UI_SETTING_HANDLER *const handler =
            *(const UI_SETTING_HANDLER **)Vector_Get(m_Handlers, i);
        // A handler for a setting this game does not have is not an error: one
        // registration serves every game, as one tab list does.
        const CONFIG_OPTION *const option = Config_FindOption(handler->key);
        if (option != nullptr) {
            Vector_Add(m_Resolved, &(M_ENTRY) { option, handler });
        }
    }
}

__attribute__((destructor)) static void M_Shutdown(void)
{
    if (m_Handlers != nullptr) {
        Vector_Free(m_Handlers);
        m_Handlers = nullptr;
    }
    if (m_Resolved != nullptr) {
        Vector_Free(m_Resolved);
        m_Resolved = nullptr;
    }
}

void UI_Settings_AddHandler(const UI_SETTING_HANDLER *const handler)
{
    ASSERT(handler != nullptr);
    ASSERT(handler->key != nullptr);
    if (m_Handlers == nullptr) {
        m_Handlers = Vector_Create(sizeof(const UI_SETTING_HANDLER *));
    }
    for (int32_t i = 0; i < m_Handlers->count; i++) {
        const UI_SETTING_HANDLER *const other =
            *(const UI_SETTING_HANDLER **)Vector_Get(m_Handlers, i);
        // Two handlers for one setting is a mistake in the source, not
        // something to resolve at runtime: one of them would never run, and
        // which one would depend on link order.
        ASSERT(strcmp(other->key, handler->key) != 0);
    }
    Vector_Add(m_Handlers, &handler);
    m_ResolvedGeneration = -1;
    m_Generation++;
}

void UI_Settings_RemoveHandler(const UI_SETTING_HANDLER *const handler)
{
    ASSERT(handler != nullptr);
    for (int32_t i = 0; m_Handlers != nullptr && i < m_Handlers->count; i++) {
        if (*(const UI_SETTING_HANDLER **)Vector_Get(m_Handlers, i)
            != handler) {
            continue;
        }
        Vector_RemoveAt(m_Handlers, i);
        m_ResolvedGeneration = -1;
        m_Generation++;
        return;
    }
}

int32_t UI_Settings_GetHandlerGeneration(void)
{
    return m_Generation;
}

const UI_SETTING_HANDLER *UI_Settings_GetHandler(
    const CONFIG_OPTION *const option)
{
    if (option == nullptr) {
        return &m_Empty;
    }
    if (m_Resolved == nullptr
        || m_ResolvedGeneration != Config_GetGeneration()) {
        M_Resolve();
    }
    for (int32_t i = 0; i < m_Resolved->count; i++) {
        const M_ENTRY *const entry = Vector_Get(m_Resolved, i);
        if (entry->option == option) {
            return entry->handler;
        }
    }
    return &m_Empty;
}
