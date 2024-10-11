#include "game/objects/vars.h"

const GAME_OBJECT_PAIR g_KeyItemToReceptacleMap[] = {
    // clang-format off
    { O_KEY_OPTION_1, O_KEY_HOLE_1 },
    { O_KEY_OPTION_2, O_KEY_HOLE_2 },
    { O_KEY_OPTION_3, O_KEY_HOLE_3 },
    { O_KEY_OPTION_4, O_KEY_HOLE_4 },
    { O_PUZZLE_OPTION_1, O_PUZZLE_HOLE_1 },
    { O_PUZZLE_OPTION_2, O_PUZZLE_HOLE_2 },
    { O_PUZZLE_OPTION_3, O_PUZZLE_HOLE_3 },
    { O_PUZZLE_OPTION_4, O_PUZZLE_HOLE_4 },
    #if TR_VERSION == 1
    { O_LEADBAR_OPTION, O_MIDAS_TOUCH },
    #endif
    { NO_OBJECT, NO_OBJECT },
    // clang-format on
};

const GAME_OBJECT_PAIR g_ReceptacleToReceptacleDoneMap[] = {
    // clang-format off
    { O_PUZZLE_HOLE_1, O_PUZZLE_DONE_1 },
    { O_PUZZLE_HOLE_2, O_PUZZLE_DONE_2 },
    { O_PUZZLE_HOLE_3, O_PUZZLE_DONE_3 },
    { O_PUZZLE_HOLE_4, O_PUZZLE_DONE_4 },
    { NO_OBJECT, NO_OBJECT },
    // clang-format on
};
