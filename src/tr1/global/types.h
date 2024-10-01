#pragma once

#include "global/const.h"

#include <libtrx/game/anims.h>
#include <libtrx/game/collision.h>
#include <libtrx/game/creature.h>
#include <libtrx/game/effects/types.h>
#include <libtrx/game/gameflow/types.h>
#include <libtrx/game/items.h>
#include <libtrx/game/lara/types.h>
#include <libtrx/game/lot.h>
#include <libtrx/game/math.h>
#include <libtrx/game/objects/common.h>
#include <libtrx/game/rooms.h>
#include <libtrx/game/sound/ids.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef int16_t PHD_ANGLE;

typedef enum {
    SAMPLE_FLAG_NO_PAN = 1 << 12,
    SAMPLE_FLAG_PITCH_WIBBLE = 1 << 13,
    SAMPLE_FLAG_VOLUME_WIBBLE = 1 << 14,
} SAMPLE_FLAG;

typedef enum {
    CAM_CHASE = 0,
    CAM_FIXED = 1,
    CAM_LOOK = 2,
    CAM_COMBAT = 3,
    CAM_CINEMATIC = 4,
    CAM_HEAVY = 5,
} CAMERA_TYPE;

typedef enum {
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

typedef enum {
    MX_INACTIVE = -1,
    MX_UNUSED_0 = 0,
    MX_UNUSED_1 = 1,
    MX_TR_THEME = 2,
    MX_WHERE_THE_DEPTHS_UNFOLD_1 = 3,
    MX_TR_THEME_ALT_1 = 4,
    MX_UNUSED_2 = 5,
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
    MX_CAVES_AMBIENCE = 57,
    MX_CISTERN_AMBIENCE = 58,
    MX_WINDY_AMBIENCE = 59,
    MX_ATLANTIS_AMBIENCE = 60,
    MX_NUMBER_OF,
} MUSIC_TRACK_ID;

typedef enum {
    TARGET_NONE = 0,
    TARGET_PRIMARY = 1,
    TARGET_SECONDARY = 2,
} TARGET_TYPE;

typedef enum {
    D_TRANS1 = 1,
    D_TRANS2 = 2,
    D_TRANS3 = 3,
    D_TRANS4 = 4,
    D_NEXT = 1 << 3,
} D_FLAGS;

typedef enum {
    COLL_NONE = 0,
    COLL_FRONT = 1,
    COLL_LEFT = 2,
    COLL_RIGHT = 4,
    COLL_TOP = 8,
    COLL_TOPFRONT = 16,
    COLL_CLAMP = 32,
} COLL_TYPE;

typedef enum {
    HT_WALL = 0,
    HT_SMALL_SLOPE = 1,
    HT_BIG_SLOPE = 2,
} HEIGHT_TYPE;

typedef enum {
    AC_NULL = 0,
    AC_MOVE_ORIGIN = 1,
    AC_JUMP_VELOCITY = 2,
    AC_ATTACK_READY = 3,
    AC_DEACTIVATE = 4,
    AC_SOUND_FX = 5,
    AC_EFFECT = 6,
} ANIM_COMMAND;

typedef enum {
    BEB_POP = 1 << 0,
    BEB_PUSH = 1 << 1,
    BEB_ROT_X = 1 << 2,
    BEB_ROT_Y = 1 << 3,
    BEB_ROT_Z = 1 << 4,
} BONE_EXTRA_BITS;

typedef enum {
    RF_UNDERWATER = 1,
} ROOM_FLAG;

typedef enum {
    FT_FLOOR = 0,
    FT_DOOR = 1,
    FT_TILT = 2,
    FT_ROOF = 3,
    FT_TRIGGER = 4,
    FT_LAVA = 5,
} FLOOR_TYPE;

typedef enum {
    INV_GAME_MODE = 0,
    INV_TITLE_MODE = 1,
    INV_KEYS_MODE = 2,
    INV_SAVE_MODE = 3,
    INV_LOAD_MODE = 4,
    INV_DEATH_MODE = 5,
    INV_SAVE_CRYSTAL_MODE = 6,
} INV_MODE;

typedef enum {
    IT_NAME = 0,
    IT_QTY = 1,
    IT_NUMBER_OF = 2,
} INV_TEXT;

typedef enum {
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

typedef enum {
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

typedef enum {
    RT_MAIN = 0,
    RT_OPTION = 1,
    RT_KEYS = 2,
} RING_TYPE;

typedef enum {
    CM_PICK = 0,
    CM_KEYBOARD = 1,
    CM_CONTROLLER = 2,
} CONTROL_MODE;

typedef enum {
    SHAPE_SPRITE = 1,
    SHAPE_LINE = 2,
    SHAPE_BOX = 3,
    SHAPE_FBOX = 4
} SHAPE;

typedef enum {
    DOOR_CLOSED = 0,
    DOOR_OPEN = 1,
} DOOR_ANIM;

typedef enum {
    TRAP_SET = 0,
    TRAP_ACTIVATE = 1,
    TRAP_WORKING = 2,
    TRAP_FINISHED = 3,
} TRAP_ANIM;

typedef enum {
    SPS_START = 0,
    SPS_END = 1,
    SPS_MOVING = 2,
} SLIDING_PILLAR_STATE;

typedef enum {
    GFS_END = -1,
    GFS_START_GAME,
    GFS_LOOP_GAME,
    GFS_STOP_GAME,
    GFS_START_CINE,
    GFS_LOOP_CINE,
    GFS_PLAY_FMV,
    GFS_LEVEL_STATS,
    GFS_TOTAL_STATS,
    GFS_LOADING_SCREEN,
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
    GFS_REMOVE_AMMO,
    GFS_REMOVE_MEDIPACKS,
    GFS_SETUP_BACON_LARA,
    GFS_LEGACY,
} GAMEFLOW_SEQUENCE_TYPE;

typedef enum {
    BT_LARA_HEALTH = 0,
    BT_LARA_MAX_AIR = 1,
    BT_ENEMY_HEALTH = 2,
    BT_PROGRESS = 3,
} BAR_TYPE;

typedef enum {
    SPM_NORMAL = 0,
    SPM_UNDERWATER = 1,
    SPM_ALWAYS = 2,
} SOUND_PLAY_MODE;

typedef enum {
    UMM_FULL,
    UMM_QUIET,
    UMM_FULL_NO_AMBIENT,
    UMM_QUIET_NO_AMBIENT,
    UMM_NONE,
} UNDERWATER_MUSIC_MODE;

typedef enum {
    GBF_NGPLUS = 1 << 0,
    GBF_JAPANESE = 1 << 1,
} GAME_BONUS_FLAG;

typedef enum {
    PAGE_1 = 0,
    PAGE_2 = 1,
    PAGE_3 = 2,
    PAGE_COUNT = 3,
} PASSPORT_PAGE;

typedef enum {
    PASSPORT_MODE_BROWSE = 0,
    PASSPORT_MODE_LOAD_GAME = 1,
    PASSPORT_MODE_SELECT_LEVEL = 2,
    PASSPORT_MODE_STORY_SO_FAR = 3,
    PASSPORT_MODE_SAVE_GAME = 4,
    PASSPORT_MODE_NEW_GAME = 5,
    PASSPORT_MODE_RESTART = 6,
    PASSPORT_MODE_EXIT_TITLE = 7,
    PASSPORT_MODE_EXIT_GAME = 8,
    PASSPORT_MODE_UNAVAILABLE = 9,
} PASSPORT_MODE;

typedef struct {
    int width;
    int height;
} RESOLUTION;

typedef struct {
    float r;
    float g;
    float b;
} RGB_F;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RGB_888;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} RGBA_8888;

typedef struct {
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

typedef struct {
    float xv;
    float yv;
    float zv;
    float xs;
    float ys;
    float u;
    float v;
    float g;
} POINT_INFO;

typedef struct {
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

typedef struct {
    uint16_t u;
    uint16_t v;
} PHD_UV;

typedef struct {
    uint16_t drawtype;
    uint16_t tpage;
    PHD_UV uv[4];
} PHD_TEXTURE;

typedef struct {
    uint16_t tpage;
    uint16_t offset;
    uint16_t width;
    uint16_t height;
    int16_t x1;
    int16_t y1;
    int16_t x2;
    int16_t y2;
} PHD_SPRITE;

typedef struct TEXTURE_RANGE {
    int16_t num_textures;
    int16_t *textures;
    struct TEXTURE_RANGE *next_range;
} TEXTURE_RANGE;

typedef struct {
    SECTOR *sector;
    SECTOR old_sector;
    int16_t block;
} DOORPOS_DATA;

typedef struct {
    DOORPOS_DATA d1;
    DOORPOS_DATA d1flip;
    DOORPOS_DATA d2;
    DOORPOS_DATA d2flip;
} DOOR_DATA;

typedef struct {
    int16_t tx;
    int16_t ty;
    int16_t tz;
    int16_t cx;
    int16_t cy;
    int16_t cz;
    int16_t fov;
    int16_t roll;
} CINE_CAMERA;

typedef struct {
    XYZ_32 pos;
    XYZ_16 rot;
} CINE_POSITION;

typedef struct {
    uint32_t timer;
    uint32_t death_count;
    uint32_t kill_count;
    uint16_t secret_flags;
    uint8_t pickup_count;
    uint32_t max_kill_count;
    uint16_t max_secret_count;
    uint8_t max_pickup_count;
} GAME_STATS;

typedef struct {
    int32_t lara_hitpoints;
    uint16_t pistol_ammo;
    uint16_t magnum_ammo;
    uint16_t uzi_ammo;
    uint16_t shotgun_ammo;
    uint8_t num_medis;
    uint8_t num_big_medis;
    uint8_t num_scions;
    int8_t gun_status;
    LARA_GUN_TYPE equipped_gun_type;
    LARA_GUN_TYPE holsters_gun_type;
    LARA_GUN_TYPE back_gun_type;
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

typedef struct {
    RESUME_INFO *current;
    uint8_t bonus_flag;
    bool bonus_level_unlock;
    int32_t current_save_slot;
    int16_t save_initial_version;
    PASSPORT_MODE passport_selection;
    int32_t select_level_num;
    bool death_counter_supported;
    GAMEFLOW_LEVEL_TYPE current_level_type;
    GAMEFLOW_COMMAND override_gf_command;
    bool remove_guns;
    bool remove_scions;
    bool remove_ammo;
    bool remove_medipacks;
    bool inv_showing_medpack;
    bool inv_ring_above;
    bool ask_for_save;
} GAME_INFO;

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

typedef enum {
    BSM_DEFAULT,
    BSM_FLASHING_OR_DEFAULT,
    BSM_FLASHING_ONLY,
    BSM_ALWAYS,
    BSM_NEVER,
    BSM_PS1,
    BSM_BOSS_ONLY,
} BAR_SHOW_MODE;

typedef enum {
    BL_TOP_LEFT,
    BL_TOP_CENTER,
    BL_TOP_RIGHT,
    BL_BOTTOM_LEFT,
    BL_BOTTOM_CENTER,
    BL_BOTTOM_RIGHT,
    BL_CUSTOM,
} BAR_LOCATION;

typedef struct {
    BAR_TYPE type;
    int32_t value;
    int32_t max_value;
    bool show;
    BAR_SHOW_MODE show_mode;
    bool blink;
    int32_t timer;
    int32_t color;
    BAR_LOCATION location;
    int16_t custom_x;
    int16_t custom_y;
    int16_t custom_width;
    int16_t custom_height;
} BAR_INFO;

typedef struct {
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
            uint32_t progress_bar : 1;
            uint32_t multiline : 1;
            uint32_t manual_draw : 1;
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
    BAR_INFO progress_bar;
} TEXTSTRING;

typedef struct {
    int16_t poly_count;
    int16_t vertex_count;
    struct {
        uint16_t x;
        uint16_t y;
        uint16_t z;
    } vertex[32];
} SHADOW_INFO;

typedef struct {
    bool loaded;
    int16_t nmeshes;
    int16_t mesh_num;
    int16_t flags;
    BOUNDS_16 p;
    BOUNDS_16 c;
} STATIC_INFO;

typedef struct {
    union {
        struct {
            int32_t x;
            int32_t y;
            int32_t z;
        };
        XYZ_32 pos;
    };
    int16_t room_num;
} GAME_VECTOR;

typedef struct {
    union {
        struct {
            int32_t x;
            int32_t y;
            int32_t z;
        };
        XYZ_32 pos;
    };
    int16_t data;
    int16_t flags;
} OBJECT_VECTOR;

typedef struct {
    GAME_VECTOR pos;
    GAME_VECTOR target;
    int32_t type;
    int32_t shift;
    int32_t flags;
    int32_t fixed_camera;
    int32_t bounce;
    bool underwater;
    int32_t target_distance;
    int32_t target_square;
    int16_t target_angle;
    int16_t target_elevation;
    int16_t box;
    int16_t number;
    int16_t last;
    int16_t timer;
    int16_t speed;
    int16_t roll;
    ITEM *item;
    ITEM *last_item;
    OBJECT_VECTOR *fixed;
    // used for the manual camera control
    int16_t additional_angle;
    int16_t additional_elevation;

    struct {
        struct {
            XYZ_32 target;
            XYZ_32 pos;
            int32_t shift;
        } result, prev;
        int16_t room_num;
    } interp;
} CAMERA_INFO;

typedef struct {
    int32_t xv;
    int32_t yv;
    int32_t zv;
} DOOR_VBUF;

typedef struct {
    PHD_ANGLE lock_angles[4];
    PHD_ANGLE left_angles[4];
    PHD_ANGLE right_angles[4];
    PHD_ANGLE aim_speed;
    PHD_ANGLE shot_accuracy;
    int32_t gun_height;
    int16_t damage;
    int32_t target_dist;
    int16_t recoil_frame;
    int16_t flash_time;
    int16_t sample_num;
} WEAPON_INFO;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t z;
    int32_t r;
} SPHERE;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t z;
    int32_t mesh_num;
} BITE;

typedef struct {
    int16_t zone_num;
    int16_t enemy_zone;
    int32_t distance;
    int32_t ahead;
    int32_t bite;
    int16_t angle;
    int16_t enemy_facing;
} AI_INFO;

typedef struct {
    int32_t left;
    int32_t right;
    int32_t top;
    int32_t bottom;
    int16_t height;
    int16_t overlap_index;
} BOX_INFO;

typedef struct {
    bool is_blocked;
    char *content_text;
    TEXTSTRING *content;
} REQUESTER_ITEM;

typedef struct {
    uint16_t items_used;
    uint16_t max_items;
    uint16_t requested;
    uint16_t vis_lines;
    uint16_t line_offset;
    uint16_t line_old_offset;
    uint16_t pix_width;
    uint16_t line_height;
    bool is_blockable;
    int16_t x;
    int16_t y;
    char *heading_text;
    TEXTSTRING *heading;
    TEXTSTRING *background;
    TEXTSTRING *moreup;
    TEXTSTRING *moredown;
    REQUESTER_ITEM *items;
} REQUEST_INFO;

typedef struct {
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

typedef struct {
    int16_t shape;
    int16_t x;
    int16_t y;
    int16_t z;
    int32_t param1;
    int32_t param2;
    int16_t sprnum;
} INVENTORY_SPRITE;

typedef struct {
    GAME_OBJECT_ID object_id;
    int16_t frames_total;
    int16_t current_frame;
    int16_t goal_frame;
    int16_t open_frame;
    int16_t anim_direction;
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

typedef struct {
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
    struct {
        XYZ_32 pos;
        XYZ_16 rot;
    } ringpos;
    struct {
        XYZ_32 pos;
        XYZ_16 rot;
    } camera;
    XYZ_32 light;
    IMOTION_INFO *imo;
} RING_INFO;

typedef struct {
    int16_t number;
    int16_t volume;
    int16_t randomness;
    int16_t flags;
} SAMPLE_INFO;

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
        uint64_t menu_up : 1;
        uint64_t menu_down : 1;
        uint64_t menu_left : 1;
        uint64_t menu_right : 1;
        uint64_t menu_confirm : 1;
        uint64_t menu_back : 1;
        uint64_t enter_console : 1;
        uint64_t change_target : 1;
    };
} INPUT_STATE;

typedef enum {
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
    INPUT_ROLE_ENTER_CONSOLE = 33,
    INPUT_ROLE_CHANGE_TARGET = 34,
    INPUT_ROLE_NUMBER_OF = 35,
} INPUT_ROLE;

typedef enum {
    INPUT_LAYOUT_DEFAULT,
    INPUT_LAYOUT_CUSTOM_1,
    INPUT_LAYOUT_CUSTOM_2,
    INPUT_LAYOUT_CUSTOM_3,
    INPUT_LAYOUT_NUMBER_OF,
} INPUT_LAYOUT;

typedef enum {
    BT_BUTTON = 0,
    BT_AXIS = 1,
} BUTTON_TYPE;

typedef uint16_t INPUT_SCANCODE;
typedef int16_t INPUT_BUTTON;

typedef struct {
    int32_t mesh_count;
    int32_t mesh_ptr_count;
    int32_t anim_count;
    int32_t anim_change_count;
    int32_t anim_range_count;
    int32_t anim_command_count;
    int32_t anim_bone_count;
    int32_t anim_frame_data_count;
    int32_t anim_frame_count;
    int32_t anim_frame_mesh_rot_count;
    int32_t *anim_frame_offsets;
    int32_t object_count;
    int32_t static_count;
    int32_t texture_count;
    int32_t texture_page_count;
    uint8_t *texture_palette_page_ptrs;
    RGBA_8888 *texture_rgb_page_ptrs;
    int16_t *floor_data;
    int32_t anim_texture_range_count;
    int32_t item_count;
    int32_t sprite_info_count;
    int32_t sprite_count;
    int32_t overlap_count;
    int32_t sample_info_count;
    int32_t sample_count;
    int32_t *sample_offsets;
    int32_t sample_data_size;
    char *sample_data;
    RGB_888 *palette;
    int32_t palette_size;
} LEVEL_INFO;

typedef enum {
    TB_UNSPECIFIED = -1,
    TB_OFF = 0,
    TB_ON = 1,
} TRISTATE_BOOL;

typedef enum {
    SCREENSHOT_FORMAT_JPEG,
    SCREENSHOT_FORMAT_PNG,
} SCREENSHOT_FORMAT;

typedef enum {
    UI_STYLE_PS1,
    UI_STYLE_PC,
} UI_STYLE;

typedef enum {
    BC_GOLD,
    BC_BLUE,
    BC_GREY,
    BC_RED,
    BC_SILVER,
    BC_GREEN,
    BC_GOLD2,
    BC_BLUE2,
    BC_PINK,
    BC_PURPLE,
} BAR_COLOR;

typedef enum {
    TLM_FULL,
    TLM_SEMI,
    TLM_NONE,
} TARGET_LOCK_MODE;
