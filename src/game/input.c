#include "game/input.h"

#include "specific/s_input.h"

INPUT_STATE g_Input = { 0 };
INPUT_STATE g_InputDB = { 0 };
INPUT_STATE g_OldInputDB = { 0 };

static INPUT_STATE Input_GetDebounced(INPUT_STATE input);

INPUT_STATE Input_GetDebounced(INPUT_STATE input)
{
    INPUT_STATE result;
    result.any = input.any & ~g_OldInputDB.any;

    // Allow holding down key to move faster
    static int32_t holdBack = -12;
    static int32_t holdForward = -12;

    if (input.forward || !input.back) {
        holdBack = -12;
    } else if (input.back && ++holdBack >= 3) {
        result.back = 1;
        holdBack = 0;
    }

    if (!input.forward || input.back) {
        holdForward = -12;
    } else if (input.forward && ++holdForward >= 3) {
        result.forward = 1;
        holdForward = 0;
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
