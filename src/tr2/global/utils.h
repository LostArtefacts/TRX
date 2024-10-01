#pragma once

#include "global/const.h"

#define ROUND_TO_CLICK(V) ((V) & ~(STEP_L - 1))
#define ROUND_TO_SECTOR(V) ((V) & ~(WALL_L - 1))
