#pragma once

#include "global/const.h"

#include <stdbool.h>
#include <stdint.h>

typedef int16_t PHD_ANGLE;

typedef enum SAMPLE_FLAG {
    SAMPLE_FLAG_NO_PAN = 1 << 12,
    SAMPLE_FLAG_PITCH_WIBBLE = 1 << 13,
    SAMPLE_FLAG_VOLUME_WIBBLE = 1 << 14,
} SAMPLE_FLAG;

typedef enum CAMERA_TYPE {
    CAM_CHASE = 0,
    CAM_FIXED = 1,
    CAM_LOOK = 2,
    CAM_COMBAT = 3,
    CAM_CINEMATIC = 4,
    CAM_HEAVY = 5,
} CAMERA_TYPE;

typedef enum GAME_OBJECT_ID {
    O_INVALID = -1,
    O_LARA = 0,
    O_PISTOLS = 1,
    O_SHOTGUN = 2,
    O_MAGNUM = 3,
    O_UZI = 4,
    O_LARA_EXTRA = 5,
    O_BACON_LARA = 6,
    O_WOLF = 7,
    O_BEAR = 8,
    O_BAT = 9,
    O_CROCODILE = 10,
    O_ALLIGATOR = 11,
    O_LION = 12,
    O_LIONESS = 13,
    O_PUMA = 14,
    O_APE = 15,
    O_RAT = 16,
    O_VOLE = 17,
    O_TREX = 18,
    O_RAPTOR = 19,
    O_WARRIOR1 = 20, // flying mutant
    O_WARRIOR2 = 21,
    O_WARRIOR3 = 22,
    O_CENTAUR = 23,
    O_MUMMY = 24,
    O_DINO_WARRIOR = 25,
    O_FISH = 26,
    O_LARSON = 27,
    O_PIERRE = 28,
    O_SKATEBOARD = 29,
    O_SKATEKID = 30,
    O_COWBOY = 31,
    O_BALDY = 32,
    O_NATLA = 33,
    O_TORSO = 34, // a.k.a. Abortion, Adam or Evil Natla
    O_FALLING_BLOCK = 35,
    O_PENDULUM = 36,
    O_SPIKES = 37,
    O_ROLLING_BALL = 38,
    O_DARTS = 39,
    O_DART_EMITTER = 40,
    O_DRAW_BRIDGE = 41,
    O_TEETH_TRAP = 42,
    O_DAMOCLES_SWORD = 43,
    O_THORS_HANDLE = 44,
    O_THORS_HEAD = 45,
    O_LIGHTNING_EMITTER = 46,
    O_MOVING_BAR = 47,
    O_MOVABLE_BLOCK = 48,
    O_MOVABLE_BLOCK2 = 49,
    O_MOVABLE_BLOCK3 = 50,
    O_MOVABLE_BLOCK4 = 51,
    O_ROLLING_BLOCK = 52,
    O_FALLING_CEILING1 = 53,
    O_FALLING_CEILING2 = 54,
    O_SWITCH_TYPE1 = 55,
    O_SWITCH_TYPE2 = 56,
    O_DOOR_TYPE1 = 57,
    O_DOOR_TYPE2 = 58,
    O_DOOR_TYPE3 = 59,
    O_DOOR_TYPE4 = 60,
    O_DOOR_TYPE5 = 61,
    O_DOOR_TYPE6 = 62,
    O_DOOR_TYPE7 = 63,
    O_DOOR_TYPE8 = 64,
    O_TRAPDOOR = 65,
    O_TRAPDOOR2 = 66,
    O_BIGTRAPDOOR = 67,
    O_BRIDGE_FLAT = 68,
    O_BRIDGE_TILT1 = 69,
    O_BRIDGE_TILT2 = 70,
    O_PASSPORT_OPTION = 71,
    O_MAP_OPTION = 72,
    O_PHOTO_OPTION = 73,
    O_COG_1 = 74,
    O_COG_2 = 75,
    O_COG_3 = 76,
    O_PLAYER_1 = 77,
    O_PLAYER_2 = 78,
    O_PLAYER_3 = 79,
    O_PLAYER_4 = 80,
    O_PASSPORT_CLOSED = 81,
    O_MAP_CLOSED = 82,
    O_SAVEGAME_ITEM = 83,
    O_GUN_ITEM = 84,
    O_SHOTGUN_ITEM = 85,
    O_MAGNUM_ITEM = 86,
    O_UZI_ITEM = 87,
    O_GUN_AMMO_ITEM = 88,
    O_SG_AMMO_ITEM = 89,
    O_MAG_AMMO_ITEM = 90,
    O_UZI_AMMO_ITEM = 91,
    O_EXPLOSIVE_ITEM = 92,
    O_MEDI_ITEM = 93,
    O_BIGMEDI_ITEM = 94,
    O_DETAIL_OPTION = 95,
    O_SOUND_OPTION = 96,
    O_CONTROL_OPTION = 97,
    O_GAMMA_OPTION = 98,
    O_GUN_OPTION = 99,
    O_SHOTGUN_OPTION = 100,
    O_MAGNUM_OPTION = 101,
    O_UZI_OPTION = 102,
    O_GUN_AMMO_OPTION = 103,
    O_SG_AMMO_OPTION = 104,
    O_MAG_AMMO_OPTION = 105,
    O_UZI_AMMO_OPTION = 106,
    O_EXPLOSIVE_OPTION = 107,
    O_MEDI_OPTION = 108,
    O_BIGMEDI_OPTION = 109,
    O_PUZZLE_ITEM1 = 110,
    O_PUZZLE_ITEM2 = 111,
    O_PUZZLE_ITEM3 = 112,
    O_PUZZLE_ITEM4 = 113,
    O_PUZZLE_OPTION1 = 114,
    O_PUZZLE_OPTION2 = 115,
    O_PUZZLE_OPTION3 = 116,
    O_PUZZLE_OPTION4 = 117,
    O_PUZZLE_HOLE1 = 118,
    O_PUZZLE_HOLE2 = 119,
    O_PUZZLE_HOLE3 = 120,
    O_PUZZLE_HOLE4 = 121,
    O_PUZZLE_DONE1 = 122,
    O_PUZZLE_DONE2 = 123,
    O_PUZZLE_DONE3 = 124,
    O_PUZZLE_DONE4 = 125,
    O_LEADBAR_ITEM = 126,
    O_LEADBAR_OPTION = 127,
    O_MIDAS_TOUCH = 128,
    O_KEY_ITEM1 = 129,
    O_KEY_ITEM2 = 130,
    O_KEY_ITEM3 = 131,
    O_KEY_ITEM4 = 132,
    O_KEY_OPTION1 = 133,
    O_KEY_OPTION2 = 134,
    O_KEY_OPTION3 = 135,
    O_KEY_OPTION4 = 136,
    O_KEY_HOLE1 = 137,
    O_KEY_HOLE2 = 138,
    O_KEY_HOLE3 = 139,
    O_KEY_HOLE4 = 140,
    O_PICKUP_ITEM1 = 141,
    O_PICKUP_ITEM2 = 142,
    O_SCION_ITEM = 143,
    O_SCION_ITEM2 = 144,
    O_SCION_ITEM3 = 145,
    O_SCION_ITEM4 = 146,
    O_SCION_HOLDER = 147,
    O_PICKUP_OPTION1 = 148,
    O_PICKUP_OPTION2 = 149,
    O_SCION_OPTION = 150,
    O_EXPLOSION1 = 151,
    O_EXPLOSION2 = 152,
    O_SPLASH1 = 153,
    O_SPLASH2 = 154,
    O_BUBBLES1 = 155,
    O_BUBBLES2 = 156,
    O_BUBBLE_EMITTER = 157,
    O_BLOOD1 = 158,
    O_BLOOD2 = 159,
    O_DART_EFFECT = 160,
    O_STATUE = 161,
    O_PORTACABIN = 162,
    O_PODS = 163,
    O_RICOCHET1 = 164,
    O_TWINKLE = 165,
    O_GUN_FLASH = 166,
    O_DUST = 167,
    O_BODY_PART = 168,
    O_CAMERA_TARGET = 169,
    O_WATERFALL = 170,
    O_MISSILE1 = 171,
    O_MISSILE2 = 172,
    O_MISSILE3 = 173,
    O_MISSILE4 = 174,
    O_MISSILE5 = 175,
    O_LAVA = 176,
    O_LAVA_EMITTER = 177,
    O_FLAME = 178,
    O_FLAME_EMITTER = 179,
    O_LAVA_WEDGE = 180,
    O_BIG_POD = 181,
    O_BOAT = 182,
    O_EARTHQUAKE = 183,
    O_TEMP5 = 184,
    O_TEMP6 = 185,
    O_TEMP7 = 186,
    O_TEMP8 = 187,
    O_TEMP9 = 188,
    O_TEMP10 = 189,
    O_HAIR = O_TEMP10,
    O_ALPHABET = 190,
    O_NUMBER_OF = 191,
} GAME_OBJECT_ID;

