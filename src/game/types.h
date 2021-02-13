#ifndef TR1MAIN_GAME_TYPES_H
#define TR1MAIN_GAME_TYPES_H

#include "const.h"
#include <stdint.h>

typedef uint16_t PHD_ANGLE;
typedef uint32_t SG_COL;
typedef void UNKNOWN_STRUCT;

typedef enum {
    CHASE_CAMERA = 0,
    FIXED_CAMERA = 1,
    LOOK_CAMERA = 2,
    COMBAT_CAMERA = 3,
    CINEMATIC_CAMERA = 4,
    HEAVY_CAMERA = 5,
} CAMERA_TYPE;

typedef enum {
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
    O_EVIL_NATLA = 34, // Adam, Torso
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
    O_ALPHABET = 190,
    NUMBER_OBJECTS = 191,
} GAME_OBJECT_ID;

typedef enum {
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
    AF_PICKUP = 3443,
    AF_PICKUP_UW = 2970,
    AF_PICKUPSCION = 44,
    AF_USEPUZZLE = 3372,
} LARA_ANIMATION_FRAMES;

typedef enum {
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
} LARA_ANIMATION_ANIMS;

typedef enum {
    LARA_ABOVEWATER = 0,
    LARA_UNDERWATER = 1,
    LARA_SURFACE = 2,
} LARA_WATER_STATES;

typedef enum {
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
} LARA_STATES;

typedef enum {
    LG_ARMLESS = 0,
    LG_HANDSBUSY = 1,
    LG_DRAW = 2,
    LG_UNDRAW = 3,
    LG_READY = 4,
} LARA_GUN_STATES;

typedef enum {
    LG_UNARMED = 0,
    LG_PISTOLS = 1,
    LG_MAGNUMS = 2,
    LG_UZIS = 3,
    LG_SHOTGUN = 4,
    NUM_WEAPONS = 5
} LARA_GUN_TYPES;

typedef enum {
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
} LARA_MESHES;

typedef enum {
    BORED_MOOD = 0,
    ATTACK_MOOD = 1,
    ESCAPE_MOOD = 2,
    STALK_MOOD = 3,
} MOOD_TYPE;

typedef enum {
    GBUF_RoomInfos = 11,
    GBUF_RoomMesh = 12,
    GBUF_RoomDoor = 13,
    GBUF_RoomFloor = 14,
    GBUF_RoomLights = 15,
    GBUF_RoomStaticMeshInfos = 16,
    GBUF_FloorData = 17,
    GBUF_Items = 18,
    GBUF_CreatureData = 33,
    GBUF_CreatureLot = 34,
} GAMEALLOC_BUFFER;

typedef enum {
    LV_GYM = 0,
    LV_FIRSTLEVEL,
    LV_LEVEL1 = 1, // Peru 1: Caves
    LV_LEVEL2 = 2, // Peru 2: City of Vilcabamba
    LV_LEVEL3A = 3, // Peru 3: The Lost Valley
    LV_LEVEL3B = 4, // Peru 4: Tomb of Qualopec
    LV_LEVEL4 = 5, // Greece 1: St Francis Folly
    LV_LEVEL5 = 6, // Greece 2: Colosseum
    LV_LEVEL6 = 7, // Greece 3: Place Midas
    LV_LEVEL7A = 8, // Greece 4: Cistern
    LV_LEVEL7B = 9, // Greece 5: Tomb of Tihocan
    LV_LEVEL8A = 10, // Egypt 1: City of Khamoon
    LV_LEVEL8B = 11, // Egypt 2: Obelisk of Khamoon
    LV_LEVEL8C = 12, // Egypt 3: Sanctuary of Scion
    LV_LEVEL10A = 13, // Lost island 1: Natla's Mines
    LV_LEVEL10B = 14, // Lost island 2: Atlantis
    LV_LEVEL10C = 15, // Lost island 3: The great pyramid
    LV_CUTSCENE1 = 16,
    LV_CUTSCENE2 = 17,
    LV_CUTSCENE3 = 18,
    LV_CUTSCENE4 = 19,
    LV_TITLE = 20,
    LV_CURRENT = 21,
    // UB_LEVEL1     = 22, // TRUB - Egypt
    // UB_LEVEL2     = 23, // TRUB - Temple of Cat
    // UB_LEVEL3     = 24,
    // UB_LEVEL4     = 25,
    NUMBER_OF_LEVELS = 22,
} GAME_LEVELS;

