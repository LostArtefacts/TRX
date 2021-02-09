#ifndef TR1MAIN_STRUCT_H
#define TR1MAIN_STRUCT_H

#include <stdio.h>
#include <stdint.h>

typedef uint16_t PHD_ANGLE;
typedef uint32_t SG_COL;
typedef void UNKNOWN_STRUCT;

#define NUM_SLOTS           8
#define MAX_ROOMS           1024
#define NUMBER_ITEMS        256
#define MAX_SECRETS         16
#define MAX_SAVEGAME_BUFFER (10*1024)
#define SAVEGAME_VERSION    0x1
#define LARA_HITPOINTS      1000
#define LARA_AIR            1800
#define NO_ITEM             -1

#define NUM_EFFECTS 100

#define TREAD_A 108
#define TREAD_F 1736
#define STOP_A  11
#define STOP_F  185

#define WALL_L 1024
#define STEP_L 256

typedef enum {
    ID_LARA = 0,
    ID_SMALL_MEDIPACK_ITEM = 93,
    ID_LARGE_MEDIPACK_ITEM = 94,
    ID_NUMBER_OBJECTS = 191,
} GAME_OBJECT_ID;

typedef enum {
    LARA_ABOVEWATER = 0,
    LARA_UNDERWATER = 1,
    LARA_SURFACE = 2,
} LARA_WATER_STATES;

typedef enum {
    AS_WALK        = 0,
    AS_RUN         = 1,
    AS_STOP        = 2,
    AS_FORWARDJUMP = 3,
    AS_POSE        = 4,
    AS_FASTBACK    = 5,
    AS_TURN_R      = 6,
    AS_TURN_L      = 7,
    AS_DEATH       = 8,
    AS_FASTFALL    = 9,
    AS_HANG        = 10,
    AS_REACH       = 11,
    AS_SPLAT       = 12,
    AS_TREAD       = 13,
    AS_LAND        = 14,
    AS_COMPRESS    = 15,
    AS_BACK        = 16,
    AS_SWIM        = 17,
    AS_GLIDE       = 18,
    AS_NULL        = 19,
    AS_FASTTURN    = 20,
    AS_STEPRIGHT   = 21,
    AS_STEPLEFT    = 22,
    AS_HIT         = 23,
    AS_SLIDE       = 24,
    AS_BACKJUMP    = 25,
    AS_RIGHTJUMP   = 26,
    AS_LEFTJUMP    = 27,
    AS_UPJUMP      = 28,
    AS_FALLBACK    = 29,
    AS_HANGLEFT    = 30,
    AS_HANGRIGHT   = 31,
    AS_SLIDEBACK   = 32,
    AS_SURFTREAD   = 33,
    AS_SURFSWIM    = 34,
    AS_DIVE        = 35,
    AS_PUSHBLOCK   = 36,
    AS_PULLBLOCK   = 37,
    AS_PPREADY     = 38,
    AS_PICKUP      = 39,
    AS_SWITCHON    = 40,
    AS_SWITCHOFF   = 41,
    AS_USEKEY      = 42,
    AS_USEPUZZLE   = 43,
    AS_UWDEATH     = 44,
    AS_ROLL        = 45,
    AS_SPECIAL     = 46,
    AS_SURFBACK    = 47,
    AS_SURFLEFT    = 48,
    AS_SURFRIGHT   = 49,
    AS_USEMIDAS    = 50,
    AS_DIEMIDAS    = 51,
    AS_SWANDIVE    = 52,
    AS_FASTDIVE    = 53,
    AS_GYMNAST     = 54,
    AS_WATEROU     = 55,
} LARA_STATES;

typedef enum {
    BORED_MOOD,
    ATTACK_MOOD,
    ESCAPE_MOOD,
    STALK_MOOD
} MOOD_TYPE;

typedef enum {
    GBUF_RoomInfos           = 11,
    GBUF_RoomMesh            = 12,
    GBUF_RoomDoor            = 13,
    GBUF_RoomFloor           = 14,
    GBUF_RoomLights          = 15,
    GBUF_RoomStaticMeshInfos = 16,
    GBUF_FloorData           = 17,
    GBUF_Items               = 18,
    GBUF_CreatureData        = 33,
    GBUF_CreatureLot         = 34,
} GAMEALLOC_BUFFER;