typedef enum GAME_STATIC_ID {
    STATIC_PLANT0 = 0,
    STATIC_PLANT1 = 1,
    STATIC_PLANT2 = 2,
    STATIC_PLANT3 = 3,
    STATIC_PLANT4 = 4,
    STATIC_PLANT5 = 5,
    STATIC_PLANT6 = 6,
    STATIC_PLANT7 = 7,
    STATIC_PLANT8 = 8,
    STATIC_PLANT9 = 9,
    STATIC_FURNITURE0 = 10,
    STATIC_FURNITURE1 = 11,
    STATIC_FURNITURE2 = 12,
    STATIC_FURNITURE3 = 13,
    STATIC_FURNITURE4 = 14,
    STATIC_FURNITURE5 = 15,
    STATIC_FURNITURE6 = 16,
    STATIC_FURNITURE7 = 17,
    STATIC_FURNITURE8 = 18,
    STATIC_FURNITURE9 = 19,
    STATIC_ROCK0 = 20,
    STATIC_ROCK1 = 21,
    STATIC_ROCK2 = 22,
    STATIC_ROCK3 = 23,
    STATIC_ROCK4 = 24,
    STATIC_ROCK5 = 25,
    STATIC_ROCK6 = 26,
    STATIC_ROCK7 = 27,
    STATIC_ROCK8 = 28,
    STATIC_ROCK9 = 29,
    STATIC_ARCHITECTURE0 = 30,
    STATIC_ARCHITECTURE1 = 31,
    STATIC_ARCHITECTURE2 = 32,
    STATIC_ARCHITECTURE3 = 33,
    STATIC_ARCHITECTURE4 = 34,
    STATIC_ARCHITECTURE5 = 35,
    STATIC_ARCHITECTURE6 = 36,
    STATIC_ARCHITECTURE7 = 37,
    STATIC_ARCHITECTURE8 = 38,
    STATIC_ARCHITECTURE9 = 39,
    STATIC_DEBRIS0 = 40,
    STATIC_DEBRIS1 = 41,
    STATIC_DEBRIS2 = 42,
    STATIC_DEBRIS3 = 43,
    STATIC_DEBRIS4 = 44,
    STATIC_DEBRIS5 = 45,
    STATIC_DEBRIS6 = 46,
    STATIC_DEBRIS7 = 47,
    STATIC_DEBRIS8 = 48,
    STATIC_DEBRIS9 = 49,
    STATIC_NUMBER_OF = 50,
} GAME_STATIC_ID;

typedef enum SOUND_EFFECT_ID {
    SFX_LARA_FEET = 0,
    SFX_LARA_CLIMB2 = 1,
    SFX_LARA_NO = 2,
    SFX_LARA_SLIPPING = 3,
    SFX_LARA_LAND = 4,
    SFX_LARA_CLIMB1 = 5,
    SFX_LARA_DRAW = 6,
    SFX_LARA_HOLSTER = 7,
    SFX_LARA_FIRE = 8,
    SFX_LARA_RELOAD = 9,
    SFX_LARA_RICOCHET = 10,
    SFX_BEAR_GROWL = 11,
    SFX_BEAR_FEET = 12,
    SFX_BEAR_ATTACK = 13,
    SFX_BEAR_SNARL = 14,
    SFX_BEAR_HURT = 16,
    SFX_BEAR_DEATH = 18,
    SFX_WOLF_JUMP = 19,
    SFX_WOLF_HURT = 20,
    SFX_WOLF_DEATH = 22,
    SFX_WOLF_HOWL = 24,
    SFX_WOLF_ATTACK = 25,
    SFX_LARA_CLIMB3 = 26,
    SFX_LARA_BODYSL = 27,
    SFX_LARA_SHIMMY2 = 28,
    SFX_LARA_JUMP = 29,
    SFX_LARA_FALL = 30,
    SFX_LARA_INJURY = 31,
    SFX_LARA_ROLL = 32,
    SFX_LARA_SPLASH = 33,
    SFX_LARA_GETOUT = 34,
    SFX_LARA_SWIM = 35,
    SFX_LARA_BREATH = 36,
    SFX_LARA_BUBBLES = 37,
    SFX_LARA_SWITCH = 38,
    SFX_LARA_KEY = 39,
    SFX_LARA_OBJECT = 40,
    SFX_LARA_GENERAL_DEATH = 41,
    SFX_LARA_KNEES_DEATH = 42,
    SFX_LARA_UZI_FIRE = 43,
    SFX_LARA_MAGNUMS = 44,
    SFX_LARA_SHOTGUN = 45,
    SFX_LARA_BLOCK_PUSH1 = 46,
    SFX_LARA_BLOCK_PUSH2 = 47,
    SFX_LARA_EMPTY = 48,
    SFX_LARA_BULLETHIT = 50,
    SFX_LARA_BLKPULL = 51,
    SFX_LARA_FLOATING = 52,
    SFX_LARA_FALLDETH = 53,
    SFX_LARA_GRABHAND = 54,
    SFX_LARA_GRABBODY = 55,
    SFX_LARA_GRABFEET = 56,
    SFX_LARA_SWITCHUP = 57,
    SFX_BAT_SQK = 58,
    SFX_BAT_FLAP = 59,
    SFX_UNDERWATER = 60,
    SFX_UNDERWATER_SWITCH = 61,
    SFX_BLOCK_SOUND = 63,
    SFX_DOOR = 64,
    SFX_PENDULUM_BLADES = 65,
    SFX_ROCK_FALL_CRUMBLE = 66,
    SFX_ROCK_FALL_FALL = 67,
    SFX_ROCK_FALL_LAND = 68,
    SFX_T_REX_DEATH = 69,
    SFX_T_REX_FOOTSTOMP = 70,
    SFX_T_REX_ROAR = 71,
    SFX_T_REX_ATTACK = 72,
    SFX_RAPTOR_ROAR = 73,
    SFX_RAPTOR_ATTACK = 74,
    SFX_RAPTOR_FEET = 75,
    SFX_MUMMY_GROWL = 76,
    SFX_LARSON_FIRE = 77,
    SFX_LARSON_RICOCHET = 78,
    SFX_WATERFALL_LOOP = 79,
    SFX_WATER_LOOP = 80,
    SFX_WATERFALL_BIG = 81,
    SFX_CHAINDOOR_UP = 82,
    SFX_CHAINDOOR_DOWN = 83,
    SFX_COGS = 84,
    SFX_LION_HURT = 85,
    SFX_LION_ATTACK = 86,
    SFX_LION_ROAR = 87,
    SFX_LION_DEATH = 88,
    SFX_GORILLA_FEET = 89,
    SFX_GORILLA_PANT = 90,
    SFX_GORILLA_DEATH = 91,
    SFX_CROC_FEET = 92,
    SFX_CROC_ATTACK = 93,
    SFX_RAT_FEET = 94,
    SFX_RAT_CHIRP = 95,
    SFX_RAT_ATTACK = 96,
    SFX_RAT_DEATH = 97,
    SFX_THUNDER = 98,
    SFX_EXPLOSION = 99,
    SFX_GORILLA_GRUNT = 100,
    SFX_GORILLA_GRUNTS = 101,
    SFX_CROC_DEATH = 102,
    SFX_DAMOCLES_SWORD = 103,
    SFX_ATLANTEAN_EXPLODE = 104,
    SFX_MENU_ROTATE = 108,
    SFX_MENU_CHOOSE = 109,
    SFX_MENU_GAMEBOY = 110,
    SFX_MENU_SPININ = 111,
    SFX_MENU_SPINOUT = 112,
    SFX_MENU_COMPASS = 113,
    SFX_MENU_GUNS = 114,
    SFX_MENU_PASSPORT = 115,
    SFX_MENU_MEDI = 116,
    SFX_RAISINGBLOCK_FX = 117,
    SFX_SAND_FX = 118,
    SFX_STAIRS2SLOPE_FX = 119,
    SFX_ATLANTEAN_WALK = 120,
    SFX_ATLANTEAN_ATTACK = 121,
    SFX_ATLANTEAN_JUMP_ATTACK = 122,
    SFX_ATLANTEAN_NEEDLE = 123,
    SFX_ATLANTEAN_BALL = 124,
    SFX_ATLANTEAN_WINGS = 125,
    SFX_ATLANTEAN_RUN = 126,
    SFX_SLAMDOOR_CLOSE = 127,
    SFX_SLAMDOOR_OPEN = 128,
    SFX_SKATEBOARD_MOVE = 129,
    SFX_SKATEBOARD_STOP = 130,
    SFX_SKATEBOARD_SHOOT = 131,
    SFX_SKATEBOARD_HIT = 132,
    SFX_SKATEBOARD_START = 133,
    SFX_SKATEBOARD_DEATH = 134,
    SFX_SKATEBOARD_HIT_GROUND = 135,
    SFX_TORSO_HIT_GROUND = 136,
    SFX_TORSO_ATTACK1 = 137,
    SFX_TORSO_ATTACK2 = 138,
    SFX_TORSO_DEATH = 139,
    SFX_TORSO_ARM_SWING = 140,
    SFX_TORSO_MOVE = 141,
    SFX_TORSO_HIT = 142,
    SFX_CENTAUR_FEET = 143,
    SFX_CENTAUR_ROAR = 144,
    SFX_LARA_SPIKE_DEATH = 145,
    SFX_LARA_DEATH3 = 146,
    SFX_ROLLING_BALL = 147,
    SFX_LAVA_LOOP = 148,
    SFX_LAVA_FOUNTAIN = 149,
    SFX_FIRE = 150,
    SFX_DARTS = 151,
    SFX_METAL_DOOR_CLOSE = 152,
    SFX_METAL_DOOR_OPEN = 153,
    SFX_ALTAR_LOOP = 154,
    SFX_POWERUP_FX = 155,
    SFX_COWBOY_DEATH = 156,
    SFX_BLACK_GOON_DEATH = 157,
    SFX_LARSON_DEATH = 158,
    SFX_PIERRE_DEATH = 159,
    SFX_NATLA_DEATH = 160,
    SFX_TRAPDOOR_OPEN = 161,
    SFX_TRAPDOOR_CLOSE = 162,
    SFX_ATLANTEAN_EGG_LOOP = 163,
    SFX_ATLANTEAN_EGG_HATCH = 164,
    SFX_DRILL_ENGINE_START = 165,
    SFX_DRILL_ENGINE_LOOP = 166,
    SFX_CONVEYOR_BELT = 167,
    SFX_HUT_LOWERED = 168,
    SFX_HUT_HIT_GROUND = 169,
    SFX_EXPLOSION_FX = 170,
    SFX_ATLANTEAN_DEATH = 171,
    SFX_CHAINBLOCK_FX = 172,
    SFX_SECRET = 173,
    SFX_GYM_HINT_01 = 174,
    SFX_GYM_HINT_02 = 175,
    SFX_GYM_HINT_03 = 176,
    SFX_GYM_HINT_04 = 177,
    SFX_GYM_HINT_05 = 178,
    SFX_GYM_HINT_06 = 179,
    SFX_GYM_HINT_07 = 180,
    SFX_GYM_HINT_08 = 181,
    SFX_GYM_HINT_09 = 182,
    SFX_GYM_HINT_10 = 183,
    SFX_GYM_HINT_11 = 184,
    SFX_GYM_HINT_12 = 185,
    SFX_GYM_HINT_13 = 186,
    SFX_GYM_HINT_14 = 187,
    SFX_GYM_HINT_15 = 188,
    SFX_GYM_HINT_16 = 189,
    SFX_GYM_HINT_17 = 190,
    SFX_GYM_HINT_18 = 191,
    SFX_GYM_HINT_19 = 192,
    SFX_GYM_HINT_20 = 193,
    SFX_GYM_HINT_21 = 194,
    SFX_GYM_HINT_22 = 195,
    SFX_GYM_HINT_23 = 196,
    SFX_GYM_HINT_24 = 197,
    SFX_GYM_HINT_25 = 198,
    SFX_BALDY_SPEECH = 199,
    SFX_COWBOY_SPEECH = 200,
    SFX_LARSON_SPEECH = 201,
    SFX_NATLA_SPEECH = 202,
    SFX_PIERRE_SPEECH = 203,
    SFX_SKATEKID_SPEECH = 204,
    SFX_LARA_SETUP = 205,
} SOUND_EFFECT_ID;