typedef enum {
    INV_GAME_MODE = 0,
    INV_TITLE_MODE = 1,
    INV_KEYS_MODE = 2,
    INV_SAVE_MODE = 3,
    INV_LOAD_MODE = 4,
    INV_DEATH_MODE = 5,
} INV_MODES;

typedef enum {
    STARTGAME = 0,
    STARTCINE = 1 << 6,
    STARTFMV = 2 << 6,
    STARTDEMO = 3 << 6,
    EXIT_TO_TITLE = 4 << 6,
    LEVELCOMPLETE = 5 << 6,
    EXITGAME = 6 << 6,
    EXIT_TO_OPTION = 7 << 6,
    TITLE_DESELECT = 8 << 6,
    STARTMENU = 9 << 6,
    LOOPINV = 10 << 6,
    LOOPGAME = 11 << 6,
    ENDGAME = 12 << 6,
    INTRO = 13 << 6,
    PLAYFMV = 14 << 6,
    CREDITS = 15 << 6,
    PREWARMGAME = 16 << 6,
    LOOPCINE = 17 << 6
} TITLE_OPTIONS;

#define IN_FORWARD (1 << 0)
#define IN_BACK (1 << 1)
#define IN_LEFT (1 << 2)
#define IN_RIGHT (1 << 3)
#define IN_JUMP (1 << 4)
#define IN_DRAW (1 << 5)
#define IN_ACTION (1 << 6)
#define IN_SLOW (1 << 7)
#define IN_OPTION (1 << 8)
#define IN_LOOK (1 << 9)
#define IN_STEPL (1 << 10)
#define IN_STEPR (1 << 11)
#define IN_ROLL (1 << 12)
#define IN_PAUSE (1 << 13)
#define IN_A (1 << 14)
#define IN_B (1 << 15)
#define IN_C (1 << 16)
#define IN_MENUBACK (1 << 17)
#define IN_UP (1 << 18)
#define IN_DOWN (1 << 19)
#define IN_SELECT (1 << 20)
#define IN_DESELECT (1 << 21)
#define IN_SAVE (1 << 22)
#define IN_LOAD (1 << 23)
#define IN_ACTION_AUTO (1 << 24)
#define IN_CHEAT (1 << 25)
#define IN_D (1 << 26)
#define IN_E (1 << 27)
#define IN_F (1 << 28)

#pragma pack(push, 1)

typedef struct {
    /* 0000 */ uint16_t x;
    /* 0002 */ uint16_t y;
    /* 0004 end */
} POS_2D;

typedef struct {
    /* 0000 */ uint16_t x;
    /* 0002 */ uint16_t y;
    /* 0004 */ uint16_t z;
    /* 0006 end */
} POS_3D;

typedef struct {
    /* 0000 */ int32_t x;
    /* 0004 */ int32_t y;
    /* 0008 */ int32_t z;
    /* 000C end */
} PHD_VECTOR;

typedef struct {
    /* 0000 */ uint16_t room;
    /* 0002 */ uint16_t x;
    /* 0004 */ uint16_t y;
    /* 0006 */ uint16_t z;
    /* 0008 */ POS_3D vertex[4];
    /* 0020 end */
} DOOR_INFO;

typedef struct {
    /* 0000 */ uint16_t count;
    /* 0002 */ DOOR_INFO door[];
    /* 0006 end */
} DOOR_INFOS;

typedef struct {
    /* 0000 */ uint16_t index;
    /* 0002 */ uint16_t box;
    /* 0004 */ uint8_t pit_room;
    /* 0005 */ uint8_t floor;
    /* 0006 */ uint8_t sky_room;
    /* 0007 */ uint8_t ceiling;
    /* 0008 end */
} FLOOR_INFO;

typedef struct {
    /* 0000 */ uint32_t x;
    /* 0004 */ uint32_t y;
    /* 0008 */ uint32_t z;
    /* 000C */ uint16_t intensity;
    /* 000E */ uint32_t falloff;
    /* 0012 end */
} LIGHT_INFO;

typedef struct {
    /* 0000 */ uint32_t x;
    /* 0004 */ uint32_t y;
    /* 0008 */ uint32_t z;
    /* 000C */ PHD_ANGLE y_rot;
    /* 000E */ uint16_t shade;
    /* 0010 */ uint16_t static_number;
    /* 0012 end */
} MESH_INFO;

