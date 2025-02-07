#pragma once

#include <libtrx/game/const.h>
#include <libtrx/utils.h>

#define PHD_ONE 0x10000

#define MAX_REQLINES 18
#define MAX_SAMPLES 256
#define NUM_SLOTS 32
#define MAX_FRAMES 10
#define MAX_CD_TRACKS 64
#define MAX_FLIP_MAPS 10
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
#define LARA_HEIGHT 762 // global height of g_Lara - less than 3/4 block
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
#define NO_ITEM (-1)
#define NO_CAMERA (-1)
#define PELLET_SCATTER (20 * DEG_1)
#define NUM_SG_SHELLS 2
#define GUN_AMMO_CLIP 16
#define MAGNUM_AMMO_CLIP 25
#define UZI_AMMO_CLIP 50
#define SHOTGUN_AMMO_CLIP 6
#define GUN_AMMO_QTY (GUN_AMMO_CLIP * 2)
#define MAGNUM_AMMO_QTY (MAGNUM_AMMO_CLIP * 2)
#define UZI_AMMO_QTY (UZI_AMMO_CLIP * 2)
#define SHOTGUN_AMMO_QTY (SHOTGUN_AMMO_CLIP * NUM_SG_SHELLS)
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
#define ROUND_TO_CLICK(V) ((V) & ~(STEP_L - 1))
#define ROUND_TO_SECTOR(V) ((V) & ~(WALL_L - 1))
#define STEPUP_HEIGHT ((STEP_L * 3) / 2) // = 384
#define FRONT_ARC DEG_90
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
#define UNIT_SHADOW 256
#define NO_BAD_POS (-NO_HEIGHT)
#define NO_BAD_NEG NO_HEIGHT
#define BAD_JUMP_CEILING ((STEP_L * 3) / 4) // = 192
#define MAX_WIBBLE 2
#define MAX_SHADE 0x300
#define MAX_LIGHTING 0x1FFF
#define NO_VERT_MOVE 0x2000
#define MAX_EXPANSION 5
#define NO_BOX (-1)
#define BOX_NUMBER 0x7FFF
#define BLOCKABLE 0x8000
#define BLOCKED 0x4000
#define OVERLAP_INDEX 0x3FFF
#define SEARCH_NUMBER 0x7FFF
#define BLOCKED_SEARCH 0x8000
#define CLIP_LEFT 1
#define CLIP_RIGHT 2
#define CLIP_TOP 4
#define CLIP_BOTTOM 8
#define ALL_CLIP (CLIP_LEFT | CLIP_RIGHT | CLIP_TOP | CLIP_BOTTOM)
#define SECONDARY_CLIP 16
#define STALK_DIST (WALL_L * 3) // = 3072
#define TARGET_DIST (WALL_L * 4) // = 4096
#define ESCAPE_DIST (WALL_L * 5) // = 5120
#define ATTACK_RANGE SQUARE(WALL_L * 3) // = 9437184
#define ESCAPE_CHANCE 2048
#define RECOVER_CHANCE 256
#define PASSPORT_FOV 65
#define PICKUPS_FOV 65

#define BIFF (WALL_L >> 1)

#define SLOPE_DIF 60
#define VAULT_ANGLE (30 * DEG_1)
#define HANG_ANGLE (35 * DEG_1)
#define END_BIT 0x8000

#define MIN_SQUARE SQUARE(WALL_L / 4) // = 65536
#define GROUND_SHIFT (STEP_L)

#define DEFAULT_RADIUS 10

#define RINGSWITCH_FRAMES (96 / 2)
#define SELECTING_FRAMES (32 / 2)
#define OPTION_RING_OBJECTS 4
#define TITLE_RING_OBJECTS 5
#define RING_RADIUS 688
#define CAMERA_2_RING 598
#define LOW_LIGHT 0x1400 // = 5120
#define HIGH_LIGHT 0x1000 // = 4096

#if _MSC_VER > 0x500
    #define strdup _strdup // fixes error about POSIX function
#endif
