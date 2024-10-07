#include "game/input/internal.h"

void Input_ConflictHelper(
    const INPUT_LAYOUT layout,
    bool (*check_conflict_func)(
        INPUT_LAYOUT layout, INPUT_ROLE role1, INPUT_ROLE role2),
    void (*assign_conflict_func)(
        INPUT_LAYOUT layout, INPUT_ROLE role, bool conflict))
{
    for (INPUT_ROLE role1 = 0; role1 < INPUT_ROLE_NUMBER_OF; role1++) {
        if (!Input_IsRoleRebindable(role1)) {
            continue;
        }

        bool conflict = false;
        for (INPUT_ROLE role2 = 0; role2 < INPUT_ROLE_NUMBER_OF; role2++) {
            if (!Input_IsRoleRebindable(role2)) {
                continue;
            }

            if (role1 == role2) {
                continue;
            }

            if (check_conflict_func(layout, role1, role2)) {
                conflict = true;
            }
        }

        assign_conflict_func(layout, role1, conflict);
    }
}