typedef struct {
    /* 0000 */ int16_t* data;
    /* 0004 */ DOOR_INFOS* doors;
    /* 0008 */ FLOOR_INFO* floor;
    /* 000C */ LIGHT_INFO* light;
    /* 0010 */ MESH_INFO* mesh;
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
    /* 0032 */ int16_t bound_left;
    /* 0034 */ int16_t bound_right;
    /* 0036 */ int16_t bound_top;
    /* 0038 */ int16_t bound_bottom;
    /* 003A */ int16_t bound_active;
    /* 003C */ int16_t item_number;
    /* 003E */ int16_t fx_number;
    /* 0040 */ int16_t flipped_room;
    /* 0042 */ uint16_t flags;
    /* 0044 end */
} ROOM_INFO;

typedef struct {
    /* 0000 */ int32_t x;
    /* 0004 */ int32_t y;
    /* 0008 */ int32_t z;
    /* 000C */ int16_t x_rot;
    /* 000E */ int16_t y_rot;
    /* 0010 */ int16_t z_rot;
    /* 0012 end */
} PHD_3DPOS;

typedef struct {
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
    /* 002C */ void* data;
    /* 0030 */ PHD_3DPOS pos;
    /* 0042 */ uint16_t active : 1;
    /*      */ uint16_t status : 2;
    /*      */ uint16_t gravity_status : 1;
    /*      */ uint16_t hit_status : 1;
    /*      */ uint16_t collidable : 1;
    /*      */ uint16_t looked_at : 1;
    /* 0044 end */
} ITEM_INFO;

typedef struct {
    /* 0000 */ uint16_t* frame_base;
    /* 0004 */ uint16_t frame_number;
    /* 0006 */ uint16_t lock;
    /* 0008 */ PHD_ANGLE y_rot;
    /* 000A */ PHD_ANGLE x_rot;
    /* 000C */ PHD_ANGLE z_rot;
    /* 000E */ uint16_t flash_gun;
    /* 0010 end */
} LARA_ARM;

typedef struct {
    /* 0000 */ int32_t ammo;
    /* 0004 */ int32_t hit;
    /* 0008 */ int32_t miss;
    /* 000C end */
} AMMO_INFO;

typedef struct {
    /* 0000 */ int16_t exit_box;
    /* 0002 */ uint16_t search_number;
    /* 0004 */ int16_t next_expansion;
    /* 0006 */ int16_t box_number;
    /* 0008 end */
} BOX_NODE;

typedef struct {
    /* 0000 */ BOX_NODE* node;
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
    /* 001E end */
} LOT_INFO;

typedef struct {
    /* 0000 */ PHD_3DPOS pos;
    /* 0012 */ int16_t room_number;
    /* 0014 */ int16_t object_number;
    /* 0016 */ int16_t next_fx;
    /* 0018 */ int16_t next_active;
    /* 001A */ int16_t speed;
    /* 001C */ int16_t fallspeed;
    /* 001E */ int16_t frame_number;
    /* 0020 */ int16_t counter;
    /* 0022 */ int16_t shade;
    /* 0024 end */
} FX_INFO;