typedef enum MUSIC_TRACK_ID {
    MX_UNUSED_0 = 0,
    MX_UNUSED_1 = 1,
    MX_TR_THEME = 2,
    MX_WHERE_THE_DEPTHS_UNFOLD_1 = 3,
    MX_TR_THEME_ALT_1 = 4,
    MX_CAVES_AMBIENT = 5,
    MX_TIME_TO_RUN_1 = 6,
    MX_FRIEND_SINCE_GONE = 7,
    MX_T_REX_1 = 8,
    MX_A_LONG_WAY_DOWN = 9,
    MX_LONGING_FOR_HOME = 10,
    MX_SPOOKY_1 = 11,
    MX_KEEP_YOUR_BALANCE = 12,
    MX_SECRET = 13,
    MX_SPOOKY_3 = 14,
    MX_WHERE_THE_DEPTHS_UNFOLD_2 = 15,
    MX_T_REX_2 = 16,
    MX_WHERE_THE_DEPTHS_UNFOLD_3 = 17,
    MX_WHERE_THE_DEPTHS_UNFOLD_4 = 18,
    MX_TR_THEME_ALT_2 = 19,
    MX_TIME_TO_RUN_2 = 20,
    MX_LONGING_FOR_HOME_ALT = 21,
    MX_NATLA_FALLS_CUTSCENE = 22,
    MX_LARSON_CUTSCENE = 23,
    MX_NATLA_PLACES_SCION_CUTSCENE = 24,
    MX_LARA_TIHOCAN_CUTSCENE = 25,
    MX_GYM_HINT_01 = 26,
    MX_GYM_HINT_02 = 27,
    MX_GYM_HINT_03 = 28,
    MX_GYM_HINT_04 = 29,
    MX_GYM_HINT_05 = 30,
    MX_GYM_HINT_06 = 31,
    MX_GYM_HINT_07 = 32,
    MX_GYM_HINT_08 = 33,
    MX_GYM_HINT_09 = 34,
    MX_GYM_HINT_10 = 35,
    MX_GYM_HINT_11 = 36,
    MX_GYM_HINT_12 = 37,
    MX_GYM_HINT_13 = 38,
    MX_GYM_HINT_14 = 39,
    MX_GYM_HINT_15 = 40,
    MX_GYM_HINT_16 = 41,
    MX_GYM_HINT_17 = 42,
    MX_GYM_HINT_18 = 43,
    MX_GYM_HINT_19 = 44,
    MX_GYM_HINT_20 = 45,
    MX_GYM_HINT_21 = 46,
    MX_GYM_HINT_22 = 47,
    MX_GYM_HINT_23 = 48,
    MX_GYM_HINT_24 = 49,
    MX_GYM_HINT_25 = 50,
    MX_BALDY_SPEECH = 51,
    MX_COWBOY_SPEECH = 52,
    MX_LARSON_SPEECH = 53,
    MX_NATLA_SPEECH = 54,
    MX_PIERRE_SPEECH = 55,
    MX_SKATEKID_SPEECH = 56,
    MX_ST_FRANCIS_FOLLY_AMBIENCE = 57,
    MX_CISTERN_AMBIENCE = 58,
    MX_WINDY_AMBIENCE = 59,
    MX_ATLANTIS_AMBIENCE = 60,
    MX_NUMBER_OF,
} MUSIC_TRACK_ID;

typedef enum LARA_ANIMATION_FRAME {
    AF_VAULT12 = 759,
    AF_VAULT34 = 614,
    AF_FASTFALL = 481,
    AF_STOP = 185,
    AF_FALLDOWN = 492,
    AF_STOP_LEFT = 58,
    AF_STOP_RIGHT = 74,
    AF_HITWALLLEFT = 800,
    AF_HITWALLRIGHT = 815,
    AF_RUNSTEPUP_LEFT = 837,
    AF_RUNSTEPUP_RIGHT = 830,
    AF_WALKSTEPUP_LEFT = 844,
    AF_WALKSTEPUP_RIGHT = 858,
    AF_WALKSTEPD_LEFT = 887,
    AF_WALKSTEPD_RIGHT = 874,
    AF_BACKSTEPD_LEFT = 899,
    AF_BACKSTEPD_RIGHT = 930,
    AF_LANDFAR = 358,
    AF_GRABLEDGE = 1493,
    AF_GRABLEDGE_OLD = 621,
    AF_SWIMGLIDE = 1431,
    AF_FALLBACK = 1473,
    AF_HANG = 1514,
    AF_HANG_OLD = 642,
    AF_STARTHANG = 1505,
    AF_STARTHANG_OLD = 634,
    AF_STOPHANG = 448,
    AF_SLIDE = 1133,
    AF_SLIDEBACK = 1677,
    AF_TREAD = 1736,
    AF_SURFTREAD = 1937,
    AF_SURFDIVE = 2041,
    AF_SURFCLIMB = 1849,
    AF_JUMPIN = 1895,
    AF_ROLL = 3857,
    AF_RBALL_DEATH = 3561,
    AF_SPIKE_DEATH = 3887,
    AF_GRABLEDGEIN = 3974,
    AF_PPREADY = 2091,
    AF_PICKUP_ERASE = 3443,
    AF_PICKUP_UW = 2970,
    AF_PICKUPSCION = 44,
    AF_USEPUZZLE = 3372,
} LARA_ANIMATION_FRAME;

typedef enum LARA_SHOTGUN_ANIMATION_FRAME {
    AF_SG_AIM = 0,
    AF_SG_DRAW = 13,
    AF_SG_RECOIL = 47,
    AF_SG_UNDRAW = 80,
    AF_SG_UNAIM = 114,
    AF_SG_END = 127,
} LARA_SHOTGUN_ANIMATION_FRAME;

typedef enum LARA_GUN_ANIMATION_FRAME {
    AF_G_AIM = 0,
    AF_G_AIM_L = 4,
    AF_G_DRAW1 = 5,
    AF_G_DRAW1_L = 12,
    AF_G_DRAW2 = 13,
    AF_G_DRAW2_L = 23,
    AF_G_RECOIL = 24,
} LARA_GUN_ANIMATION_FRAME;

