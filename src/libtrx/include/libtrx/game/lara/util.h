#pragma once

#include "./col.h"
#include "./state.h"

#define REGISTER_LARA_COL(state, handle_func)                                  \
    __attribute__((constructor)) static void M_RegisterColHandler##state(void) \
    {                                                                          \
        Lara_Col_Register(state, handle_func);                                 \
    }

#define REGISTER_LARA_STATE(state, handle_func)                                \
    __attribute__((constructor)) static void M_RegisterStateHandler##state(    \
        void)                                                                  \
    {                                                                          \
        Lara_State_Register(state, handle_func);                               \
    }
