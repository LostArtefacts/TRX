#include <trx/core/subsystem.h>

#include <trx/debug.h>

#include <stdint.h>

typedef enum {
    M_PHASE_INIT,
    M_PHASE_LOAD,
    M_PHASE_APPLY_CONFIG,
    M_PHASE_SHUTDOWN,
} M_PHASE;

static SUBSYSTEM *m_First = nullptr;
static SUBSYSTEM *m_Last = nullptr;

static SUBSYSTEM_FUNC M_GetPhaseFunc(
    const SUBSYSTEM *const subsystem, const M_PHASE phase)
{
    switch (phase) {
    case M_PHASE_INIT:
        return subsystem->init;
    case M_PHASE_LOAD:
        return subsystem->load;
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

void Subsystem_LoadAll(void)
{
    M_RunPhase(M_PHASE_LOAD, false);
}

void Subsystem_ApplyConfigAll(void)
{
    M_RunPhase(M_PHASE_APPLY_CONFIG, false);
}

void Subsystem_ShutdownAll(void)
{
    M_RunPhase(M_PHASE_SHUTDOWN, true);
}
