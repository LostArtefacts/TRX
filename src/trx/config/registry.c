#include <trx/config/registry.h>

#include <trx/config/common.h>
#include <trx/config/file.h>
#include <trx/config/priv.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/subsystem.h>
#include <trx/core/vector.h>
#include <trx/debug.h>

#include <string.h>

// Options are held by pointer rather than by value: an option's address is its
// identity, and the vector behind this moves when it grows.
static VECTOR *m_Options = nullptr;
// The null-terminated view handed out by Config_GetOptions, rebuilt whenever
// the set of options changes.
static VECTOR *m_View = nullptr;
static int32_t m_Generation = 0;

static CONFIG_OPTION *M_Get(const int32_t idx)
{
    return *(CONFIG_OPTION **)Vector_Get(m_Options, idx);
}

static void M_RebuildView(void)
{
    m_Generation++;
    if (m_View == nullptr) {
        m_View = Vector_Create(sizeof(CONFIG_OPTION *));
    }
    Vector_Clear(m_View);
    for (int32_t i = 0; m_Options != nullptr && i < m_Options->count; i++) {
        Vector_Add(m_View, Vector_Get(m_Options, i));
    }
    Vector_Add(m_View, &(CONFIG_OPTION *) { nullptr });
}

// Whether an option already answers to this name. It is not enough for the
// whole name to be free: the settings file keys an option by the last segment
// of its name and so does a lookup by path, so a second option taking
// `visuals.fov`'s last segment would read and write that option's value out of
// the file, and stand between it and the console.
static bool M_IsNameTaken(const char *const name)
{
    const char *const leaf = Config_ResolveOptionName(name);
    for (int32_t i = 0; m_Options != nullptr && i < m_Options->count; i++) {
        const CONFIG_OPTION *const other = M_Get(i);
        if (strcmp(other->name, name) == 0
            || strcmp(Config_ResolveOptionName(other->name), leaf) == 0) {
            return true;
        }
    }
    return false;
}

static void M_Shutdown(void)
{
    Config_DropAllOptions();
    if (m_Options != nullptr) {
        Vector_Free(m_Options);
        m_Options = nullptr;
    }
    if (m_View != nullptr) {
        Vector_Free(m_View);
        m_View = nullptr;
    }
}

void Config_DropAllOptions(void)
{
    // A report names the options that moved by address, so it cannot outlive
    // them. Nothing has read it if it is still here, and a fresh set of options
    // has nothing to say about what the last set did.
    Config_DiscardPendingChanges();
    // The documents answered for these options, so they go with them.
    ConfigFile_Forget();
    for (int32_t i = 0; m_Options != nullptr && i < m_Options->count; i++) {
        CONFIG_OPTION *const option = M_Get(i);
        Config_Option_Free(option);
        Memory_Free(option);
    }
    if (m_Options != nullptr) {
        Vector_Clear(m_Options);
    }
    M_RebuildView();
}

CONFIG_OPTION *Config_Register(const CONFIG_OPTION_DESC *const desc)
{
    ASSERT(desc != nullptr);
    ASSERT(desc->name != nullptr);

    if (M_IsNameTaken(desc->name)) {
        LOG_WARNING("Config option '%s' already exists", desc->name);
        return nullptr;
    }

    if (m_Options == nullptr) {
        m_Options = Vector_Create(sizeof(CONFIG_OPTION *));
    }

    CONFIG_OPTION *const option = Memory_Alloc(sizeof(CONFIG_OPTION));
    Config_Option_Init(option, desc);
    Vector_Add(m_Options, &option);
    M_RebuildView();

    // What the settings file and the game flow had to say about it. Both are
    // kept after the read, so an option that arrives later still finds them.
    ConfigFile_ApplyFileValueTo(option);
    ConfigFile_ApplyEnforcedTo(option);
    Config_Option_Sanitize(option);
    return option;
}

CONFIG_OPTION *const *Config_GetOptions(void)
{
    if (m_View == nullptr) {
        M_RebuildView();
    }
    return Vector_GetData(m_View);
}

CONFIG_OPTION *Config_FindOption(const char *const path)
{
    if (path == nullptr || m_Options == nullptr) {
        return nullptr;
    }
    for (int32_t i = 0; i < m_Options->count; i++) {
        CONFIG_OPTION *const option = M_Get(i);
        if (strcmp(option->name, path) == 0
            || strcmp(Config_ResolveOptionName(option->name), path) == 0) {
            return option;
        }
    }
    return nullptr;
}

CONFIG_OPTION *Config_FindOptionByMirror(const void *const mirror)
{
    if (mirror == nullptr || m_Options == nullptr) {
        return nullptr;
    }
    for (int32_t i = 0; i < m_Options->count; i++) {
        CONFIG_OPTION *const option = M_Get(i);
        if (option->mirror == mirror) {
            return option;
        }
    }
    return nullptr;
}

int32_t Config_GetGeneration(void)
{
    return m_Generation;
}

void Config_ClearHolds(void)
{
    for (int32_t i = 0; m_Options != nullptr && i < m_Options->count; i++) {
        Config_Option_ReleaseHolds(M_Get(i));
    }
}

REGISTER_SUBSYSTEM(.shutdown = M_Shutdown)
