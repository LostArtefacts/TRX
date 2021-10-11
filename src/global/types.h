#ifndef T1M_GLOBAL_TYPES_H
#define T1M_GLOBAL_TYPES_H

#include "global/const.h"

#include <stdint.h>

typedef int16_t PHD_ANGLE;
typedef uint32_t SG_COL;

typedef enum CAMERA_TYPE {
    CAM_CHASE = 0,
    CAM_FIXED = 1,
    CAM_LOOK = 2,
    CAM_COMBAT = 3,
    CAM_CINEMATIC = 4,
    CAM_HEAVY = 5,
} CAMERA_TYPE;

typedef enum GAME_OBJECT_ID {
    O_LARA = 0,
    O_PISTOLS = 1,
    O_SHOTGUN = 2,
    O_MAGNUM = 3,
    O_UZI = 4,
    O_LARA_EXTRA = 5,
    O_EVIL_LARA = 6,
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
    O_DINOSAUR = 18,
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
    O_MERCENARY1 = 30,
    O_MERCENARY2 = 31,
    O_MERCENARY3 = 32,
    O_NATLA = 33,
    O_ABORTION = 34, // a.k.a. Adam, Torso or Evil Natla
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
    SFX_ABORTION_HIT_GROUND = 136,
    SFX_ABORTION_ATTACK1 = 137,
    SFX_ABORTION_ATTACK2 = 138,
    SFX_ABORTION_DEATH = 139,
    SFX_ABORTION_ARM_SWING = 140,
    SFX_ABORTION_MOVE = 141,
    SFX_ABORTION_HIT = 142,
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
    AF_GRABLEDGE_OLD = 621,		//not used
    AF_SWIMGLIDE = 1431,		//not used
    AF_FALLBACK = 1473,
    AF_HANG = 1514,
    AF_HANG_OLD = 642,			//not used
    AF_STARTHANG = 1505,
    AF_STARTHANG_OLD = 634,		//not used
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
    AF_PICKUP = 3443,
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
    AF_G_DRAW1_L = 12,	//not used
    AF_G_DRAW2 = 13,
    AF_G_DRAW2_L = 23,
    AF_G_RECOIL = 24,
} LARA_GUN_ANIMATION_FRAME;

typedef enum LARA_ANIMATION {
    AA_VAULT12 = 50,
    AA_VAULT34 = 42,
    AA_FASTFALL = 32,
    AA_STOP = 11,
    AA_FALLDOWN = 34,
    AA_STOP_LEFT = 2,
    AA_STOP_RIGHT = 3,
    AA_HITWALLLEFT = 53,
    AA_HITWALLRIGHT = 54,
    AA_RUNSTEPUP_LEFT = 56,
    AA_RUNSTEPUP_RIGHT = 55,
    AA_WALKSTEPUP_LEFT = 57,
    AA_WALKSTEPUP_RIGHT = 58,
    AA_WALKSTEPD_LEFT = 60,
    AA_WALKSTEPD_RIGHT = 59,
    AA_BACKSTEPD_LEFT = 61,
    AA_BACKSTEPD_RIGHT = 62,
    AA_LANDFAR = 24,
    AA_GRABLEDGE = 96,
    AA_GRABLEDGE_OLD = 32,
    AA_SWIMGLIDE = 87,
    AA_FALLBACK = 93,
    AA_HANG = 96,
    AA_HANG_OLD = 33,
    AA_STARTHANG = 96,
    AA_STARTHANG_OLD = 33,
    AA_STOPHANG = 28,
    AA_SLIDE = 70,
    AA_SLIDEBACK = 104,
    AA_TREAD = 108,
    AA_SURFTREAD = 114,
    AA_SURFDIVE = 119,
    AA_SURFCLIMB = 111,
    AA_JUMPIN = 112,
    AA_ROLL = 146,
    AA_RBALL_DEATH = 139,
    AA_SPIKE_DEATH = 149,
    AA_GRABLEDGEIN = 150,
    AA_SPAZ_FORWARD = 125,
    AA_SPAZ_BACK = 126,
    AA_SPAZ_RIGHT = 127,
    AA_SPAZ_LEFT = 128,
} LARA_ANIMATION;

typedef enum LARA_WATER_STATUS {
    LWS_ABOVEWATER = 0,
    LWS_UNDERWATER = 1,
    LWS_SURFACE = 2,
    LWS_CHEAT = 3,
} LARA_WATER_STATUS;

typedef enum LARA_STATE {
    AS_WALK = 0,
    AS_RUN = 1,
    AS_STOP = 2,
    AS_FORWARDJUMP = 3,
    AS_POSE = 4,
    AS_FASTBACK = 5,
    AS_TURN_R = 6,
    AS_TURN_L = 7,
    AS_DEATH = 8,
    AS_FASTFALL = 9,
    AS_HANG = 10,
    AS_REACH = 11,
    AS_SPLAT = 12,
    AS_TREAD = 13,
    AS_LAND = 14,
    AS_COMPRESS = 15,
    AS_BACK = 16,
    AS_SWIM = 17,
    AS_GLIDE = 18,
    AS_NULL = 19,
    AS_FASTTURN = 20,
    AS_STEPRIGHT = 21,
    AS_STEPLEFT = 22,
    AS_HIT = 23,
    AS_SLIDE = 24,
    AS_BACKJUMP = 25,
    AS_RIGHTJUMP = 26,
    AS_LEFTJUMP = 27,
    AS_UPJUMP = 28,
    AS_FALLBACK = 29,
    AS_HANGLEFT = 30,
    AS_HANGRIGHT = 31,
    AS_SLIDEBACK = 32,
    AS_SURFTREAD = 33,
    AS_SURFSWIM = 34,
    AS_DIVE = 35,
    AS_PUSHBLOCK = 36,
    AS_PULLBLOCK = 37,
    AS_PPREADY = 38,
    AS_PICKUP = 39,
    AS_SWITCHON = 40,
    AS_SWITCHOFF = 41,
    AS_USEKEY = 42,
    AS_USEPUZZLE = 43,
    AS_UWDEATH = 44,
    AS_ROLL = 45,
    AS_SPECIAL = 46,
    AS_SURFBACK = 47,
    AS_SURFLEFT = 48,
    AS_SURFRIGHT = 49,
    AS_USEMIDAS = 50,
    AS_DIEMIDAS = 51,
    AS_SWANDIVE = 52,
    AS_FASTDIVE = 53,
    AS_GYMNAST = 54,
    AS_WATEROUT = 55,
} LARA_STATE;

