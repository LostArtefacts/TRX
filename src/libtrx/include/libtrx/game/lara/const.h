#pragma once

#include "../const.h"
#include "../rooms/const.h"

#define LARA_MAX_HITPOINTS 1000
#define LARA_MAX_AIR 1800
#define LARA_DIVE_WAIT 10

#define LARA_HEIGHT 762
#define LARA_HEIGHT_SURF 700
#define LARA_HEIGHT_UW 400

#define LARA_RADIUS 100
#define LARA_RADIUS_UW 300
#define LARA_RADIUS_SURF LARA_RADIUS

#define LARA_WADE_DEPTH 384
#define LARA_SWIM_DEPTH 730
#define LARA_CLIMB_WIDTH_LEFT 80
#define LARA_CLIMB_WIDTH_RIGHT 120
#define LARA_CLIMB_HEIGHT (WALL_L / 2) // = 512

#define LARA_TURN_UNDO (2 * DEG_1) // = 364
#define LARA_TURN_RATE ((DEG_1 / 4) + LARA_TURN_UNDO) // = 409

#define LARA_SLOW_TURN ((DEG_1 * 2) + LARA_TURN_UNDO) // = 728
#define LARA_MED_TURN ((DEG_1 * 4) + LARA_TURN_UNDO) // = 1092
#define LARA_FAST_TURN ((DEG_1 * 6) + LARA_TURN_UNDO) // = 1456

#define LARA_JUMP_TURN ((DEG_1 * 1) + LARA_TURN_UNDO) // = 546

#define LARA_LEAN_UNDO DEG_1 // = 182
#define LARA_LEAN_UNDO_SURF (LARA_LEAN_UNDO * 2) // = 364
#define LARA_LEAN_UNDO_UW LARA_LEAN_UNDO_SURF // = 364

#define LARA_LEAN_RATE 273
#define LARA_LEAN_MAX ((10 * DEG_1) + LARA_LEAN_UNDO) // = 2002
#define LARA_LEAN_MAX_UW (LARA_LEAN_MAX * 2) // = 4004

#define LARA_FAST_FALL_SPEED (FAST_FALL_SPEED + 3) // = 131
#define LARA_SWING_FAST_FALL_SPEED (LARA_FAST_FALL_SPEED + 2) // = 133

#define LARA_UW_WALL_DEFLECT (2 * DEG_1) // = 364

#define LARA_DEFLECT_ANGLE (5 * DEG_1) // = 910
#define LARA_HANG_ANGLE (35 * DEG_1) // = 6370
#define LARA_VAULT_ANGLE (30 * DEG_1) // = 5460

// TODO: tidy these
#if TR_VERSION == 1
    #define HEAD_TURN (4 * DEG_1) // = 728
    #define MAX_HEAD_ROTATION (50 * DEG_1) // = 9100
    #define HEAD_TURN_SURF (3 * DEG_1) // = 546
    #define MAX_HEAD_ROTATION_SURF (50 * DEG_1) // = 9100
    #define MAX_HEAD_TILT_SURF (40 * DEG_1) // = 7280
    #define MIN_HEAD_TILT_SURF (-MAX_HEAD_TILT_SURF) // = -7280
#else
    #define HEAD_TURN (2 * DEG_1) // = 364
    #define MAX_HEAD_ROTATION (44 * DEG_1) // = 8008
#endif
#define MIN_HEAD_ROTATION (-MAX_HEAD_ROTATION) // = -8008
#define MAX_HEAD_TILT (22 * DEG_1) // = 4004
#define MIN_HEAD_TILT (-42 * DEG_1) // = -7644

#define NO_BAD_POS (-NO_HEIGHT) // = 32512
#define NO_BAD_NEG (NO_HEIGHT) // = -32512
#define BAD_JUMP_CEILING ((STEP_L * 3) / 4) // = 192
#define STEPUP_HEIGHT ((STEP_L * 3) / 2) // = 384
#define SLOPE_DIF 60

#define DAMAGE_START 140
#define DAMAGE_LENGTH 14

// TODO: move to merged game.c
#define DEATH_WAIT (5 * 2 * LOGIC_FPS) // = 300
#define DEATH_WAIT_INPUT (2 * LOGIC_FPS) // = 60

// TODO: move to merged lara/state.c
#define CAM_WADE_ELEVATION (-22 * DEG_1) // = -4004
#define CAM_REACH_ANGLE (85 * DEG_1) // = 15470
#define CAM_SLIDE_ELEVATION (-45 * DEG_1) // = -8190
#define CAM_BACK_JUMP_ANGLE (135 * DEG_1) // = 24570
#define CAM_PUSH_BLOCK_ANGLE (35 * DEG_1) // = 6370
#define CAM_PUSH_BLOCK_ELEVATION (-25 * DEG_1) // = -4550
#define CAM_PP_READY_ANGLE (75 * DEG_1) // = 13650
#define CAM_PICKUP_ANGLE (-130 * DEG_1) // = -23660
#define CAM_PICKUP_ELEVATION (-15 * DEG_1) // = -2730
#define CAM_PICKUP_DISTANCE WALL_L // = 1024
#define CAM_SWITCH_ON_ANGLE (80 * DEG_1) // = 14560
#define CAM_SWITCH_ON_ELEVATION (-25 * DEG_1) // = -4550
#define CAM_SWITCH_ON_DISTANCE WALL_L // = 1024
#define CAM_SWITCH_ON_SPEED 6
#define CAM_USE_KEY_ANGLE (-CAM_SWITCH_ON_ANGLE) // = -14560
#define CAM_USE_KEY_ELEVATION CAM_SWITCH_ON_ELEVATION // = -4550
#define CAM_USE_KEY_DISTANCE WALL_L // = 1024
#define CAM_SPECIAL_ANGLE (170 * DEG_1) // = 30940
#define CAM_SPECIAL_ELEVATION (-25 * DEG_1) // = -4550
#define CAM_SPECIAL_DISTANCE (2 * WALL_L) // = 2048

#if TR_VERSION == 2
    #define CAM_ZIPLINE_ANGLE (70 * DEG_1) // = 12740
#endif