typedef enum LARA_ANIMATION {
    LA_RUN = 0,
    LA_WALK_FORWARD = 1,
    LA_WALK_BACK = 40,
    LA_VAULT_12 = 50,
    LA_VAULT_34 = 42,
    LA_FAST_FALL = 32,
    LA_STOP = 11,
    LA_FALL_DOWN = 34,
    LA_STOP_LEFT = 2,
    LA_STOP_RIGHT = 3,
    LA_HIT_WALL_LEFT = 53,
    LA_HIT_WALL_RIGHT = 54,
    LA_RUN_STEP_UP_LEFT = 56,
    LA_RUN_STEP_UP_RIGHT = 55,
    LA_WALK_STEP_UP_LEFT = 57,
    LA_WALK_STEP_UP_RIGHT = 58,
    LA_WALK_STEP_DOWN_LEFT = 60,
    LA_WALK_STEP_DOWN_RIGHT = 59,
    LA_BACK_STEP_DOWN_LEFT = 61,
    LA_BACK_STEP_DOWN_RIGHT = 62,
    LA_WALL_SWITCH_DOWN = 63,
    LA_WALL_SWITCH_UP = 64,
    LA_SIDE_STEP_LEFT = 65,
    LA_SIDE_STEP_LEFT_END = 66,
    LA_SIDE_STEP_RIGHT = 67,
    LA_SIDE_STEP_RIGHT_END = 68,
    LA_LAND_FAR = 24,
    LA_GRAB_LEDGE = 96,
    LA_GRAB_LEDGE_OLD = 32,
    LA_SWIM_GLIDE = 87,
    LA_FALL_BACK = 93,
    LA_HANG = 96,
    LA_HANG_OLD = 33,
    LA_START_HANG = 96,
    LA_START_HANG_OLD = 33,
    LA_STOP_HANG = 28,
    LA_SLIDE = 70,
    LA_SLIDE_BACK = 104,
    LA_TREAD = 108,
    LA_SURF_TREAD = 114,
    LA_SURF_DIVE = 119,
    LA_SURF_CLIMB = 111,
    LA_JUMP_IN = 112,
    LA_ROLL = 146,
    LA_PICKUP_UW = 130,
    LA_PICKUP = 135,
    LA_ROLLING_BALL_DEATH = 139,
    LA_SPIKE_DEATH = 149,
    LA_GRAB_LEDGE_IN = 150,
    LA_SPAZ_FORWARD = 125,
    LA_SPAZ_BACK = 126,
    LA_SPAZ_RIGHT = 127,
    LA_SPAZ_LEFT = 128,
} LARA_ANIMATION;

typedef enum LARA_WATER_STATUS {
    LWS_ABOVE_WATER = 0,
    LWS_UNDERWATER = 1,
    LWS_SURFACE = 2,
    LWS_CHEAT = 3,
} LARA_WATER_STATUS;

typedef enum LARA_STATE {
    LS_WALK = 0,
    LS_RUN = 1,
    LS_STOP = 2,
    LS_JUMP_FORWARD = 3,
    LS_POSE = 4,
    LS_FAST_BACK = 5,
    LS_TURN_R = 6,
    LS_TURN_L = 7,
    LS_DEATH = 8,
    LS_FAST_FALL = 9,
    LS_HANG = 10,
    LS_REACH = 11,
    LS_SPLAT = 12,
    LS_TREAD = 13,
    LS_LAND = 14,
    LS_COMPRESS = 15,
    LS_BACK = 16,
    LS_SWIM = 17,
    LS_GLIDE = 18,
    LS_NULL = 19,
    LS_FAST_TURN = 20,
    LS_STEP_RIGHT = 21,
    LS_STEP_LEFT = 22,
    LS_HIT = 23,
    LS_SLIDE = 24,
    LS_JUMP_BACK = 25,
    LS_JUMP_RIGHT = 26,
    LS_JUMP_LEFT = 27,
    LS_JUMP_UP = 28,
    LS_FALL_BACK = 29,
    LS_HANG_LEFT = 30,
    LS_HANG_RIGHT = 31,
    LS_SLIDE_BACK = 32,
    LS_SURF_TREAD = 33,
    LS_SURF_SWIM = 34,
    LS_DIVE = 35,
    LS_PUSH_BLOCK = 36,
    LS_PULL_BLOCK = 37,
    LS_PP_READY = 38,
    LS_PICKUP = 39,
    LS_SWITCH_ON = 40,
    LS_SWITCH_OFF = 41,
    LS_USE_KEY = 42,
    LS_USE_PUZZLE = 43,
    LS_UW_DEATH = 44,
    LS_ROLL = 45,
    LS_SPECIAL = 46,
    LS_SURF_BACK = 47,
    LS_SURF_LEFT = 48,
    LS_SURF_RIGHT = 49,
    LS_USE_MIDAS = 50,
    LS_DIE_MIDAS = 51,
    LS_SWAN_DIVE = 52,
    LS_FAST_DIVE = 53,
    LS_GYMNAST = 54,
    LS_WATER_OUT = 55,
    LS_CONTROLLED = 56,
} LARA_STATE;

typedef enum LARA_GUN_STATE {
    LGS_ARMLESS = 0,
    LGS_HANDS_BUSY = 1,
    LGS_DRAW = 2,
    LGS_UNDRAW = 3,
    LGS_READY = 4,
} LARA_GUN_STATE;

typedef enum LARA_GUN_TYPE {
    LGT_UNARMED = 0,
    LGT_PISTOLS = 1,
    LGT_MAGNUMS = 2,
    LGT_UZIS = 3,
    LGT_SHOTGUN = 4,
    NUM_WEAPONS = 5
} LARA_GUN_TYPE;

typedef enum LARA_MESH {
    LM_HIPS = 0,
    LM_THIGH_L = 1,
    LM_CALF_L = 2,
    LM_FOOT_L = 3,
    LM_THIGH_R = 4,
    LM_CALF_R = 5,
    LM_FOOT_R = 6,
    LM_TORSO = 7,
    LM_UARM_R = 8,
    LM_LARM_R = 9,
    LM_HAND_R = 10,
    LM_UARM_L = 11,
    LM_LARM_L = 12,
    LM_HAND_L = 13,
    LM_HEAD = 14,
    LM_NUMBER_OF = 15,
} LARA_MESH;

typedef enum MOOD_TYPE {
    MOOD_BORED = 0,
    MOOD_ATTACK = 1,
    MOOD_ESCAPE = 2,
    MOOD_STALK = 3,
} MOOD_TYPE;

typedef enum TARGET_TYPE {
    TARGET_NONE = 0,
    TARGET_PRIMARY = 1,
    TARGET_SECONDARY = 2,
} TARGET_TYPE;

typedef enum D_FLAGS {
    D_TRANS1 = 1,
    D_TRANS2 = 2,
    D_TRANS3 = 3,
    D_TRANS4 = 4,
    D_NEXT = 1 << 3,
} D_FLAGS;

typedef enum COLL_TYPE {
    COLL_NONE = 0,
    COLL_FRONT = 1,
    COLL_LEFT = 2,
    COLL_RIGHT = 4,
    COLL_TOP = 8,
    COLL_TOPFRONT = 16,
    COLL_CLAMP = 32,
} COLL_TYPE;

typedef enum HEIGHT_TYPE {
    HT_WALL = 0,
    HT_SMALL_SLOPE = 1,
    HT_BIG_SLOPE = 2,
} HEIGHT_TYPE;

typedef enum DIRECTION {
    DIR_NORTH = 0,
    DIR_EAST = 1,
    DIR_SOUTH = 2,
    DIR_WEST = 3,
} DIRECTION;

typedef enum ANIM_COMMAND {
    AC_NULL = 0,
    AC_MOVE_ORIGIN = 1,
    AC_JUMP_VELOCITY = 2,
    AC_ATTACK_READY = 3,
    AC_DEACTIVATE = 4,
    AC_SOUND_FX = 5,
    AC_EFFECT = 6,
} ANIM_COMMAND;

typedef enum BONE_EXTRA_BITS {
    BEB_POP = 1 << 0,
    BEB_PUSH = 1 << 1,
    BEB_ROT_X = 1 << 2,
    BEB_ROT_Y = 1 << 3,
    BEB_ROT_Z = 1 << 4,
} BONE_EXTRA_BITS;

typedef enum ITEM_STATUS {
    IS_NOT_ACTIVE = 0,
    IS_ACTIVE = 1,
    IS_DEACTIVATED = 2,
    IS_INVISIBLE = 3,
} ITEM_STATUS;

typedef enum ROOM_FLAG {
    RF_UNDERWATER = 1,
} ROOM_FLAG;

typedef enum FLOOR_TYPE {
    FT_FLOOR = 0,
    FT_DOOR = 1,
    FT_TILT = 2,
    FT_ROOF = 3,
    FT_TRIGGER = 4,
    FT_LAVA = 5,
} FLOOR_TYPE;

typedef enum TRIGGER_TYPE {
    TT_TRIGGER = 0,
    TT_PAD = 1,
    TT_SWITCH = 2,
    TT_KEY = 3,
    TT_PICKUP = 4,
    TT_HEAVY = 5,
    TT_ANTIPAD = 6,
    TT_COMBAT = 7,
    TT_DUMMY = 8,
} TRIGGER_TYPE;

typedef enum TRIGGER_OBJECT {
    TO_OBJECT = 0,
    TO_CAMERA = 1,
    TO_SINK = 2,
    TO_FLIPMAP = 3,
    TO_FLIPON = 4,
    TO_FLIPOFF = 5,
    TO_TARGET = 6,
    TO_FINISH = 7,
    TO_CD = 8,
    TO_FLIPEFFECT = 9,
    TO_SECRET = 10,
} TRIGGER_OBJECT;

typedef enum ITEM_FLAG {
    IF_ONESHOT = 0x0100,
    IF_CODE_BITS = 0x3E00,
    IF_REVERSE = 0x4000,
    IF_NOT_VISIBLE = 0x0100,
    IF_KILLED_ITEM = 0x8000,
} ITEM_FLAG;

