#pragma once

#include "../const.h"
#include "../rooms/const.h"

#if TR_VERSION == 1
    #define LARA_ORIGINAL_ANIM_COUNT 160
#elif TR_VERSION == 2
    #define LARA_ORIGINAL_ANIM_COUNT 218
#endif

#define LARA_MAX_HITPOINTS 1000
#define LARA_MAX_AIR 1800
#define LARA_DIVE_WAIT 10
#define LARA_MAX_SPRINT (4 * LOGIC_FPS)
#define LARA_MAX_EXPOSURE (20 * LOGIC_FPS)

#define LARA_HEIGHT 762
#define LARA_HEIGHT_UW 400
#define LARA_RADIUS 100

#define LARA_TURN_UNDO (2 * DEG_1) // = 364
#define LARA_TURN_RATE ((DEG_1 / 4) + LARA_TURN_UNDO) // = 409
#define LARA_SLOW_TURN ((DEG_1 * 2) + LARA_TURN_UNDO) // = 728
#define LARA_MED_TURN ((DEG_1 * 4) + LARA_TURN_UNDO) // = 1092

#define LARA_LEAN_UNDO DEG_1 // = 182
#define LARA_LEAN_RATE 273
#define LARA_LEAN_MAX ((10 * DEG_1) + LARA_LEAN_UNDO) // = 2002

#define LARA_UW_WALL_DEFLECT (2 * DEG_1) // = 364
#define LARA_DEFLECT_ANGLE (5 * DEG_1) // = 910
#define LARA_HANG_ANGLE (35 * DEG_1) // = 6370

#define NO_BAD_POS (-NO_HEIGHT) // = 32512
#define NO_BAD_NEG (NO_HEIGHT) // = -32512
#define STEPUP_HEIGHT ((STEP_L * 3) / 2) // = 384
#define SLOPE_DIF 60

#define DAMAGE_START 140
#define DAMAGE_LENGTH 14

// TODO: move to merged game.c
#define DEATH_WAIT (5 * 2 * LOGIC_FPS) // = 300
#define DEATH_WAIT_INPUT (2 * LOGIC_FPS) // = 60

#define CAM_WADE_ELEVATION (-22 * DEG_1) // = -4004