typedef enum {
    GYM              = 0,
    LEVEL1           = 1, // Peru 1: Caves
    LEVEL2           = 2, // Peru 2: City of Vilcabamba
    LEVEL3A          = 3, // Peru 3: The Lost Valley
    LEVEL3B          = 4, // Peru 4: Tomb of Qualopec
    LEVEL4           = 5, // Greece 1: St Francis Folly
    LEVEL5           = 6, // Greece 2: Colosseum
    LEVEL6           = 7, // Greece 3: Place Midas
    LEVEL7A          = 8, // Greece 4: Cistern
    LEVEL7B          = 9, // Greece 5: Tomb of Tihocan
    LEVEL8A          = 10, // Egypt 1: City of Khamoon
    LEVEL8B          = 11, // Egypt 2: Obelisk of Khamoon
    LEVEL8C          = 12, // Egypt 3: Sanctuary of Scion
    LEVEL10A         = 13, // Lost island 1: Natla's Mines
    LEVEL10B         = 14, // Lost island 2: Atlantis
    LEVEL10C         = 15, // Lost island 3: The great pyramid
    CUTSCENE1        = 16,
    CUTSCENE2        = 17,
    CUTSCENE3        = 18,
    CUTSCENE4        = 19,
    TITLE            = 20,
    CURRENT          = 21,
    // UB_LEVEL1     = 22, // TRUB - Egypt
    // UB_LEVEL2     = 23, // TRUB - Temple of Cat
    // UB_LEVEL3     = 24,
    // UB_LEVEL4     = 25,
    NUMBER_OF_LEVELS = 22,
} GAME_LEVELS;

#define IN_FORWARD     (1<<0)
#define IN_BACK        (1<<1)
#define IN_LEFT        (1<<2)
#define IN_RIGHT       (1<<3)
#define IN_JUMP        (1<<4)
#define IN_DRAW        (1<<5)
#define IN_ACTION      (1<<6)
#define IN_SLOW        (1<<7)
#define IN_OPTION      (1<<8)
#define IN_LOOK        (1<<9)
#define IN_STEPL       (1<<10)
#define IN_STEPR       (1<<11)
#define IN_ROLL        (1<<12)
#define IN_PAUSE       (1<<13)
#define IN_A           (1<<14) // A to F are Debug thingys..
#define IN_B           (1<<15)
#define IN_C           (1<<16)
#define IN_MENUBACK    (1<<17)
#define IN_UP          (1<<18)
#define IN_DOWN        (1<<19)
#define IN_SELECT      (1<<20)
#define IN_DESELECT    (1<<21)
#define IN_SAVE        (1<<22)
#define IN_LOAD        (1<<23)
#define IN_ACTION_AUTO (1<<24)
#define IN_CHEAT       (1<<25)
#define IN_D           (1<<26)
#define IN_E           (1<<27)
#define IN_F           (1<<28)

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

typedef struct {
    /* 0000 */ uint16_t *frame_base;
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
    /* 001C */ UNKNOWN_STRUCT *spaz_effect;
    /* 0020 */ int32_t mesh_effects;
    /* 0024 */ int16_t *mesh_ptrs[15];
    /* 0060 */ ITEM_INFO *target;
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
    /* 001A */ SG_COL *bgnd_gour;
    /* 001E */ int16_t outl_colour;
    /* 0020 */ SG_COL *outl_gour;
    /* 0024 */ int16_t bgnd_size_x;
    /* 0026 */ int16_t bgnd_size_y;
    /* 0028 */ int16_t bgnd_off_x;
    /* 002A */ int16_t bgnd_off_y;
    /* 002C */ int16_t bgnd_off_z;
    /* 002E */ int32_t scale_h;
    /* 0032 */ int32_t scale_v;
    /* 0034 */ char *string;
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

#pragma pop

#endif