typedef enum INV_MODE {
    INV_GAME_MODE = 0,
    INV_TITLE_MODE = 1,
    INV_KEYS_MODE = 2,
    INV_SAVE_MODE = 3,
    INV_LOAD_MODE = 4,
    INV_DEATH_MODE = 5,
    INV_SAVE_CRYSTAL_MODE = 6,
} INV_MODE;

typedef enum INV_TEXT {
    IT_NAME = 0,
    IT_QTY = 1,
    IT_NUMBER_OF = 2,
} INV_TEXT;

typedef enum INV_COLOUR {
    IC_BLACK = 0,
    IC_GREY = 1,
    IC_WHITE = 2,
    IC_RED = 3,
    IC_ORANGE = 4,
    IC_YELLOW = 5,
    IC_GREEN1 = 6,
    IC_GREEN2 = 7,
    IC_GREEN3 = 8,
    IC_GREEN4 = 9,
    IC_GREEN5 = 10,
    IC_GREEN6 = 11,
    IC_DARKGREEN = 12,
    IC_GREEN = 13,
    IC_CYAN = 14,
    IC_BLUE = 15,
    IC_MAGENTA = 16,
    IC_NUMBER_OF = 17,
} INV_COLOUR;

typedef enum RING_STATUS {
    RNG_OPENING = 0,
    RNG_OPEN = 1,
    RNG_CLOSING = 2,
    RNG_MAIN2OPTION = 3,
    RNG_MAIN2KEYS = 4,
    RNG_KEYS2MAIN = 5,
    RNG_OPTION2MAIN = 6,
    RNG_SELECTING = 7,
    RNG_SELECTED = 8,
    RNG_DESELECTING = 9,
    RNG_DESELECT = 10,
    RNG_CLOSING_ITEM = 11,
    RNG_EXITING_INVENTORY = 12,
    RNG_DONE = 13,
} RING_STATUS;

typedef enum RING_TYPE {
    RT_MAIN = 0,
    RT_OPTION = 1,
    RT_KEYS = 2,
} RING_TYPE;

typedef enum SHAPE {
    SHAPE_SPRITE = 1,
    SHAPE_LINE = 2,
    SHAPE_BOX = 3,
    SHAPE_FBOX = 4
} SHAPE;

typedef enum DOOR_ANIM {
    DOOR_CLOSED = 0,
    DOOR_OPEN = 1,
} DOOR_ANIM;

typedef enum TRAP_ANIM {
    TRAP_SET = 0,
    TRAP_ACTIVATE = 1,
    TRAP_WORKING = 2,
    TRAP_FINISHED = 3,
} TRAP_ANIM;

typedef enum ROLLING_BLOCK_STATE {
    RBS_START = 0,
    RBS_END = 1,
    RBS_MOVING = 2,
} ROLLING_BLOCK_STATE;

typedef enum REQUEST_INFO_FLAGS {
    RIF_BLOCKED = 1 << 0,
    RIF_BLOCKABLE = 1 << 1,
} REQUEST_INFO_FLAGS;

typedef enum GAMEFLOW_LEVEL_TYPE {
    GFL_TITLE = 0,
    GFL_NORMAL = 1,
    GFL_SAVED = 2,
    GFL_DEMO = 3,
    GFL_CUTSCENE = 4,
    GFL_GYM = 5,
    GFL_CURRENT = 6, // legacy level type for reading TombATI's savegames
    GFL_RESTART = 7,
    GFL_SELECT = 8,
} GAMEFLOW_LEVEL_TYPE;

typedef enum GAMEFLOW_OPTION {
    GF_NOP_BREAK = -2,
    GF_NOP = -1,
    GF_START_GAME = 0,
    GF_START_CINE = 1 << 6,
    GF_START_FMV = 2 << 6,
    GF_START_DEMO = 3 << 6,
    GF_EXIT_TO_TITLE = 4 << 6,
    GF_LEVEL_COMPLETE = 5 << 6,
    GF_EXIT_GAME = 6 << 6,
    GF_START_SAVED_GAME = 7 << 6,
    GF_RESTART_GAME = 8 << 6,
    GF_SELECT_GAME = 9 << 6,
    GF_START_GYM = 10 << 6,
    GF_STORY_SO_FAR = 11 << 6,
} GAMEFLOW_OPTION;

typedef enum GAMEFLOW_SEQUENCE_TYPE {
    GFS_END = -1,
    GFS_START_GAME,
    GFS_LOOP_GAME,
    GFS_STOP_GAME,
    GFS_START_CINE,
    GFS_LOOP_CINE,
    GFS_STOP_CINE,
    GFS_PLAY_FMV,
    GFS_LEVEL_STATS,
    GFS_TOTAL_STATS,
    GFS_DISPLAY_PICTURE,
    GFS_EXIT_TO_TITLE,
    GFS_EXIT_TO_LEVEL,
    GFS_EXIT_TO_CINE,
    GFS_SET_CAM_X,
    GFS_SET_CAM_Y,
    GFS_SET_CAM_Z,
    GFS_SET_CAM_ANGLE,
    GFS_FLIP_MAP,
    GFS_REMOVE_GUNS,
    GFS_REMOVE_SCIONS,
    GFS_GIVE_ITEM,
    GFS_PLAY_SYNCED_AUDIO,
    GFS_MESH_SWAP,
    GFS_FIX_PYRAMID_SECRET_TRIGGER,
    GFS_REMOVE_AMMO,
    GFS_REMOVE_MEDIPACKS,
} GAMEFLOW_SEQUENCE_TYPE;

typedef enum GAME_STRING_ID {
    GS_HEADING_INVENTORY,
    GS_HEADING_GAME_OVER,
    GS_HEADING_OPTION,
    GS_HEADING_ITEMS,

    GS_PASSPORT_SELECT_LEVEL,
    GS_PASSPORT_STORY_SO_FAR,
    GS_PASSPORT_LEGACY_SELECT_LEVEL_1,
    GS_PASSPORT_LEGACY_SELECT_LEVEL_2,
    GS_PASSPORT_RESTART_LEVEL,
    GS_PASSPORT_SELECT_MODE,
    GS_PASSPORT_MODE_NEW_GAME,
    GS_PASSPORT_MODE_NEW_GAME_PLUS,
    GS_PASSPORT_MODE_NEW_GAME_JP,
    GS_PASSPORT_MODE_NEW_GAME_JP_PLUS,
    GS_PASSPORT_NEW_GAME,
    GS_PASSPORT_LOAD_GAME,
    GS_PASSPORT_SAVE_GAME,
    GS_PASSPORT_EXIT_GAME,
    GS_PASSPORT_EXIT_TO_TITLE,

    GS_DETAIL_SELECT_DETAIL,
    GS_DETAIL_LEVEL_HIGH,
    GS_DETAIL_LEVEL_MEDIUM,
    GS_DETAIL_LEVEL_LOW,
    GS_DETAIL_PERSPECTIVE,
    GS_DETAIL_BILINEAR,
    GS_DETAIL_VSYNC,
    GS_DETAIL_BRIGHTNESS,
    GS_DETAIL_UI_TEXT_SCALE,
    GS_DETAIL_UI_BAR_SCALE,
    GS_DETAIL_RESOLUTION,
    GS_DETAIL_FLOAT_FMT,
    GS_DETAIL_RESOLUTION_FMT,

    GS_SOUND_SET_VOLUMES,

    GS_CONTROL_DEFAULT_KEYS,
    GS_CONTROL_CUSTOM_1,
    GS_CONTROL_CUSTOM_2,
    GS_CONTROL_CUSTOM_3,
    GS_CONTROL_RESET_DEFAULTS,
    GS_CONTROL_UNBIND,

    GS_KEYMAP_RUN,
    GS_KEYMAP_BACK,
    GS_KEYMAP_LEFT,
    GS_KEYMAP_RIGHT,
    GS_KEYMAP_STEP_LEFT,
    GS_KEYMAP_STEP_RIGHT,
    GS_KEYMAP_WALK,
    GS_KEYMAP_JUMP,
    GS_KEYMAP_ACTION,
    GS_KEYMAP_DRAW_WEAPON,
    GS_KEYMAP_LOOK,
    GS_KEYMAP_ROLL,
    GS_KEYMAP_INVENTORY,
    GS_KEYMAP_FLY_CHEAT,
    GS_KEYMAP_ITEM_CHEAT,
    GS_KEYMAP_LEVEL_SKIP_CHEAT,
    GS_KEYMAP_TURBO_CHEAT,
    GS_KEYMAP_PAUSE,
    GS_KEYMAP_CAMERA_UP,
    GS_KEYMAP_CAMERA_DOWN,
    GS_KEYMAP_CAMERA_LEFT,
    GS_KEYMAP_CAMERA_RIGHT,
    GS_KEYMAP_CAMERA_RESET,
    GS_KEYMAP_EQUIP_PISTOLS,
    GS_KEYMAP_EQUIP_SHOTGUN,
    GS_KEYMAP_EQUIP_MAGNUMS,
    GS_KEYMAP_EQUIP_UZIS,
    GS_KEYMAP_USE_SMALL_MEDI,
    GS_KEYMAP_USE_BIG_MEDI,
    GS_KEYMAP_SAVE,
    GS_KEYMAP_LOAD,
    GS_KEYMAP_FPS,
    GS_KEYMAP_BILINEAR,

    GS_STATS_KILLS_DETAIL_FMT,
    GS_STATS_KILLS_BASIC_FMT,
    GS_STATS_PICKUPS_DETAIL_FMT,
    GS_STATS_PICKUPS_BASIC_FMT,
    GS_STATS_SECRETS_FMT,
    GS_STATS_DEATHS_FMT,
    GS_STATS_TIME_TAKEN_FMT,
    GS_STATS_FINAL_STATISTICS,

    GS_PAUSE_PAUSED,
    GS_PAUSE_EXIT_TO_TITLE,
    GS_PAUSE_CONTINUE,
    GS_PAUSE_QUIT,
    GS_PAUSE_ARE_YOU_SURE,
    GS_PAUSE_YES,
    GS_PAUSE_NO,

    GS_MISC_ON,
    GS_MISC_OFF,
    GS_MISC_EMPTY_SLOT_FMT,
    GS_MISC_DEMO_MODE,

    GS_INV_ITEM_MEDI,
    GS_INV_ITEM_BIG_MEDI,

    GS_INV_ITEM_PUZZLE1,
    GS_INV_ITEM_PUZZLE2,
    GS_INV_ITEM_PUZZLE3,
    GS_INV_ITEM_PUZZLE4,

    GS_INV_ITEM_KEY1,
    GS_INV_ITEM_KEY2,
    GS_INV_ITEM_KEY3,
    GS_INV_ITEM_KEY4,

    GS_INV_ITEM_PICKUP1,
    GS_INV_ITEM_PICKUP2,
    GS_INV_ITEM_LEADBAR,
    GS_INV_ITEM_SCION,

    GS_INV_ITEM_PISTOLS,
    GS_INV_ITEM_SHOTGUN,
    GS_INV_ITEM_MAGNUM,
    GS_INV_ITEM_UZI,
    GS_INV_ITEM_GRENADE,

    GS_INV_ITEM_PISTOL_AMMO,
    GS_INV_ITEM_SHOTGUN_AMMO,
    GS_INV_ITEM_MAGNUM_AMMO,
    GS_INV_ITEM_UZI_AMMO,

    GS_INV_ITEM_COMPASS,
    GS_INV_ITEM_GAME,
    GS_INV_ITEM_DETAILS,
    GS_INV_ITEM_SOUND,
    GS_INV_ITEM_CONTROLS,
    GS_INV_ITEM_GAMMA,
    GS_INV_ITEM_LARAS_HOME,

    GS_NUMBER_OF,
} GAME_STRING_ID;

