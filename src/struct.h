#ifndef TR1MAIN_STRUCT_H
#define TR1MAIN_STRUCT_H

#include <stdio.h>
#include <stdint.h>

typedef uint16_t PHD_ANGLE;
typedef void UNKNOWN_STRUCT;

#define LARA_HITPOINTS 1000
#define LARA_AIR       1800

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
    GBUF_RoomInfos           = 11,
    GBUF_RoomMesh            = 12,
    GBUF_RoomDoor            = 13,
    GBUF_RoomFloor           = 14,
    GBUF_RoomLights          = 15,
    GBUF_RoomStaticMeshInfos = 16,
    GBUF_FloorData           = 17,
} GAMEALLOC_BUFFER;

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
    /* 0000 */ uint16_t *data;
    /* 0004 */ DOOR_INFOS *doors;
    /* 0008 */ FLOOR_INFO *floor;
    /* 000C */ LIGHT_INFO *light;
    /* 0010 */ MESH_INFO *mesh;
    /* 0014 */ uint32_t x;
    /* 0018 */ uint32_t y;
    /* 001C */ uint32_t z;
    /* 0020 */ uint32_t min_floor;
    /* 0024 */ uint32_t max_ceiling;
    /* 0028 */ uint16_t x_size;
    /* 002A */ uint16_t y_size;
    /* 002C */ uint16_t ambient;
    /* 002E */ uint16_t num_lights;
    /* 0030 */ uint16_t num_meshes;
    /* 0032 */ uint16_t bound_left;
    /* 0034 */ uint16_t bound_right;
    /* 0036 */ uint16_t bound_top;
    /* 0038 */ uint16_t bound_bottom;
    /* 003A */ uint16_t bound_active;
    /* 003C */ uint16_t item_number;
    /* 003E */ uint16_t fx_number;
    /* 0040 */ uint16_t flipped_room;
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
    /* 0000 */ uint32_t floor;
    /* 0004 */ uint32_t touch_bits;
    /* 0008 */ uint32_t mesh_bits;
    /* 000C */ uint16_t object_id;
    /* 000E */ uint16_t current_anim_state;
    /* 0010 */ uint16_t goal_anim_state;
    /* 0012 */ uint16_t required_anim_state;
    /* 0014 */ uint16_t anim_number;
    /* 0016 */ uint16_t frame_number;
    /* 0018 */ uint16_t room_number;
    /* 001A */ uint16_t next_item;
    /* 001C */ uint16_t next_active;
    /* 001E */ uint16_t speed;
    /* 0020 */ uint16_t fall_speed;
    /* 0022 */ uint16_t hit_points;
    /* 0024 */ uint16_t box_number;
    /* 0026 */ uint16_t timer;
    /* 0028 */ uint16_t flags;
    /* 002A */ uint16_t shade1;
    /* 002C */ void *data;
    /* 0030 */ PHD_3DPOS pos;
    /* 0042 */ uint16_t more_flags;
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

#pragma pop

#endif
