#include <trx/config/section.h>

#include <trx/debug.h>

#include <string.h>

// Sections are named one per module that owns a part of the file, so there are
// a handful and the count is known at link time. A fixed table keeps this
// reachable from a constructor without allocating.
#define M_MAX_SECTIONS 16

static const CONFIG_SECTION *m_Sections[M_MAX_SECTIONS + 1] = {};
static int32_t m_SectionCount = 0;
static bool m_Changed = false;

void Config_Section_Add(const CONFIG_SECTION *const section)
{
    ASSERT(section != nullptr);
    ASSERT(section->key != nullptr);
    ASSERT(m_SectionCount < M_MAX_SECTIONS);
    m_Sections[m_SectionCount++] = section;
}

const CONFIG_SECTION *const *Config_Section_GetAll(void)
{
    return m_Sections;
}

bool Config_Section_OwnsKey(const char *const key)
{
    if (key == nullptr) {
        return false;
    }
    for (int32_t i = 0; i < m_SectionCount; i++) {
        if (strcmp(m_Sections[i]->key, key) == 0) {
            return true;
        }
    }
    return false;
}

void Config_SectionChanged(void)
{
    m_Changed = true;
}

bool Config_Section_TakeChanged(void)
{
    const bool result = m_Changed;
    m_Changed = false;
    return result;
}