typedef enum BAR_TYPE {
    BT_LARA_HEALTH = 0,
    BT_LARA_AIR = 1,
    BT_ENEMY_HEALTH = 2,
} BAR_TYPE;

typedef enum SOUND_PLAY_MODE {
    SPM_NORMAL = 0,
    SPM_UNDERWATER = 1,
    SPM_ALWAYS = 2,
} SOUND_PLAY_MODE;

typedef enum GAME_BONUS_FLAG {
    GBF_NGPLUS = 1 << 0,
    GBF_JAPANESE = 1 << 1,
} GAME_BONUS_FLAG;

typedef enum PASSPORT_PAGE {
    PASSPORT_PAGE_FLIPPING = -1,
    PASSPORT_PAGE_1 = 0,
    PASSPORT_PAGE_2 = 1,
    PASSPORT_PAGE_3 = 2,
    PASSPORT_PAGE_COUNT = 3,
} PASSPORT_PAGE;

typedef enum PASSPORT_MODE {
    PASSPORT_MODE_FLIP = 0,
    PASSPORT_MODE_SHOW_SAVES = 1,
    PASSPORT_MODE_NEW_GAME = 2,
    PASSPORT_MODE_SELECT_LEVEL = 3,
    PASSPORT_MODE_STORY_SO_FAR = 4,
} PASSPORT_MODE;

#pragma pack(push, 1)

typedef struct RESOLUTION {
    int width;
    int height;
} RESOLUTION;

typedef struct RGBF {
    float r;
    float g;
    float b;
} RGBF;

