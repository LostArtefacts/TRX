#include <trx/core/subsystem.h>

#include <trx/debug.h>

#include <stdint.h>
#include <string.h>

// The registration macro names a module after __FILE__, which the compiler
// spells as the path it was given, so the part above the sources is cut off.
#define M_SOURCE_ROOT "src/trx/"

typedef enum {
    M_PHASE_INIT,
    M_PHASE_APPLY_CONFIG,
    M_PHASE_SHUTDOWN,
} M_PHASE;

static SUBSYSTEM *m_First = nullptr;
static SUBSYSTEM *m_Last = nullptr;

static const char *M_TrimName(const char *const name)
{
    const char *const root = strstr(name, M_SOURCE_ROOT);
    return root != nullptr ? root + strlen(M_SOURCE_ROOT) : name;
}

static SUBSYSTEM_FUNC M_GetPhaseFunc(
    const SUBSYSTEM *const subsystem, const M_PHASE phase)
{
    switch (phase) {
    case M_PHASE_INIT:
        return subsystem->init;
    case M_PHASE_APPLY_CONFIG:
        return subsystem->apply_config;
    case M_PHASE_SHUTDOWN:
        return subsystem->shutdown;
    }
    return nullptr;
}

static void M_RunPhase(const M_PHASE phase, const bool reverse)
{
    for (int32_t t = 0; t < SUBSYSTEM_TIER_NUMBER_OF; t++) {
        const int32_t tier = reverse ? SUBSYSTEM_TIER_NUMBER_OF - 1 - t : t;
        for (SUBSYSTEM *subsystem = reverse ? m_Last : m_First;
             subsystem != nullptr;
             subsystem = reverse ? subsystem->prev : subsystem->next) {
            if ((int32_t)subsystem->tier != tier) {
                continue;
            }
            const SUBSYSTEM_FUNC func = M_GetPhaseFunc(subsystem, phase);
            if (func != nullptr) {
                func();
            }
        }
    }
}

void Subsystem_Register(SUBSYSTEM *const subsystem)
{
    ASSERT(subsystem->tier < SUBSYSTEM_TIER_NUMBER_OF);
    subsystem->name = M_TrimName(subsystem->name);
    subsystem->prev = m_Last;
    subsystem->next = nullptr;
    if (m_Last != nullptr) {
        m_Last->next = subsystem;
    } else {
        m_First = subsystem;
    }
    m_Last = subsystem;
}

void Subsystem_InitAll(void)
{
    M_RunPhase(M_PHASE_INIT, false);
}

RESULT Subsystem_LoadAll(void)
{
    for (int32_t tier = 0; tier < SUBSYSTEM_TIER_NUMBER_OF; tier++) {
        for (SUBSYSTEM *subsystem = m_First; subsystem != nullptr;
             subsystem = subsystem->next) {
            if ((int32_t)subsystem->tier != tier
                || subsystem->load == nullptr) {
                continue;
            }
            MUST(subsystem->load(), "%s", subsystem->name);
        }
    }
    return OK;
}

void Subsystem_ApplyConfigAll(void)
{
    M_RunPhase(M_PHASE_APPLY_CONFIG, false);
}

void Subsystem_ShutdownAll(void)
{
    M_RunPhase(M_PHASE_SHUTDOWN, true);
}