typedef enum LARA_GUN_STATE {
    LGS_ARMLESS = 0,
    LGS_HANDSBUSY = 1,
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

typedef enum GAMEALLOC_BUFFER {
    GBUF_TEXTURE_PAGES = 0,
    GBUF_OBJECT_TEXTURES = 1,
    GBUF_MESH_POINTERS = 2,
    GBUF_MESHES = 3,
    GBUF_ANIMS = 4,
    GBUF_ANIM_CHANGES = 5,
    GBUF_ANIM_RANGES = 6,
    GBUF_ANIM_COMMANDS = 7,
    GBUF_ANIM_BONES = 8,
    GBUF_ANIM_FRAMES = 9,
    GBUF_ROOM_TEXTURES = 10,
    GBUF_ROOM_INFOS = 11,
    GBUF_ROOM_MESH = 12,
    GBUF_ROOM_DOOR = 13,
    GBUF_ROOM_FLOOR = 14,
    GBUF_ROOM_LIGHTS = 15,
    GBUF_ROOM_STATIC_MESH_INFOS = 16,
    GBUF_FLOOR_DATA = 17,
    GBUF_ITEMS = 18,
    GBUF_CAMERAS = 19,
    GBUF_SOUND_FX = 20,
    GBUF_BOXES = 21,
    GBUF_OVERLAPS = 22,
    GBUF_GROUNDZONE = 23,
    GBUF_FLYZONE = 24,
    GBUF_ANIMATING_TEXTURE_RANGES = 25,
    GBUF_CINEMATIC_FRAMES = 26,
    GBUF_LOADDEMO_BUFFER = 27,
    GBUF_SAVEDEMO_BUFFER = 28,
    GBUF_CINEMATIC_EFFECTS = 29,
    GBUF_MUMMY_HEAD_TURN = 30,
    GBUF_EXTRA_DOOR_STUFF = 31,
    GBUF_EFFECTS = 32,
    GBUF_CREATURE_DATA = 33,
    GBUF_CREATURE_LOT = 34,
    GBUF_SAMPLE_INFOS = 35,
    GBUF_SAMPLES = 36,
    GBUF_SAMPLE_OFFSETS = 37,
    GBUF_ROLLINGBALL_STUFF = 38,
} GAMEALLOC_BUFFER;

typedef enum KEY_NUMBER {
    KEY_UP = 0,
    KEY_DOWN = 1,
    KEY_LEFT = 2,
    KEY_RIGHT = 3,
    KEY_STEP_L = 4,
    KEY_STEP_R = 5,
    KEY_SLOW = 6,
    KEY_JUMP = 7,
    KEY_ACTION = 8,
    KEY_DRAW = 9,
    KEY_LOOK = 10,
    KEY_ROLL = 11,
    KEY_OPTION = 12,
    KEY_FLY_CHEAT = 13,
    KEY_ITEM_CHEAT = 14,
    KEY_LEVEL_SKIP_CHEAT = 15,
    KEY_PAUSE = 16,
    KEY_NUMBER_OF = 17,
} KEY_NUMBER;

typedef enum INPUT_STATE {
    IN_FORWARD = 1 << 0,
    IN_BACK = 1 << 1,
    IN_LEFT = 1 << 2,
    IN_RIGHT = 1 << 3,
    IN_JUMP = 1 << 4,
    IN_DRAW = 1 << 5,
    IN_ACTION = 1 << 6,
    IN_SLOW = 1 << 7,
    IN_OPTION = 1 << 8,
    IN_LOOK = 1 << 9,
    IN_STEPL = 1 << 10,
    IN_STEPR = 1 << 11,
    IN_ROLL = 1 << 12,
    IN_PAUSE = 1 << 13,
    IN_A = 1 << 14,
    IN_B = 1 << 15,
    IN_C = 1 << 16,
    IN_MENUBACK = 1 << 17,
    IN_UP = 1 << 18,
    IN_DOWN = 1 << 19,
    IN_SELECT = 1 << 20,
    IN_DESELECT = 1 << 21,
    IN_SAVE = 1 << 22,
    IN_LOAD = 1 << 23,
    IN_FLY_CHEAT = 1 << 24,
    IN_ITEM_CHEAT = 1 << 25,
} INPUT_STATE;

typedef enum TEXTSTRING_FLAG {
    TF_ACTIVE = 1 << 0,
    TF_FLASH = 1 << 1,
    TF_ROTATE_H = 1 << 2,
    TF_ROTATE_V = 1 << 3,
    TF_CENTRE_H = 1 << 4,
    TF_CENTRE_V = 1 << 5,
    TF_RIGHT = 1 << 7,
    TF_BOTTOM = 1 << 8,
    TF_BGND = 1 << 9,
    TF_OUTLINE = 1 << 10,
} TEXTSTRING_FLAG;

typedef enum D_FLAGS {
    D_TRANS1 = 1,
    D_TRANS2 = 2,
    D_TRANS3 = 3,
    D_TRANS4 = 4,
    D_NEXT = 1 << 3,
} D_FLAGS;

typedef enum APP_SETTINGS_FLAG {
    ASF_PERSPECTIVE = 1 << 0,
    ASF_BILINEAR = 1 << 1,
    ASF_FPS = 1 << 2,
    ASF_60FPS_SHIFT = 3,
    ASF_60FPS = 1 << ASF_60FPS_SHIFT,
} APP_SETTINGS_FLAG;

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

typedef enum FMV_SEQUENCE {
    FMV_INTRO = 0,
    FMV_GYM = 1,
    FMV_SNOW = 2,
    FMV_LIFT = 3,
    FMV_VISION = 4,
    FMV_CANYON = 5,
    FMV_PYRAMID = 6,
    FMV_PRISON = 7,
    FMV_ENDSEQ = 8,
    FMV_CORE = 9,
    FMV_ESCAPE = 10,
    FMV_NUMBER_OF = 11,
} FMV_SEQUENCE;

typedef enum INV_MODE {
    INV_GAME_MODE = 0,
    INV_TITLE_MODE = 1,
    INV_KEYS_MODE = 2,
    INV_SAVE_MODE = 3,
    INV_LOAD_MODE = 4,
    INV_DEATH_MODE = 5,
    INV_SAVE_CRYSTAL_MODE = 6,
    INV_PAUSE_MODE = 7,
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
    GFS_PLAY_SYNCED_AUDIO,
    GFS_MESH_SWAP,
    GFS_FIX_PYRAMID_SECRET_TRIGGER,
} GAMEFLOW_SEQUENCE_TYPE;

typedef enum GAME_STRING_ID {
    GS_HEADING_INVENTORY,
    GS_HEADING_GAME_OVER,
    GS_HEADING_OPTION,
    GS_HEADING_ITEMS,

    GS_PASSPORT_SELECT_LEVEL,
    GS_PASSPORT_SELECT_MODE,
    GS_PASSPORT_MODE_NEW_GAME,
    GS_PASSPORT_MODE_NEW_GAME_PLUS,
    GS_PASSPORT_MODE_JAPANESE_NEW_GAME,
    GS_PASSPORT_MODE_JAPANESE_NEW_GAME_PLUS,
    GS_PASSPORT_NEW_GAME,
    GS_PASSPORT_LOAD_GAME,
    GS_PASSPORT_SAVE_GAME,
    GS_PASSPORT_EXIT_GAME,
    GS_PASSPORT_EXIT_TO_TITLE,

    GS_DETAIL_SELECT_DETAIL,
    GS_DETAIL_LEVEL_HIGH,
    GS_DETAIL_LEVEL_MEDIUM,
    GS_DETAIL_LEVEL_LOW,
    GS_DETAIL_PERSPECTIVE_FMT,
    GS_DETAIL_BILINEAR_FMT,
    GS_DETAIL_UI_TEXT_SCALE_FMT,
    GS_DETAIL_UI_BAR_SCALE_FMT,
    GS_DETAIL_VIDEO_MODE_FMT,
    GS_DETAIL_VIDEO_FPS_RATE,

    GS_SOUND_SET_VOLUMES,

    GS_CONTROL_DEFAULT_KEYS,
    GS_CONTROL_USER_KEYS,

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
    GS_KEYMAP_PAUSE,

    GS_STATS_TIME_TAKEN_FMT,
    GS_STATS_SECRETS_FMT,
    GS_STATS_PICKUPS_FMT,
    GS_STATS_KILLS_FMT,

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

#pragma pack(push, 1)

typedef struct RGB888 {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RGB888;

typedef struct PcxHeader_t {
    uint8_t manufacturer;
    uint8_t version;
    uint8_t rle;
    uint8_t bpp;
    uint16_t x_min;
    uint16_t y_min;
    uint16_t x_max;
    uint16_t y_max;
    uint16_t h_dpi;
    uint16_t v_dpi;
    RGB888 header_palette[16];
    uint8_t reserved;
    uint8_t planes;
    uint16_t bytes_per_line;
    uint16_t pal_interpret;
    uint16_t h_res;
    uint16_t v_res;
    uint8_t reserved_data[54];
} PCX_HEADER;

typedef struct POS_2D {
    /* 0000 */ uint16_t x;
    /* 0002 */ uint16_t y;
    /* 0004 end */
} POS_2D;

typedef struct POS_3D {
    /* 0000 */ uint16_t x;
    /* 0002 */ uint16_t y;
    /* 0004 */ uint16_t z;
    /* 0006 end */
} POS_3D;

typedef struct PHD_VECTOR {
    /* 0000 */ int32_t x;
    /* 0004 */ int32_t y;
    /* 0008 */ int32_t z;
    /* 000C end */
} PHD_VECTOR;

typedef struct PHD_MATRIX {
    /* 0000 */ int32_t _00;
    /* 0004 */ int32_t _01;
    /* 0008 */ int32_t _02;
    /* 000C */ int32_t _03;
    /* 0010 */ int32_t _10;
    /* 0014 */ int32_t _11;
    /* 0018 */ int32_t _12;
    /* 001C */ int32_t _13;
    /* 0020 */ int32_t _20;
    /* 0024 */ int32_t _21;
    /* 0028 */ int32_t _22;
    /* 002C */ int32_t _23;
    /* 0030 end */
} PHD_MATRIX;

typedef struct PHD_3DPOS {
    /* 0000 */ int32_t x;
    /* 0004 */ int32_t y;
    /* 0008 */ int32_t z;
    /* 000C */ int16_t x_rot;
    /* 000E */ int16_t y_rot;
    /* 0010 */ int16_t z_rot;
    /* 0012 end */
} PHD_3DPOS;

typedef struct PHD_3DPOS_F {
    /* 0000 */ double x;
    /* 0004 */ double y;
    /* 0008 */ double z;
    /* 000C */ double x_rot;
    /* 000E */ double y_rot;
    /* 0010 */ double z_rot;
    /* 0012 end */
} PHD_3DPOS_F;

typedef struct PHD_VBUF {
    /* 0000 */ int32_t xv;
    /* 0004 */ int32_t yv;
    /* 0008 */ int32_t zv;
    /* 000C */ int32_t xs;
    /* 0010 */ int32_t ys;
    /* 0014 */ int32_t dist;
    /* 0018 */ int16_t clip;
    /* 001A */ int16_t g;
    /* 001C */ uint16_t u;
    /* 001E */ uint16_t v;
    /* 0020 end */
} PHD_VBUF;

typedef struct PHD_TEXTURE {
    /* 0000 */ uint16_t drawtype;
    /* 0002 */ uint16_t tpage;
    /* 0004 */ uint16_t u1;
    /* 0006 */ uint16_t v1;
    /* 0008 */ uint16_t u2;
    /* 000A */ uint16_t v2;
    /* 000C */ uint16_t u3;
    /* 000E */ uint16_t v3;
    /* 0010 */ uint16_t u4;
    /* 0012 */ uint16_t v4;
    /* 0014 end */
} PHD_TEXTURE;

typedef struct PHD_SPRITE {
    /* 0000 */ uint16_t tpage;
    /* 0002 */ uint16_t offset;
    /* 0004 */ uint16_t width;
    /* 0006 */ uint16_t height;
    /* 0008 */ int16_t x1;
    /* 000A */ int16_t y1;
    /* 000C */ int16_t x2;
    /* 000E */ int16_t y2;
    /* 0010 end */
} PHD_SPRITE;

typedef struct DOOR_INFO {
    /* 0000 */ int16_t room_num;
    /* 0002 */ int16_t x;
    /* 0004 */ int16_t y;
    /* 0006 */ int16_t z;
    /* 0008 */ POS_3D vertex[4];
    /* 0020 end */
} DOOR_INFO;

typedef struct DOOR_INFOS {
    /* 0000 */ uint16_t count;
    /* 0002 */ DOOR_INFO door[];
    /* 0006 end */
} DOOR_INFOS;

typedef struct FLOOR_INFO {
    /* 0000 */ uint16_t index;
    /* 0002 */ int16_t box;
    /* 0004 */ uint8_t pit_room;
    /* 0005 */ int8_t floor;
    /* 0006 */ uint8_t sky_room;
    /* 0007 */ int8_t ceiling;
    /* 0008 end */
} FLOOR_INFO;

typedef struct DOORPOS_DATA {
    /* 0000 */ FLOOR_INFO *floor;
    /* 0004 */ FLOOR_INFO old_floor;
    /* 000C */ int16_t block;
    /* 000E end */
} DOORPOS_DATA;

typedef struct DOOR_DATA {
    /* 0000 */ DOORPOS_DATA d1;
    /* 000E */ DOORPOS_DATA d1flip;
    /* 001C */ DOORPOS_DATA d2;
    /* 002A */ DOORPOS_DATA d2flip;
    /* 0038 end */
} DOOR_DATA;

typedef struct LIGHT_INFO {
    /* 0000 */ int32_t x;
    /* 0004 */ int32_t y;
    /* 0008 */ int32_t z;
    /* 000C */ int16_t intensity;
    /* 000E */ int32_t falloff;
    /* 0012 end */
} LIGHT_INFO;

typedef struct MESH_INFO {
    /* 0000 */ uint32_t x;
    /* 0004 */ uint32_t y;
    /* 0008 */ uint32_t z;
    /* 000C */ PHD_ANGLE y_rot;
    /* 000E */ uint16_t shade;
    /* 0010 */ uint16_t static_number;
    /* 0012 end */
} MESH_INFO;

typedef struct ROOM_INFO {
    /* 0000 */ int16_t *data;
    /* 0004 */ DOOR_INFOS *doors;
    /* 0008 */ FLOOR_INFO *floor;
    /* 000C */ LIGHT_INFO *light;
    /* 0010 */ MESH_INFO *mesh;
    /* 0014 */ int32_t x;
    /* 0018 */ int32_t y;
    /* 001C */ int32_t z;
    /* 0020 */ int32_t min_floor;
    /* 0024 */ int32_t max_ceiling;
    /* 0028 */ int16_t x_size;
    /* 002A */ int16_t y_size;
    /* 002C */ int16_t ambient;
    /* 002E */ int16_t num_lights;
    /* 0030 */ int16_t num_meshes;
    /* 0032 */ int16_t left;
    /* 0034 */ int16_t right;
    /* 0036 */ int16_t top;
    /* 0038 */ int16_t bottom;
    /* 003A */ int16_t bound_active;
    /* 003C */ int16_t item_number;
    /* 003E */ int16_t fx_number;
    /* 0040 */ int16_t flipped_room;
    /* 0042 */ uint16_t flags;
    /* 0044 end */
} ROOM_INFO;

typedef struct ITEM_INFO {
    /* 0000 */ int32_t floor;
    /* 0004 */ uint32_t touch_bits;
    /* 0008 */ uint32_t mesh_bits;
    /* 000C */ int16_t object_number;
    /* 000E */ int16_t current_anim_state;
    /* 0010 */ int16_t goal_anim_state;
    /* 0012 */ int16_t required_anim_state;
    /* 0014 */ int16_t anim_number;
    /* 0016 */ int16_t frame_number;
    /* 0018 */ int16_t room_number;
    /* 001A */ int16_t next_item;
    /* 001C */ int16_t next_active;
    /* 001E */ int16_t speed;
    /* 0020 */ int16_t fall_speed;
    /* 0022 */ int16_t hit_points;
    /* 0024 */ int16_t box_number;
    /* 0026 */ int16_t timer;
    /* 0028 */ int16_t flags;
    /* 002A */ int16_t shade;
    /* 002C */ void *data;
    /* 0030 */ PHD_3DPOS pos;
    /* 0042 */ uint16_t active : 1;
    /*      */ uint16_t status : 2;
    /*      */ uint16_t gravity_status : 1;
    /*      */ uint16_t hit_status : 1;
    /*      */ uint16_t collidable : 1;
    /*      */ uint16_t looked_at : 1;
    /* 0044 end */
} ITEM_INFO;

typedef struct LARA_ARM {
    /* 0000 */ int16_t *frame_base;
    /* 0004 */ int16_t frame_number;
    /* 0006 */ int16_t lock;
    /* 0008 */ PHD_ANGLE y_rot;
    /* 000A */ PHD_ANGLE x_rot;
    /* 000C */ PHD_ANGLE z_rot;
    /* 000E */ uint16_t flash_gun;
    /* 0010 end */
} LARA_ARM;

typedef struct AMMO_INFO {
    /* 0000 */ int32_t ammo;
    /* 0004 */ int32_t hit;
    /* 0008 */ int32_t miss;
    /* 000C end */
} AMMO_INFO;

typedef struct BOX_NODE {
    /* 0000 */ int16_t exit_box;
    /* 0002 */ uint16_t search_number;
    /* 0004 */ int16_t next_expansion;
    /* 0006 */ int16_t box_number;
    /* 0008 end */
} BOX_NODE;

typedef struct LOT_INFO {
    /* 0000 */ BOX_NODE *node;
    /* 0004 */ int16_t head;
    /* 0006 */ int16_t tail;
    /* 0008 */ uint16_t search_number;
    /* 000A */ uint16_t block_mask;
    /* 000C */ int16_t step;
    /* 000E */ int16_t drop;
    /* 0010 */ int16_t fly;
    /* 0012 */ int16_t zone_count;
    /* 0014 */ int16_t target_box;
    /* 0016 */ int16_t required_box;
    /* 0018 */ PHD_VECTOR target;
    /* 0024 end */
} LOT_INFO;

typedef struct FX_INFO {
    /* 0000 */ PHD_3DPOS pos;
    /* 0012 */ int16_t room_number;
    /* 0014 */ int16_t object_number;
    /* 0016 */ int16_t next_fx;
    /* 0018 */ int16_t next_active;
    /* 001A */ int16_t speed;
    /* 001C */ int16_t fall_speed;
    /* 001E */ int16_t frame_number;
    /* 0020 */ int16_t counter;
    /* 0022 */ int16_t shade;
    /* 0024 end */
} FX_INFO;

typedef struct LARA_INFO {
    /* 0000 */ int16_t item_number;
    /* 0002 */ int16_t gun_status;
    /* 0004 */ int16_t gun_type;
    /* 0006 */ int16_t request_gun_type;
    /* 0008 */ int16_t calc_fall_speed;
    /* 000A */ int16_t water_status;
    /* 000C */ int16_t pose_count;
    /* 000E */ int16_t hit_frame;
    /* 0010 */ int16_t hit_direction;
    /* 0012 */ int16_t air;
    /* 0014 */ int16_t dive_count;
    /* 0016 */ int16_t death_count;
    /* 0018 */ int16_t current_active;
    /* 001A */ int16_t spaz_effect_count;
    /* 001C */ FX_INFO *spaz_effect;
    /* 0020 */ int32_t mesh_effects;
    /* 0024 */ int16_t *mesh_ptrs[LM_NUMBER_OF];
    /* 0060 */ ITEM_INFO *target;
    /* 0064 */ PHD_ANGLE target_angles[2];
    /* 0068 */ int16_t turn_rate;
    /* 006A */ int16_t move_angle;
    /* 006C */ int16_t head_y_rot;
    /* 006E */ int16_t head_x_rot;
    /* 0070 */ int16_t head_z_rot;
    /* 0072 */ int16_t torso_y_rot;
    /* 0074 */ int16_t torso_x_rot;
    /* 0076 */ int16_t torso_z_rot;
    /* 0078 */ LARA_ARM left_arm;
    /* 0088 */ LARA_ARM right_arm;
    /* 0098 */ AMMO_INFO pistols;
    /* 00A4 */ AMMO_INFO magnums;
    /* 00B0 */ AMMO_INFO uzis;
    /* 00BC */ AMMO_INFO shotgun;
    /* 00C8 */ LOT_INFO LOT;
    /* 00E6 end */
} LARA_INFO;

typedef struct START_INFO {
    /* 0000 */ uint16_t pistol_ammo;
    /* 0002 */ uint16_t magnum_ammo;
    /* 0004 */ uint16_t uzi_ammo;
    /* 0006 */ uint16_t shotgun_ammo;
    /* 0008 */ uint8_t num_medis;
    /* 0009 */ uint8_t num_big_medis;
    /* 000A */ uint8_t num_scions;
    /* 000B */ int8_t gun_status;
    /* 000C */ int8_t gun_type;
    /* 000D */ uint16_t available : 1;
    /*      */ uint16_t got_pistols : 1;
    /*      */ uint16_t got_magnums : 1;
    /*      */ uint16_t got_uzis : 1;
    /*      */ uint16_t got_shotgun : 1;
    /*      */ uint16_t costume : 1;
    /* 000F end */
} START_INFO;

typedef struct SAVEGAME_INFO {
    START_INFO *start;
    uint32_t timer;
    uint32_t kills;
    uint16_t secrets;
    uint16_t current_level;
    uint8_t pickups;
    uint8_t bonus_flag;
    uint8_t num_pickup1;
    uint8_t num_pickup2;
    uint8_t num_puzzle1;
    uint8_t num_puzzle2;
    uint8_t num_puzzle3;
    uint8_t num_puzzle4;
    uint8_t num_key1;
    uint8_t num_key2;
    uint8_t num_key3;
    uint8_t num_key4;
    uint8_t num_leadbar;
    uint8_t challenge_failed;
    char buffer[MAX_SAVEGAME_BUFFER];
} SAVEGAME_INFO;

typedef struct CREATURE_INFO {
    /* 0000 */ int16_t head_rotation;
    /* 0002 */ int16_t neck_rotation;
    /* 0004 */ int16_t maximum_turn;
    /* 0006 */ uint16_t flags;
    /* 0008 */ int16_t item_num;
    /* 000A */ int32_t mood;
    /* 000E */ LOT_INFO LOT;
    /* 002C */ PHD_VECTOR target;
    /* 003E end */
} CREATURE_INFO;

typedef struct TEXTSTRING {
    /* 0000 */ uint32_t flags;
    /* 0004 */ uint16_t text_flags;
    /* 0006 */ uint16_t bgnd_flags;
    /* 0008 */ uint16_t outl_flags;
    /* 000A */ int16_t xpos;
    /* 000C */ int16_t ypos;
    /* 000E */ int16_t zpos; // unused
    /* 0010 */ int16_t letter_spacing;
    /* 0012 */ int16_t word_spacing;
    /* 0014 */ int16_t flash_rate;
    /* 0016 */ int16_t flash_count;
    /* 0018 */ int16_t bgnd_colour; // unused
    /* 001A */ SG_COL *bgnd_gour; // unused
    /* 001E */ int16_t outl_colour; // unused
    /* 0020 */ SG_COL *outl_gour; // unused
    /* 0024 */ int16_t bgnd_size_x;
    /* 0026 */ int16_t bgnd_size_y;
    /* 0028 */ int16_t bgnd_off_x;
    /* 002A */ int16_t bgnd_off_y;
    /* 002C */ int16_t bgnd_off_z; // unused
    /* 002E */ int32_t scale_h;
    /* 0032 */ int32_t scale_v;
    /* 0034 */ char *string;
    /* 0038 end */
} TEXTSTRING;

typedef struct DISPLAYPU {
    /* 0000 */ int16_t duration;
    /* 0002 */ int16_t sprnum;
    /* 0004 end */
} DISPLAYPU;

typedef struct COLL_INFO {
    /* 0000 */ int32_t mid_floor;
    /* 0004 */ int32_t mid_ceiling;
    /* 0008 */ int32_t mid_type;
    /* 000C */ int32_t front_floor;
    /* 0010 */ int32_t front_ceiling;
    /* 0014 */ int32_t front_type;
    /* 0018 */ int32_t left_floor;
    /* 001C */ int32_t left_ceiling;
    /* 0020 */ int32_t left_type;
    /* 0024 */ int32_t right_floor;
    /* 0028 */ int32_t right_ceiling;
    /* 002C */ int32_t right_type;
    /* 0030 */ int32_t radius;
    /* 0034 */ int32_t bad_pos;
    /* 0038 */ int32_t bad_neg;
    /* 003C */ int32_t bad_ceiling;
    /* 0040 */ PHD_VECTOR shift;
    /* 0046 */ PHD_VECTOR old;
    /* 004C */ int16_t facing;
    /* 004E */ int16_t quadrant;
    /* 0050 */ int16_t coll_type;
    /* 0052 */ int16_t *trigger;
    /* 0056 */ int8_t tilt_x;
    /* 0057 */ int8_t tilt_z;
    /* 0058 */ int8_t hit_by_baddie;
    /* 0059 */ int8_t hit_static;
    /* 005A */ uint16_t slopes_are_walls : 1;
    /*      */ uint16_t slopes_are_pits : 1;
    /*      */ uint16_t lava_is_pit : 1;
    /*      */ uint16_t enable_baddie_push : 1;
    /*      */ uint16_t enable_spaz : 1;
    /* 005C end */
} COLL_INFO;

typedef struct OBJECT_INFO {
    /* 0000 */ int16_t nmeshes;
    /* 0002 */ int16_t mesh_index;
    /* 0004 */ int32_t bone_index;
    /* 0008 */ int16_t *frame_base;
    /* 000C */ void (*initialise)(int16_t item_num);
    /* 0010 */ void (*control)(int16_t item_num);
    /* 0014 */ void (*floor)(
        ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);
    /* 0018 */ void (*ceiling)(
        ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);
    /* 001C */ void (*draw_routine)(ITEM_INFO *item);
    /* 0020 */ void (*collision)(
        int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
    /* 0024 */ int16_t anim_index;
    /* 0026 */ int16_t hit_points;
    /* 0028 */ int16_t pivot_length;
    /* 002A */ int16_t radius;
    /* 002C */ int16_t smartness;
    /* 002E */ int16_t shadow_size;
    /* 0030 */ uint16_t loaded : 1;
    /*      */ uint16_t intelligent : 1;
    /*      */ uint16_t save_position : 1;
    /*      */ uint16_t save_hitpoints : 1;
    /*      */ uint16_t save_flags : 1;
    /*      */ uint16_t save_anim : 1;
    /* 0032 end */
} OBJECT_INFO;

typedef struct STATIC_INFO {
    /* 0000 */ int16_t mesh_number;
    /* 0002 */ int16_t flags;
    /* 0004 */ int16_t x_minp;
    /* 0006 */ int16_t x_maxp;
    /* 0008 */ int16_t y_minp;
    /* 000A */ int16_t y_maxp;
    /* 000C */ int16_t z_minp;
    /* 000E */ int16_t z_maxp;
    /* 0010 */ int16_t x_minc;
    /* 0012 */ int16_t x_maxc;
    /* 0014 */ int16_t y_minc;
    /* 0016 */ int16_t y_maxc;
    /* 0018 */ int16_t z_minc;
    /* 001A */ int16_t z_maxc;
    /* 001C end */
} STATIC_INFO;

typedef struct GAME_VECTOR {
    /* 0000 */ int32_t x;
    /* 0004 */ int32_t y;
    /* 0008 */ int32_t z;
    /* 000C */ int16_t room_number;
    /* 000E */ int16_t box_number;
    /* 0010 end */
} GAME_VECTOR;

typedef struct OBJECT_VECTOR {
    /* 0000 */ int32_t x;
    /* 0004 */ int32_t y;
    /* 0008 */ int32_t z;
    /* 000C */ int16_t data;
    /* 000E */ int16_t flags;
    /* 0010 end */
} OBJECT_VECTOR;

typedef struct CAMERA_INFO {
    /* 0000 */ GAME_VECTOR pos;
    /* 0010 */ GAME_VECTOR target;
    /* 0020 */ int32_t type;
    /* 0024 */ int32_t shift;
    /* 0028 */ int32_t flags;
    /* 002C */ int32_t fixed_camera;
    /* 0030 */ int32_t number_frames;
    /* 0034 */ int32_t bounce;
    /* 0038 */ int32_t underwater;
    /* 003C */ int32_t target_distance;
    /* 0040 */ int32_t target_square;
    /* 0044 */ int16_t target_angle;
    /* 0046 */ int16_t actual_angle;
    /* 0048 */ int16_t target_elevation;
    /* 004A */ int16_t box;
    /* 004C */ int16_t number;
    /* 004E */ int16_t last;
    /* 0050 */ int16_t timer;
    /* 0052 */ int16_t speed;
    /* 0054 */ ITEM_INFO *item;
    /* 0058 */ ITEM_INFO *last_item;
    /* 005C */ OBJECT_VECTOR *fixed;
    /* 0060 end */
} CAMERA_INFO;

typedef struct ANIM_STRUCT {
    /* 0000 */ int16_t *frame_ptr;
    /* 0004 */ int16_t interpolation;
    /* 0006 */ int16_t current_anim_state;
    /* 0008 */ int32_t velocity;
    /* 000C */ int32_t acceleration;
    /* 0010 */ int16_t frame_base;
    /* 0012 */ int16_t frame_end;
    /* 0014 */ int16_t jump_anim_num;
    /* 0016 */ int16_t jump_frame_num;
    /* 0018 */ int16_t number_changes;
    /* 001A */ int16_t change_index;
    /* 001C */ int16_t number_commands;
    /* 001E */ int16_t command_index;
    /* 0020 end */
} ANIM_STRUCT;

typedef struct ANIM_CHANGE_STRUCT {
    /* 0000 */ int16_t goal_anim_state;
    /* 0004 */ int16_t number_ranges;
    /* 0006 */ int16_t range_index;
    /* 0008 end */
} ANIM_CHANGE_STRUCT;

typedef struct ANIM_RANGE_STRUCT {
    /* 0000 */ int16_t start_frame;
    /* 0004 */ int16_t end_frame;
    /* 0006 */ int16_t link_anim_num;
    /* 0008 */ int16_t link_frame_num;
    /* 000A end */
} ANIM_RANGE_STRUCT;

typedef struct DOOR_VBUF {
    /* 0000 */ int32_t xv;
    /* 0004 */ int32_t yv;
    /* 0008 */ int32_t zv;
    /* 000C end */
} DOOR_VBUF;

typedef struct WEAPON_INFO {
    /* 0000 */ PHD_ANGLE lock_angles[4];
    /* 0008 */ PHD_ANGLE left_angles[4];
    /* 0010 */ PHD_ANGLE right_angles[4];
    /* 0018 */ PHD_ANGLE aim_speed;
    /* 001A */ PHD_ANGLE shot_accuracy;
    /* 001C */ int32_t gun_height;
    /* 0020 */ int32_t damage;
    /* 0024 */ int32_t target_dist;
    /* 0028 */ int16_t recoil_frame;
    /* 002A */ int16_t flash_time;
    /* 002C */ int16_t sample_num;
    /* 002E end */
} WEAPON_INFO;

typedef struct SPHERE {
    /* 0000 */ int32_t x;
    /* 0004 */ int32_t y;
    /* 0008 */ int32_t z;
    /* 000C */ int32_t r;
    /* 0010 end */
} SPHERE;

typedef struct BITE_INFO {
    /* 0000 */ int32_t x;
    /* 0004 */ int32_t y;
    /* 0008 */ int32_t z;
    /* 000C */ int32_t mesh_num;
    /* 0010 end */
} BITE_INFO;

typedef struct AI_INFO {
    /* 0000 */ int16_t zone_number;
    /* 0002 */ int16_t enemy_zone;
    /* 0004 */ int32_t distance;
    /* 0008 */ int32_t ahead;
    /* 000C */ int32_t bite;
    /* 000E */ int16_t angle;
    /* 0010 */ int16_t enemy_facing;
    /* 0012 end */
} AI_INFO;

typedef struct BOX_INFO {
    /* 0000 */ int32_t left;
    /* 0004 */ int32_t right;
    /* 0008 */ int32_t top;
    /* 000C */ int32_t bottom;
    /* 0010 */ int16_t height;
    /* 0012 */ int16_t overlap_index;
    /* 0014 end */
} BOX_INFO;

typedef struct REQUEST_INFO {
    /* 0000 */ uint16_t items;
    /* 0002 */ uint16_t requested;
    /* 0004 */ uint16_t vis_lines;
    /* 0006 */ uint16_t line_offset;
    /* 0008 */ uint16_t line_old_offset;
    /* 000A */ uint16_t pix_width;
    /* 000C */ uint16_t line_height;
    /* 000E */ int16_t x;
    /* 0010 */ int16_t y;
    /* 0012 */ int16_t z; // unused
    /* 0014 */ uint16_t flags;
    /* 0016 */ const char *heading_text;
    /* 0020 */ char *item_texts;
    /* 001E */ int16_t item_text_len;
    /* 0020 */ TEXTSTRING *heading;
    /* 0024 */ TEXTSTRING *background;
    /* 0028 */ TEXTSTRING *moreup;
    /* 002C */ TEXTSTRING *moredown;
    /* 0030 */ TEXTSTRING *texts[MAX_REQLINES];
    /* 0070 */ uint16_t item_flags[MAX_REQLINES];
    /* 0090 end */
} REQUEST_INFO;

typedef struct IMOTION_INFO {
    /* 0000 */ int16_t count;
    /* 0002 */ int16_t status;
    /* 0004 */ int16_t status_target;
    /* 0006 */ int16_t radius_target;
    /* 0008 */ int16_t radius_rate;
    /* 000A */ int16_t camera_ytarget;
    /* 000C */ int16_t camera_yrate;
    /* 000E */ int16_t camera_pitch_target;
    /* 0010 */ int16_t camera_pitch_rate;
    /* 0012 */ int16_t rotate_target;
    /* 0014 */ int16_t rotate_rate;
    /* 0016 */ PHD_ANGLE item_ptxrot_target;
    /* 0018 */ PHD_ANGLE item_ptxrot_rate;
    /* 001A */ PHD_ANGLE item_xrot_target;
    /* 001C */ PHD_ANGLE item_xrot_rate;
    /* 001E */ int32_t item_ytrans_target;
    /* 0022 */ int32_t item_ytrans_rate;
    /* 0026 */ int32_t item_ztrans_target;
    /* 002A */ int32_t item_ztrans_rate;
    /* 002E */ int32_t misc;
    /* 0032 end */
} IMOTION_INFO;

typedef struct INVENTORY_SPRITE {
    /* 0000 */ int16_t shape;
    /* 0002 */ int16_t x;
    /* 0004 */ int16_t y;
    /* 0006 */ int16_t z;
    /* 0008 */ int32_t param1;
    /* 000C */ int32_t param2;
    /* 0010 */ SG_COL *grdptr;
    /* 0014 */ int16_t sprnum;
    /* 0016 end */
} INVENTORY_SPRITE;

typedef struct INVENTORY_ITEM {
    /* 0000 */ char *string;
    /* 0004 */ int16_t object_number;
    /* 0006 */ int16_t frames_total;
    /* 0008 */ int16_t current_frame;
    /* 000A */ int16_t goal_frame;
    /* 000C */ int16_t open_frame;
    /* 000E */ int16_t anim_direction;
    /* 0010 */ int16_t anim_speed;
    /* 0012 */ int16_t anim_count;
    /* 0014 */ PHD_ANGLE pt_xrot_sel;
    /* 0016 */ PHD_ANGLE pt_xrot;
    /* 0018 */ PHD_ANGLE x_rot_sel;
    /* 001A */ PHD_ANGLE x_rot;
    /* 001C */ PHD_ANGLE y_rot_sel;
    /* 001E */ PHD_ANGLE y_rot;
    /* 0020 */ int32_t ytrans_sel;
    /* 0024 */ int32_t ytrans;
    /* 0028 */ int32_t ztrans_sel;
    /* 002C */ int32_t ztrans;
    /* 0030 */ uint32_t which_meshes;
    /* 0034 */ uint32_t drawn_meshes;
    /* 0038 */ int16_t inv_pos;
    /* 003A */ INVENTORY_SPRITE **sprlist;
    /* 003E end */
} INVENTORY_ITEM;

typedef struct RING_INFO {
    /* 0000 */ INVENTORY_ITEM **list;
    /* 0004 */ int16_t type;
    /* 0006 */ int16_t radius;
    /* 0008 */ int16_t camera_pitch;
    /* 000A */ int16_t rotating;
    /* 000C */ int16_t rot_count;
    /* 000E */ int16_t current_object;
    /* 0010 */ int16_t target_object;
    /* 0012 */ int16_t number_of_objects;
    /* 0014 */ int16_t angle_adder;
    /* 0016 */ int16_t rot_adder;
    /* 0018 */ int16_t rot_adder_l;
    /* 001A */ int16_t rot_adder_r;
    /* 001C */ PHD_3DPOS ringpos;
    /* 002E */ PHD_3DPOS camera;
    /* 0040 */ PHD_VECTOR light;
    /* 004C */ IMOTION_INFO *imo;
    /* 0050 end */
} RING_INFO;

typedef struct GAMEFLOW_SEQUENCE {
    GAMEFLOW_SEQUENCE_TYPE type;
    void *data;
} GAMEFLOW_SEQUENCE;

typedef struct GAMEFLOW_LEVEL {
    GAMEFLOW_LEVEL_TYPE level_type;
    int16_t music;
    const char *level_title;
    const char *level_file;
    const char *key1;
    const char *key2;
    const char *key3;
    const char *key4;
    const char *pickup1;
    const char *pickup2;
    const char *puzzle1;
    const char *puzzle2;
    const char *puzzle3;
    const char *puzzle4;
    int8_t demo;
    int16_t secrets;
    GAMEFLOW_SEQUENCE *sequence;
} GAMEFLOW_LEVEL;

typedef struct GAMEFLOW {
    int32_t gym_level_num;
    int32_t first_level_num;
    int32_t last_level_num;
    int32_t title_level_num;
    int32_t level_count;
    const char *save_game_fmt;
    int8_t has_demo;
    int32_t demo_delay;
    int8_t enable_game_modes;
    int8_t enable_save_crystals;
    GAMEFLOW_LEVEL *levels;
    char *strings[GS_NUMBER_OF];
} GAMEFLOW;

typedef struct MN_SFX_PLAY_INFO {
    void *handle;
    PHD_3DPOS *pos;
    uint32_t loudness;
    int16_t volume;
    int16_t pan;
    int16_t fxnum;
    int16_t mn_flags;
} MN_SFX_PLAY_INFO;

typedef struct SAMPLE_INFO {
    int16_t number;
    int16_t volume;
    int16_t randomness;
    int16_t flags;
} SAMPLE_INFO;

typedef struct SAMPLE_DATA {
    char *data;
    int32_t length;
    int16_t bits_per_sample;
    int16_t channels;
    int16_t sample_rate;
    int16_t block_align;
    int16_t volume;
    int32_t pan;
    void *handle;
} SAMPLE_DATA;

#pragma pack(pop)

typedef void (*ControlRoutine)(ITEM_INFO *, COLL_INFO *);
typedef void (*CollisionRoutine)(ITEM_INFO *, COLL_INFO *);
typedef void (*EffectRoutine)(ITEM_INFO *item);

#endif
