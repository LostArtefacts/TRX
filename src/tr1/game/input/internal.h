#pragma once

#include "game/input/common.h"

void Input_ConflictHelper(
    INPUT_LAYOUT layout,
    bool (*check_conflict_func)(
        INPUT_LAYOUT layout, INPUT_ROLE role1, INPUT_ROLE role2),
    void (*assign_conflict_func)(
        INPUT_LAYOUT layout, INPUT_ROLE role, bool conflict));
