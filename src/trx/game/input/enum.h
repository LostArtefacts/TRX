#pragma once

typedef enum {
#define X_INPUT_ROLE(role_name, state_name) role_name,
#include <trx/game/input/roles.def>
    INPUT_ROLE_NUMBER_OF,
#undef X_INPUT_ROLE
} INPUT_ROLE;

typedef enum {
    // The skip leads to a menu or another screen; every skip role is held.
    INPUT_SKIP_TO_SCREEN,
    // The skip hands over to gameplay; action stays active for Lara.
    INPUT_SKIP_TO_GAME,
    // The scene plays during gameplay; look stays active for the camera.
    INPUT_SKIP_IN_GAME,
    INPUT_SKIP_NUMBER_OF,
} INPUT_SKIP_CONTEXT;
