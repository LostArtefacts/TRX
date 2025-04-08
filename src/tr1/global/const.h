#pragma once

#include <libtrx/game/const.h>
#include <libtrx/utils.h>

#define PHD_ONE 0x10000

#define MAX_REQLINES 18
#define NUM_SLOTS 32
#define MAX_SECRETS 16
#define LARA_MAX_HITPOINTS 1000
#define LARA_MAX_AIR 1800
#define LARA_TURN_UNDO (2 * DEG_1) // = 364
#define LARA_TURN_RATE ((DEG_1 / 4) + LARA_TURN_UNDO) // = 409
#define LARA_TURN_RATE_UW (2 * DEG_1) // = 364
#define LARA_SLOW_TURN ((DEG_1 * 2) + LARA_TURN_UNDO) // = 728
#define LARA_JUMP_TURN ((DEG_1 * 1) + LARA_TURN_UNDO) // = 546
#define LARA_MED_TURN ((DEG_1 * 4) + LARA_TURN_UNDO) // = 1092
#define LARA_FAST_TURN ((DEG_1 * 6) + LARA_TURN_UNDO) // = 1456
#define LARA_LEAN_UNDO DEG_1
#define LARA_LEAN_UNDO_SURF (LARA_LEAN_UNDO * 2) // = 364
#define LARA_LEAN_UNDO_UW LARA_LEAN_UNDO_SURF // = 364
#define LARA_DEF_ADD_EDGE (5 * DEG_1) // = 910
#define LARA_LEAN_RATE 273
#define LARA_LEAN_RATE_SWIM (LARA_LEAN_RATE * 2) // = 546
#define LARA_LEAN_MAX ((10 * DEG_1) + LARA_LEAN_UNDO) // = 2002
#define LARA_LEAN_MAX_UW (LARA_LEAN_MAX * 2)
#define LARA_FASTFALL_SPEED (FAST_FALL_SPEED + 3) // = 131
#define LARA_SWING_FASTFALL_SPEED (LARA_FASTFALL_SPEED + 2) // = 133
#define LARA_RAD 100 // global radius of g_Lara
#define LARA_WADE_DEPTH 384
#define LARA_SWIM_DEPTH 730
#define UW_MAXSPEED 200
#define UW_RADIUS 300
#define UW_HEIGHT 400
#define UW_WALLDEFLECT (2 * DEG_1) // = 364
#define SURF_MAXSPEED 60
#define SURF_RADIUS 100
#define SURF_HEIGHT 700
#define WATER_FRICTION 6
#define DAMAGE_START 140
#define DAMAGE_LENGTH 14
#define NO_CAMERA (-1)
#define NUM_EFFECTS 1000
#define DEATH_WAIT (10 * LOGIC_FPS)
#define DEATH_WAIT_MIN (2 * LOGIC_FPS)
#define MAX_HEAD_ROTATION (50 * DEG_1) // = 9100
#define MAX_HEAD_TILT_LOOK (22 * DEG_1) // = 4004
#define MIN_HEAD_TILT_LOOK (-42 * DEG_1) // = -7644
#define MAX_HEAD_TILT_CAM (85 * DEG_1) // = 15470
#define MIN_HEAD_TILT_CAM (-85 * DEG_1) // = 15470
#define HEAD_TURN (4 * DEG_1) // = 728
#define HEAD_TURN_SURF (3 * DEG_1) // = 546
#define MAX_HEAD_ROTATION_SURF (50 * DEG_1) // = 9100
#define MAX_HEAD_TILT_SURF (40 * DEG_1) // = 7280
#define MIN_HEAD_TILT_SURF (-40 * DEG_1) // = -7280
#define DIVE_WAIT 10
#define STEPUP_HEIGHT ((STEP_L * 3) / 2) // = 384
#define MAX_HEAD_CHANGE (DEG_1 * 5) // = 910
#define MAX_TILT (DEG_1 * 3) // = 546
#define CAM_A_HANG 0
#define CAM_E_HANG (-60 * DEG_1) // = -10920
#define CAM_WADE_ELEVATION (-22 * DEG_1) // = -4004
#define LOOK_SPEED 4
#define COMBAT_SPEED 8
#define CHASE_SPEED 12
#define MOVE_ANG (2 * DEG_1) // = 364
#define COMBAT_DISTANCE (WALL_L * 5 / 2) // = 2560
#define MAX_ELEVATION (85 * DEG_1) // = 15470
#define DEFAULT_RADIUS 10
#define NO_BAD_POS (-NO_HEIGHT)
#define NO_BAD_NEG NO_HEIGHT
#define BAD_JUMP_CEILING ((STEP_L * 3) / 4) // = 192
#define MAX_WIBBLE 2
#define MAX_SHADE 0x300
#define MAX_LIGHTING 0x1FFF
#define NO_VERT_MOVE 0x2000
#define NO_BOX (-1)
#define PASSPORT_FOV 65
#define PICKUPS_FOV 65

#define SLOPE_DIF 60
#define VAULT_ANGLE (30 * DEG_1)
#define HANG_ANGLE (35 * DEG_1)

#define MIN_SQUARE SQUARE(WALL_L / 4) // = 65536

#define DEFAULT_RADIUS 10

#define RINGSWITCH_FRAMES (96 / 2)
#define SELECTING_FRAMES (32 / 2)
#define OPTION_RING_OBJECTS 4
#define TITLE_RING_OBJECTS 5
#define CAMERA_2_RING 598
#define LOW_LIGHT 0x1400 // = 5120
#define HIGH_LIGHT 0x1000 // = 4096

#if _MSC_VER > 0x500
    #define strdup _strdup // fixes error about POSIX function
#endif