typedef struct {
    /* 0000 */ int16_t item_number;
    /* 0002 */ int16_t gun_status;
    /* 0004 */ int16_t gun_type;
    /* 0006 */ int16_t request_gun_type;
    /* 0008 */ int16_t calc_fallspeed;
    /* 000A */ int16_t water_status;
    /* 000C */ int16_t pose_count;
    /* 000E */ int16_t hit_frames;
    /* 0010 */ int16_t hit_direction;
    /* 0012 */ int16_t air;
    /* 0014 */ int16_t dive_count;
    /* 0016 */ int16_t death_count;
    /* 0018 */ int16_t current_active;
    /* 001A */ int16_t spaz_effect_count;
    /* 001C */ FX_INFO* spaz_effect;
    /* 0020 */ int32_t mesh_effects;
    /* 0024 */ int16_t* mesh_ptrs[15];
    /* 0060 */ ITEM_INFO* target;
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

typedef struct {
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
    /* 000F end */
} START_INFO;

typedef struct {
    /* 0000 */ START_INFO start[NUMBER_OF_LEVELS];
    /* 014A */ uint32_t timer;
    /* 014E */ uint32_t kills;
    /* 0152 */ uint16_t secrets;
    /* 0154 */ uint16_t current_level;
    /* 0156 */ uint8_t pickups;
    /* 0157 */ uint8_t bonus_flag;
    /* 0158 */ uint8_t num_pickup1;
    /* 0159 */ uint8_t num_pickup2;
    /* 015A */ uint8_t num_puzzle1;
    /* 015B */ uint8_t num_puzzle2;
    /* 015C */ uint8_t num_puzzle3;
    /* 015D */ uint8_t num_puzzle4;
    /* 015E */ uint8_t num_key1;
    /* 015F */ uint8_t num_key2;
    /* 0160 */ uint8_t num_key3;
    /* 0161 */ uint8_t num_key4;
    /* 0162 */ uint8_t num_leadbar;
    /* 0163 */ uint8_t challenge_failed;
    /* 0164 */ char buffer[MAX_SAVEGAME_BUFFER];
    /* 2964 end */
} SAVEGAME_INFO;

typedef struct {
    /* 0000 */ uint32_t flags;
    /* 0004 */ uint16_t textflags;
    /* 0006 */ uint16_t bgndflags;
    /* 0008 */ uint16_t outlflags;
    /* 000A */ int16_t xpos;
    /* 000C */ int16_t ypos;
    /* 000E */ int16_t zpos;
    /* 0010 */ int16_t letter_spacing;
    /* 0012 */ int16_t word_spacing;
    /* 0014 */ int16_t flash_rate;
    /* 0016 */ int16_t flash_count;
    /* 0018 */ int16_t bgnd_colour;
    /* 001A */ SG_COL* bgnd_gour;
    /* 001E */ int16_t outl_colour;
    /* 0020 */ SG_COL* outl_gour;
    /* 0024 */ int16_t bgnd_size_x;
    /* 0026 */ int16_t bgnd_size_y;
    /* 0028 */ int16_t bgnd_off_x;
    /* 002A */ int16_t bgnd_off_y;
    /* 002C */ int16_t bgnd_off_z;
    /* 002E */ int32_t scale_h;
    /* 0032 */ int32_t scale_v;
    /* 0034 */ char* string;
    /* 0038 end */
} TEXTSTRING;

typedef struct {
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

typedef struct {
    /* 0000 */ int16_t duration;
    /* 0002 */ int16_t sprnum;
    /* 0004 end */
} DISPLAYPU;

typedef struct {
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
    /* 0052 */ int16_t* trigger;
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

typedef struct {
    /* 0000 */ int16_t nmeshes;
    /* 0002 */ int16_t mesh_index;
    /* 0004 */ int32_t bone_index;
    /* 0008 */ int16_t* frame_base;
    /* 000C */ void(__cdecl* initialise)(int16_t);
    /* 0010 */ void(__cdecl* control)(int16_t);
    /* 0014 */ void(__cdecl* floor)(
        ITEM_INFO* item, int32_t x, int32_t y, int32_t z, int16_t* height);
    /* 0018 */ void(__cdecl* ceiling)(
        ITEM_INFO* item, int32_t x, int32_t y, int32_t z, int16_t* height);
    /* 001C */ void(__cdecl* draw_routine)(ITEM_INFO* item);
    /* 0020 */ void(__cdecl* collision)(
        int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll);
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

typedef struct {
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

typedef struct {
    /* 0000 */ unsigned __int8 keymap[128];
    /* 0080 */ unsigned __int8 oldkeymap[128];
    /* 0100 */ unsigned __int8 keybuf[64];
    /* 0140 */ unsigned __int8 bufin;
    /* 0141 */ unsigned __int8 bufout;
    /* 0142 */ unsigned __int8 bufchars;
    /* 0143 */ unsigned __int8 extended;
    /* 0144 */ unsigned __int8 last_key;
    /* 0145 */ unsigned __int8 keys_held;
    /* 0146 end */
} KEYSTUFF;

typedef struct {
    /* 0000 */ int32_t x;
    /* 0004 */ int32_t y;
    /* 0008 */ int32_t z;
    /* 000C */ int16_t data;
    /* 000E */ int16_t flags;
    /* 0010 end */
} GAME_VECTOR;

typedef struct {
    /* 0000 */ int32_t x;
    /* 0004 */ int32_t y;
    /* 0008 */ int32_t z;
    /* 000C */ int16_t data;
    /* 000E */ int16_t flags;
    /* 0010 end */
} OBJECT_VECTOR;

typedef struct {
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
    /* 0054 */ ITEM_INFO* item;
    /* 0058 */ ITEM_INFO* last_item;
    /* 005C */ OBJECT_VECTOR* fixed;
    /* 0060 end */
} CAMERA_INFO;

typedef struct {
    /* 0000 */ int16_t* frame_ptr;
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

#pragma pop

typedef void(__cdecl* ControlRoutine)(ITEM_INFO*, COLL_INFO*);
typedef void(__cdecl* CollisionRoutine)(ITEM_INFO*, COLL_INFO*);

#endif
