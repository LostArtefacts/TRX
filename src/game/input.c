#include "game/input.h"

#include "specific/s_input.h"

#define DELAY_FRAMES 12
#define HOLD_FRAMES 3

INPUT_STATE g_Input = { 0 };
INPUT_STATE g_InputDB = { 0 };
INPUT_STATE g_OldInputDB = { 0 };

static int32_t m_HoldBack = 0;
static int32_t m_HoldForward = 0;

static INPUT_STATE Input_GetDebounced(INPUT_STATE input);

INPUT_STATE Input_GetDebounced(INPUT_STATE input)
{
    INPUT_STATE result;
    result.any = input.any & ~g_OldInputDB.any;

    // Allow holding down key to move faster
    if (input.forward || !input.back) {
        m_HoldBack = 0;
    } else if (input.back && ++m_HoldBack >= DELAY_FRAMES + HOLD_FRAMES) {
        result.back = 1;
        m_HoldBack = DELAY_FRAMES;
    }

    if (!input.forward || input.back) {
        m_HoldForward = 0;
    } else if (input.forward && ++m_HoldForward >= DELAY_FRAMES + HOLD_FRAMES) {
        result.forward = 1;
        m_HoldForward = DELAY_FRAMES;
    }

    g_OldInputDB = input;
    return result;
}

void Input_Init()
{
    S_Input_Init();
}

void Input_Update()
{
    g_Input = S_Input_GetCurrentState();
    g_InputDB = Input_GetDebounced(g_Input);
}
