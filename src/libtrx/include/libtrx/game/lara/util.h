#pragma once

#include "./col.h"

#define REGISTER_LARA_COL(state, handle_func)                                  \
    __attribute__((constructor)) static void M_RegisterColHandler##state(void) \
    {                                                                          \
        Lara_Col_Register(state, handle_func);                                 \
    }
