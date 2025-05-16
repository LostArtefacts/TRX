#include "game/pathing/vars.h"

#include "game/pathing/const.h"

#if TR_VERSION == 1
    #define M_DEFALT_DROP (-STEP_L)
#else
    #define M_DEFALT_DROP (-STEP_L * 2)
#endif

const LOT_SETUP g_LOT_Default = {
    .step = STEP_L,
    .drop = M_DEFALT_DROP,
    .fly = 0,
    .block_mask = BOX_BLOCKED,
};

const LOT_SETUP g_LOT_Beast = {
    .step = STEP_L,
    .drop = M_DEFALT_DROP,
    .fly = 0,
    .block_mask = BOX_BLOCKABLE,
};

const LOT_SETUP g_LOT_Quadruped = {
    .step = STEP_L,
    .drop = -WALL_L,
    .fly = 0,
    .block_mask = BOX_BLOCKED,
};

const LOT_SETUP g_LOT_Jumper = {
    .step = WALL_L / 2,
    .drop = -WALL_L,
    .fly = 0,
    .block_mask = BOX_BLOCKED,
};

const LOT_SETUP g_LOT_Climber = {
    .step = WALL_L,
    .drop = -WALL_L,
    .fly = 0,
    .block_mask = BOX_BLOCKED,
};

const LOT_SETUP g_LOT_Flyer = {
    .step = WALL_L * 20,
    .drop = -WALL_L * 20,
    .fly = STEP_L / 16,
    .block_mask = BOX_BLOCKED,
};
