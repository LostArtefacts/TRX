#include <trx/game/objects/general/switch_sequence.h>

#define M_STEP_COMPLETE SWITCH_SEQUENCE_LENGTH
#define M_STEP_RESET_START (M_STEP_COMPLETE + 1)
#define M_STEP_RESET_END (M_STEP_RESET_START + SWITCH_SEQUENCE_LENGTH)

// clang-format off
static const int8_t m_Results
    [SWITCH_SEQUENCE_LENGTH][SWITCH_SEQUENCE_LENGTH][SWITCH_SEQUENCE_LENGTH] = {
    [0][1][2] = 0,
    [0][2][1] = 1,
    [1][0][2] = 2,
    [1][2][0] = 3,
    [2][0][1] = 4,
    [2][1][0] = 5,
};
// clang-format on

static SWITCH_SEQUENCE_STATE m_State = {};

static bool M_IsValidResult(const int32_t result)
{
    return result >= 0 && result < SWITCH_SEQUENCE_RESULT_COUNT;
}

void SwitchSequence_Reset(void)
{
    m_State = (SWITCH_SEQUENCE_STATE) {};
}

const SWITCH_SEQUENCE_STATE *SwitchSequence_GetState(void)
{
    return &m_State;
}

void SwitchSequence_SetState(const SWITCH_SEQUENCE_STATE *const state)
{
    m_State = *state;
    if (m_State.step < 0 || m_State.step >= M_STEP_RESET_END) {
        m_State.step = 0;
    }
}

bool SwitchSequence_IsOpen(void)
{
    return m_State.step < M_STEP_COMPLETE;
}

bool SwitchSequence_IsComplete(void)
{
    return m_State.step == M_STEP_COMPLETE;
}

bool SwitchSequence_IsResetting(void)
{
    return m_State.step >= M_STEP_RESET_START;
}

int32_t SwitchSequence_GetEnteredCount(void)
{
    return m_State.step;
}

void SwitchSequence_Enter(const int32_t switch_ocb)
{
    if (!SwitchSequence_IsOpen()) {
        return;
    }
    m_State.entries[m_State.step++] = switch_ocb;
    if (SwitchSequence_IsComplete()
        && SwitchSequence_IsUsed(SwitchSequence_GetResult())) {
        m_State.step++;
    }
}

void SwitchSequence_SetEntries(const int32_t *const entries)
{
    for (int32_t i = 0; i < SWITCH_SEQUENCE_LENGTH; i++) {
        m_State.entries[i] = entries[i];
    }
}

int32_t SwitchSequence_GetResult(void)
{
    for (int32_t i = 0; i < SWITCH_SEQUENCE_LENGTH; i++) {
        if (m_State.entries[i] < 0
            || m_State.entries[i] >= SWITCH_SEQUENCE_LENGTH) {
            return SWITCH_SEQUENCE_NO_RESULT;
        }
    }
    const int32_t *const e = m_State.entries;
    return m_Results[e[0]][e[1]][e[2]];
}

bool SwitchSequence_IsUsed(const int32_t result)
{
    return M_IsValidResult(result) && (m_State.used_mask & (1u << result)) != 0;
}

void SwitchSequence_SetUsed(const int32_t result)
{
    if (M_IsValidResult(result)) {
        m_State.used_mask |= 1u << result;
    }
}

void SwitchSequence_BeginReset(void)
{
    m_State.step = M_STEP_RESET_START;
}

void SwitchSequence_StepReset(void)
{
    m_State.step++;
    if (m_State.step >= M_STEP_RESET_END) {
        m_State.step = 0;
    }
}
