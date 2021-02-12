#ifndef TR1MAIN_STRUCT_H
#define TR1MAIN_STRUCT_H

#include <stdint.h>
#include <stdio.h>

typedef uint16_t PHD_ANGLE;
typedef uint32_t SG_COL;
typedef void UNKNOWN_STRUCT;

#define NUM_PU 3
#define NUM_SLOTS 8
#define MAX_ROOMS 1024
#define MAX_FRAMES 10
#define NUMBER_ITEMS 256
#define MAX_SECRETS 16
#define MAX_SAVEGAME_BUFFER (10 * 1024)
#define SAVEGAME_VERSION 0x1
#define LARA_HITPOINTS 1000
#define LARA_AIR 1800
#define NO_ITEM -1
#define SHOTGUN_AMMO_CLIP 6
#define SFX_ALWAYS 2
#define NUM_EFFECTS 100
#define DEATH_WAIT (10 * 30)
#define DEATH_WAIT_MIN (0 * 30)

#define TREAD_A 108
#define TREAD_F 1736
#define STOP_A 11
#define STOP_F 185

#define WALL_L 1024
#define STEP_L 256

typedef enum {
    O_LARA = 0,
    O_GUN_ITEM = 84,
    O_SHOTGUN_ITEM = 85,
    O_MAGNUM_ITEM = 86,
    O_UZI_ITEM = 87,
    O_MEDI_ITEM = 93,
    O_BIGMEDI_ITEM = 94,
    O_GUN_OPTION = 99,
    O_SHOTGUN_OPTION = 100,
    O_MAGNUM_OPTION = 101,
    O_UZI_OPTION = 102,
    O_MEDI_OPTION = 108,
    O_BIGMEDI_OPTION = 109,
    NUMBER_OBJECTS = 191,
} GAME_OBJECT_ID;

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
    AS_WATEROU = 55,
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
    /* 0000 */ int16_t x;
    /* 0002 */ int16_t y;
    /* 0004 */ int16_t z;
    /* 0006 end */
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
    /* 000C */ uint16_t y_rot;
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
    /* 000C */ uint16_t rot_x;
    /* 000E */ uint16_t rot_y;
    /* 0010 */ uint16_t rot_z;
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
    /* 001C */ UNKNOWN_STRUCT* spaz_effect;
    /* 0020 */ int32_t mesh_effects;
    /* 0024 */ int16_t* mesh_ptrs[15];
    /* 0060 */ ITEM_INFO* target;
    /* 0064 */ PHD_ANGLE target_angles[2];
    /* 0068 */ int16_t turn_rate;
    /* 006A */ int16_t move_angle;
    /* 006C */ int16_t head_y_rot;
    /* 006E */ int16_t head_x_rot;
    /* 0070 */ int16_t head_z_rot;
    /* 0072 */ int16_t torso_x_rot;
    /* 0074 */ int16_t torso_y_rot;
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
    /* 0032 */ uint8_t pad[12];
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

#pragma pop

#endif