typedef struct RGB888 {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RGB888;

typedef struct RGBA8888 {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} RGBA8888;

typedef struct POS_2D {
    uint16_t x;
    uint16_t y;
} POS_2D;

typedef struct POS_3D {
    uint16_t x;
    uint16_t y;
    uint16_t z;
} POS_3D;

typedef struct PHD_VECTOR {
    int32_t x;
    int32_t y;
    int32_t z;
} PHD_VECTOR;

typedef struct MATRIX {
    int32_t _00;
    int32_t _01;
    int32_t _02;
    int32_t _03;
    int32_t _10;
    int32_t _11;
    int32_t _12;
    int32_t _13;
    int32_t _20;
    int32_t _21;
    int32_t _22;
    int32_t _23;
} MATRIX;

typedef struct PHD_3DPOS {
    int32_t x;
    int32_t y;
    int32_t z;
    int16_t x_rot;
    int16_t y_rot;
    int16_t z_rot;
} PHD_3DPOS;

typedef struct POINT_INFO {
    float xv;
    float yv;
    float zv;
    float xs;
    float ys;
    float u;
    float v;
    float g;
} POINT_INFO;

typedef struct PHD_VBUF {
    float xv;
    float yv;
    float zv;
    float xs;
    float ys;
    int16_t clip;
    int16_t g;
    int16_t u;
    int16_t v;
} PHD_VBUF;

typedef struct PHD_UV {
    uint16_t u;
    uint16_t v;
} PHD_UV;

typedef struct PHD_TEXTURE {
    uint16_t drawtype;
    uint16_t tpage;
    PHD_UV uv[4];
} PHD_TEXTURE;

typedef struct PHD_SPRITE {
    uint16_t tpage;
    uint16_t offset;
    uint16_t width;
    uint16_t height;
    int16_t x1;
    int16_t y1;
    int16_t x2;
    int16_t y2;
} PHD_SPRITE;

typedef struct DOOR_INFO {
    int16_t room_num;
    int16_t x;
    int16_t y;
    int16_t z;
    POS_3D vertex[4];
} DOOR_INFO;

typedef struct DOOR_INFOS {
    uint16_t count;
    DOOR_INFO door[];
} DOOR_INFOS;

typedef struct FLOOR_INFO {
    uint16_t index;
    int16_t box;
    uint8_t pit_room;
    int8_t floor;
    uint8_t sky_room;
    int8_t ceiling;
} FLOOR_INFO;

typedef struct DOORPOS_DATA {
    FLOOR_INFO *floor;
    FLOOR_INFO old_floor;
    int16_t block;
} DOORPOS_DATA;

typedef struct DOOR_DATA {
    DOORPOS_DATA d1;
    DOORPOS_DATA d1flip;
    DOORPOS_DATA d2;
    DOORPOS_DATA d2flip;
} DOOR_DATA;

typedef struct LIGHT_INFO {
    int32_t x;
    int32_t y;
    int32_t z;
    int16_t intensity;
    int32_t falloff;
} LIGHT_INFO;

typedef struct MESH_INFO {
    uint32_t x;
    uint32_t y;
    uint32_t z;
    PHD_ANGLE y_rot;
    uint16_t shade;
    uint16_t static_number;
} MESH_INFO;

typedef struct ROOM_INFO {
    int16_t *data;
    DOOR_INFOS *doors;
    FLOOR_INFO *floor;
    LIGHT_INFO *light;
    MESH_INFO *mesh;
    int32_t x;
    int32_t y;
    int32_t z;
    int32_t min_floor;
    int32_t max_ceiling;
    int16_t x_size;
    int16_t y_size;
    int16_t ambient;
    int16_t num_lights;
    int16_t num_meshes;
    int16_t left;
    int16_t right;
    int16_t top;
    int16_t bottom;
    int16_t bound_active;
    int16_t item_number;
    int16_t fx_number;
    int16_t flipped_room;
    uint16_t flags;
} ROOM_INFO;

typedef struct ITEM_INFO {
    int32_t floor;
    uint32_t touch_bits;
    uint32_t mesh_bits;
    GAME_OBJECT_ID object_number;
    int16_t current_anim_state;
    int16_t goal_anim_state;
    int16_t required_anim_state;
    int16_t anim_number;
    int16_t frame_number;
    int16_t room_number;
    int16_t next_item;
    int16_t next_active;
    int16_t speed;
    int16_t fall_speed;
    int16_t hit_points;
    int16_t box_number;
    int16_t timer;
    int16_t flags;
    int16_t shade;
    void *data;
    PHD_3DPOS pos;
    uint16_t active : 1;
    uint16_t status : 2;
    uint16_t gravity_status : 1;
    uint16_t hit_status : 1;
    uint16_t collidable : 1;
    uint16_t looked_at : 1;
} ITEM_INFO;

typedef struct LARA_ARM {
    int16_t *frame_base;
    int16_t frame_number;
    int16_t lock;
    PHD_ANGLE y_rot;
    PHD_ANGLE x_rot;
    PHD_ANGLE z_rot;
    uint16_t flash_gun;
} LARA_ARM;

typedef struct AMMO_INFO {
    int32_t ammo;
    int32_t hit;
    int32_t miss;
} AMMO_INFO;

typedef struct BOX_NODE {
    int16_t exit_box;
    uint16_t search_number;
    int16_t next_expansion;
    int16_t box_number;
} BOX_NODE;

typedef struct LOT_INFO {
    BOX_NODE *node;
    int16_t head;
    int16_t tail;
    uint16_t search_number;
    uint16_t block_mask;
    int16_t step;
    int16_t drop;
    int16_t fly;
    int16_t zone_count;
    int16_t target_box;
    int16_t required_box;
    PHD_VECTOR target;
} LOT_INFO;

typedef struct FX_INFO {
    PHD_3DPOS pos;
    int16_t room_number;
    GAME_OBJECT_ID object_number;
    int16_t next_fx;
    int16_t next_active;
    int16_t speed;
    int16_t fall_speed;
    int16_t frame_number;
    int16_t counter;
    int16_t shade;
} FX_INFO;

typedef struct LARA_INFO {
    int16_t item_number;
    int16_t gun_status;
    int16_t gun_type;
    int16_t request_gun_type;
    int16_t calc_fall_speed;
    int16_t water_status;
    int16_t pose_count;
    int16_t hit_frame;
    int16_t hit_direction;
    int16_t air;
    int16_t dive_timer;
    int16_t death_timer;
    int16_t current_active;
    int16_t spaz_effect_count;
    FX_INFO *spaz_effect;
    int32_t mesh_effects;
    int16_t *mesh_ptrs[LM_NUMBER_OF];
    ITEM_INFO *target;
    PHD_ANGLE target_angles[2];
    int16_t turn_rate;
    int16_t move_angle;
    int16_t head_y_rot;
    int16_t head_x_rot;
    int16_t head_z_rot;
    int16_t torso_y_rot;
    int16_t torso_x_rot;
    int16_t torso_z_rot;
    LARA_ARM left_arm;
    LARA_ARM right_arm;
    AMMO_INFO pistols;
    AMMO_INFO magnums;
    AMMO_INFO uzis;
    AMMO_INFO shotgun;
    LOT_INFO LOT;
    struct {
        int32_t item_num;
        int32_t move_count;
        bool is_moving;
    } interact_target;
} LARA_INFO;

typedef struct GAME_STATS {
    uint32_t timer;
    uint32_t death_count;
    uint32_t kill_count;
    uint16_t secret_flags;
    uint8_t pickup_count;
    uint32_t max_kill_count;
    uint16_t max_secret_count;
    uint8_t max_pickup_count;
} GAME_STATS;

typedef struct RESUME_INFO {
    int32_t lara_hitpoints;
    uint16_t pistol_ammo;
    uint16_t magnum_ammo;
    uint16_t uzi_ammo;
    uint16_t shotgun_ammo;
    uint8_t num_medis;
    uint8_t num_big_medis;
    uint8_t num_scions;
    int8_t gun_status;
    int8_t gun_type;
    union {
        uint16_t all;
        struct {
            uint16_t available : 1;
            uint16_t got_pistols : 1;
            uint16_t got_magnums : 1;
            uint16_t got_uzis : 1;
            uint16_t got_shotgun : 1;
            uint16_t costume : 1;
        };
    } flags;
    GAME_STATS stats;
} RESUME_INFO;

typedef struct GAME_INFO {
    RESUME_INFO *current;
    uint8_t bonus_flag;
    int32_t current_save_slot;
    int16_t save_initial_version;
    PASSPORT_PAGE passport_page;
    PASSPORT_MODE passport_mode;
    int32_t select_level_num;
    bool death_counter_supported;
    GAMEFLOW_LEVEL_TYPE current_level_type;
    bool remove_guns;
    bool remove_scions;
    bool remove_ammo;
    bool remove_medipacks;
} GAME_INFO;

typedef struct CREATURE_INFO {
    int16_t head_rotation;
    int16_t neck_rotation;
    int16_t maximum_turn;
    uint16_t flags;
    int16_t item_num;
    MOOD_TYPE mood;
    LOT_INFO LOT;
    PHD_VECTOR target;
} CREATURE_INFO;

typedef enum {
    TS_HEADING = 0,
    TS_BACKGROUND = 1,
    TS_REQUESTED = 2,
} TEXT_STYLE;

typedef enum {
    MC_PURPLE_C,
    MC_PURPLE_E,
    MC_BROWN_C,
    MC_BROWN_E,
    MC_GREY_C,
    MC_GREY_E,
    MC_GREY_TL,
    MC_GREY_TR,
    MC_GREY_BL,
    MC_GREY_BR,
    MC_BLACK,
    MC_GOLD_LIGHT,
    MC_GOLD_DARK,
    MC_NUMBER_OF,
} MENU_COLOR;

typedef struct TEXTSTRING {
    union {
        uint32_t all;
        struct {
            uint32_t active : 1;
            uint32_t flash : 1;
            uint32_t rotate_h : 1;
            uint32_t centre_h : 1;
            uint32_t centre_v : 1;
            uint32_t right : 1;
            uint32_t bottom : 1;
            uint32_t background : 1;
            uint32_t outline : 1;
            uint32_t hide : 1;
        };
    } flags;
    struct {
        int16_t x;
        int16_t y;
    } pos;
    int16_t letter_spacing;
    int16_t word_spacing;
    struct {
        int16_t rate;
        int16_t count;
    } flash;
    struct {
        int16_t x;
        int16_t y;
    } bgnd_size;
    struct {
        int16_t x;
        int16_t y;
    } bgnd_off;
    struct {
        int32_t h;
        int32_t v;
    } scale;
    struct {
        TEXT_STYLE style;
    } background;
    struct {
        TEXT_STYLE style;
    } outline;
    char *string;

    void (*on_remove)(const struct TEXTSTRING *);
} TEXTSTRING;

typedef struct DISPLAYPU {
    int16_t duration;
    int16_t sprnum;
} DISPLAYPU;

typedef struct COLL_INFO {
    int32_t mid_floor;
    int32_t mid_ceiling;
    int32_t mid_type;
    int32_t front_floor;
    int32_t front_ceiling;
    int32_t front_type;
    int32_t left_floor;
    int32_t left_ceiling;
    int32_t left_type;
    int32_t right_floor;
    int32_t right_ceiling;
    int32_t right_type;
    int32_t radius;
    int32_t bad_pos;
    int32_t bad_neg;
    int32_t bad_ceiling;
    PHD_VECTOR shift;
    PHD_VECTOR old;
    int16_t facing;
    int16_t quadrant;
    int16_t coll_type;
    int16_t *trigger;
    int8_t tilt_x;
    int8_t tilt_z;
    int8_t hit_by_baddie;
    int8_t hit_static;
    uint16_t slopes_are_walls : 1;
    uint16_t slopes_are_pits : 1;
    uint16_t lava_is_pit : 1;
    uint16_t enable_baddie_push : 1;
    uint16_t enable_spaz : 1;
} COLL_INFO;

typedef struct OBJECT_INFO {
    int16_t nmeshes;
    int16_t mesh_index;
    int32_t bone_index;
    int16_t *frame_base;
    void (*initialise)(int16_t item_num);
    void (*control)(int16_t item_num);
    void (*floor)(
        ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);
    void (*ceiling)(
        ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);
    void (*draw_routine)(ITEM_INFO *item);
    void (*collision)(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
    int16_t anim_index;
    int16_t hit_points;
    int16_t pivot_length;
    int16_t radius;
    int16_t smartness;
    int16_t shadow_size;
    uint16_t loaded : 1;
    uint16_t intelligent : 1;
    uint16_t save_position : 1;
    uint16_t save_hitpoints : 1;
    uint16_t save_flags : 1;
    uint16_t save_anim : 1;
} OBJECT_INFO;

typedef struct SHADOW_INFO {
    int16_t x;
    int16_t y;
    int16_t z;
    int16_t radius;
    int16_t poly_count;
    int16_t vertex_count;
    POS_3D vertex[32];
} SHADOW_INFO;

typedef struct STATIC_INFO {
    int16_t mesh_number;
    int16_t flags;
    int16_t x_minp;
    int16_t x_maxp;
    int16_t y_minp;
    int16_t y_maxp;
    int16_t z_minp;
    int16_t z_maxp;
    int16_t x_minc;
    int16_t x_maxc;
    int16_t y_minc;
    int16_t y_maxc;
    int16_t z_minc;
    int16_t z_maxc;
} STATIC_INFO;

typedef struct GAME_VECTOR {
    int32_t x;
    int32_t y;
    int32_t z;
    int16_t room_number;
    int16_t box_number;
} GAME_VECTOR;

typedef struct OBJECT_VECTOR {
    int32_t x;
    int32_t y;
    int32_t z;
    int16_t data;
    int16_t flags;
} OBJECT_VECTOR;

typedef struct CAMERA_INFO {
    GAME_VECTOR pos;
    GAME_VECTOR target;
    int32_t type;
    int32_t shift;
    int32_t flags;
    int32_t fixed_camera;
    int32_t number_frames;
    int32_t bounce;
    int32_t underwater;
    int32_t target_distance;
    int32_t target_square;
    int16_t target_angle;
    int16_t actual_angle;
    int16_t target_elevation;
    int16_t box;
    int16_t number;
    int16_t last;
    int16_t timer;
    int16_t speed;
    ITEM_INFO *item;
    ITEM_INFO *last_item;
    OBJECT_VECTOR *fixed;
    // used for the manual camera control
    int16_t additional_angle;
    int16_t additional_elevation;
} CAMERA_INFO;

typedef struct ANIM_STRUCT {
    int16_t *frame_ptr;
    int16_t interpolation;
    int16_t current_anim_state;
    int32_t velocity;
    int32_t acceleration;
    int16_t frame_base;
    int16_t frame_end;
    int16_t jump_anim_num;
    int16_t jump_frame_num;
    int16_t number_changes;
    int16_t change_index;
    int16_t number_commands;
    int16_t command_index;
} ANIM_STRUCT;

typedef struct ANIM_CHANGE_STRUCT {
    int16_t goal_anim_state;
    int16_t number_ranges;
    int16_t range_index;
} ANIM_CHANGE_STRUCT;

typedef struct ANIM_RANGE_STRUCT {
    int16_t start_frame;
    int16_t end_frame;
    int16_t link_anim_num;
    int16_t link_frame_num;
} ANIM_RANGE_STRUCT;

typedef struct DOOR_VBUF {
    int32_t xv;
    int32_t yv;
    int32_t zv;
} DOOR_VBUF;

typedef struct WEAPON_INFO {
    PHD_ANGLE lock_angles[4];
    PHD_ANGLE left_angles[4];
    PHD_ANGLE right_angles[4];
    PHD_ANGLE aim_speed;
    PHD_ANGLE shot_accuracy;
    int32_t gun_height;
    int32_t damage;
    int32_t target_dist;
    int16_t recoil_frame;
    int16_t flash_time;
    int16_t sample_num;
} WEAPON_INFO;

typedef struct SPHERE {
    int32_t x;
    int32_t y;
    int32_t z;
    int32_t r;
} SPHERE;

typedef struct BITE_INFO {
    int32_t x;
    int32_t y;
    int32_t z;
    int32_t mesh_num;
} BITE_INFO;

typedef struct AI_INFO {
    int16_t zone_number;
    int16_t enemy_zone;
    int32_t distance;
    int32_t ahead;
    int32_t bite;
    int16_t angle;
    int16_t enemy_facing;
} AI_INFO;

typedef struct BOX_INFO {
    int32_t left;
    int32_t right;
    int32_t top;
    int32_t bottom;
    int16_t height;
    int16_t overlap_index;
} BOX_INFO;

typedef struct REQUEST_INFO {
    uint16_t items;
    uint16_t requested;
    uint16_t vis_lines;
    uint16_t line_offset;
    uint16_t line_old_offset;
    uint16_t pix_width;
    uint16_t line_height;
    int16_t x;
    int16_t y;
    uint16_t flags;
    const char *heading_text;
    char *item_texts;
    int16_t item_text_len;
    TEXTSTRING *heading;
    TEXTSTRING *background;
    TEXTSTRING *moreup;
    TEXTSTRING *moredown;
    TEXTSTRING *texts[MAX_REQLINES];
    uint16_t item_flags[MAX_REQLINES];
} REQUEST_INFO;

typedef struct IMOTION_INFO {
    int16_t count;
    int16_t status;
    int16_t status_target;
    int16_t radius_target;
    int16_t radius_rate;
    int16_t camera_ytarget;
    int16_t camera_yrate;
    int16_t camera_pitch_target;
    int16_t camera_pitch_rate;
    int16_t rotate_target;
    int16_t rotate_rate;
    PHD_ANGLE item_ptxrot_target;
    PHD_ANGLE item_ptxrot_rate;
    PHD_ANGLE item_xrot_target;
    PHD_ANGLE item_xrot_rate;
    int32_t item_ytrans_target;
    int32_t item_ytrans_rate;
    int32_t item_ztrans_target;
    int32_t item_ztrans_rate;
    int32_t misc;
} IMOTION_INFO;

typedef struct INVENTORY_SPRITE {
    int16_t shape;
    int16_t x;
    int16_t y;
    int16_t z;
    int32_t param1;
    int32_t param2;
    int16_t sprnum;
} INVENTORY_SPRITE;

typedef struct INVENTORY_ITEM {
    char *string;
    GAME_OBJECT_ID object_number;
    int16_t frames_total;
    int16_t current_frame;
    int16_t goal_frame;
    int16_t open_frame;
    int16_t anim_direction;
    int16_t anim_speed;
    int16_t anim_count;
    PHD_ANGLE pt_xrot_sel;
    PHD_ANGLE pt_xrot;
    PHD_ANGLE x_rot_sel;
    PHD_ANGLE x_rot;
    PHD_ANGLE y_rot_sel;
    PHD_ANGLE y_rot;
    int32_t ytrans_sel;
    int32_t ytrans;
    int32_t ztrans_sel;
    int32_t ztrans;
    uint32_t which_meshes;
    uint32_t drawn_meshes;
    int16_t inv_pos;
    INVENTORY_SPRITE **sprlist;
} INVENTORY_ITEM;

typedef struct RING_INFO {
    INVENTORY_ITEM **list;
    int16_t type;
    int16_t radius;
    int16_t camera_pitch;
    int16_t rotating;
    int16_t rot_count;
    int16_t current_object;
    int16_t target_object;
    int16_t number_of_objects;
    int16_t angle_adder;
    int16_t rot_adder;
    int16_t rot_adder_l;
    int16_t rot_adder_r;
    PHD_3DPOS ringpos;
    PHD_3DPOS camera;
    PHD_VECTOR light;
    IMOTION_INFO *imo;
} RING_INFO;

typedef struct SAMPLE_INFO {
    int16_t number;
    int16_t volume;
    int16_t randomness;
    int16_t flags;
} SAMPLE_INFO;

#pragma pack(pop)

typedef struct PICTURE {
    int32_t width;
    int32_t height;
    RGB888 *data;
} PICTURE;

typedef union INPUT_STATE {
    uint64_t any;
    struct {
        uint64_t forward : 1;
        uint64_t back : 1;
        uint64_t left : 1;
        uint64_t right : 1;
        uint64_t jump : 1;
        uint64_t draw : 1;
        uint64_t action : 1;
        uint64_t slow : 1;
        uint64_t option : 1;
        uint64_t look : 1;
        uint64_t step_left : 1;
        uint64_t step_right : 1;
        uint64_t roll : 1;
        uint64_t pause : 1;
        uint64_t select : 1;
        uint64_t deselect : 1;
        uint64_t save : 1;
        uint64_t load : 1;
        uint64_t fly_cheat : 1;
        uint64_t item_cheat : 1;
        uint64_t level_skip_cheat : 1;
        uint64_t turbo_cheat : 1;
        uint64_t health_cheat : 1;
        uint64_t camera_up : 1;
        uint64_t camera_down : 1;
        uint64_t camera_left : 1;
        uint64_t camera_right : 1;
        uint64_t camera_reset : 1;
        uint64_t equip_pistols : 1;
        uint64_t equip_shotgun : 1;
        uint64_t equip_magnums : 1;
        uint64_t equip_uzis : 1;
        uint64_t use_small_medi : 1;
        uint64_t use_big_medi : 1;
        uint64_t toggle_bilinear_filter : 1;
        uint64_t toggle_perspective_filter : 1;
        uint64_t toggle_fps_counter : 1;
    };
} INPUT_STATE;

typedef enum INPUT_ROLE {
    INPUT_ROLE_UP = 0,
    INPUT_ROLE_DOWN = 1,
    INPUT_ROLE_LEFT = 2,
    INPUT_ROLE_RIGHT = 3,
    INPUT_ROLE_STEP_L = 4,
    INPUT_ROLE_STEP_R = 5,
    INPUT_ROLE_SLOW = 6,
    INPUT_ROLE_JUMP = 7,
    INPUT_ROLE_ACTION = 8,
    INPUT_ROLE_DRAW = 9,
    INPUT_ROLE_LOOK = 10,
    INPUT_ROLE_ROLL = 11,
    INPUT_ROLE_OPTION = 12,
    INPUT_ROLE_FLY_CHEAT = 13,
    INPUT_ROLE_ITEM_CHEAT = 14,
    INPUT_ROLE_LEVEL_SKIP_CHEAT = 15,
    INPUT_ROLE_TURBO_CHEAT = 16,
    INPUT_ROLE_PAUSE = 17,
    INPUT_ROLE_CAMERA_UP = 18,
    INPUT_ROLE_CAMERA_DOWN = 19,
    INPUT_ROLE_CAMERA_LEFT = 20,
    INPUT_ROLE_CAMERA_RIGHT = 21,
    INPUT_ROLE_CAMERA_RESET = 22,
    INPUT_ROLE_EQUIP_PISTOLS = 23,
    INPUT_ROLE_EQUIP_SHOTGUN = 24,
    INPUT_ROLE_EQUIP_MAGNUMS = 25,
    INPUT_ROLE_EQUIP_UZIS = 26,
    INPUT_ROLE_USE_SMALL_MEDI = 27,
    INPUT_ROLE_USE_BIG_MEDI = 28,
    INPUT_ROLE_SAVE = 29,
    INPUT_ROLE_LOAD = 30,
    INPUT_ROLE_FPS = 31,
    INPUT_ROLE_BILINEAR = 32,
    INPUT_ROLE_NUMBER_OF = 33,
} INPUT_ROLE;

typedef enum INPUT_LAYOUT {
    INPUT_LAYOUT_DEFAULT,
    INPUT_LAYOUT_CUSTOM_1,
    INPUT_LAYOUT_CUSTOM_2,
    INPUT_LAYOUT_CUSTOM_3,
    INPUT_LAYOUT_NUMBER_OF,
} INPUT_LAYOUT;

typedef int16_t INPUT_SCANCODE;
