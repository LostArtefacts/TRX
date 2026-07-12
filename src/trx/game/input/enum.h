#pragma once

typedef enum {
#define X_INPUT_ROLE(role_name, state_name) role_name,
#include <trx/game/input/roles.def>
    INPUT_ROLE_NUMBER_OF,
#undef X_INPUT_ROLE
} INPUT_ROLE;
