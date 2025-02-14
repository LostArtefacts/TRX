#pragma once

#include "game/stats/types.h"
#include "global/const.h"

#include <libtrx/game/anims.h>
#include <libtrx/game/camera/enum.h>
#include <libtrx/game/camera/types.h>
#include <libtrx/game/collision.h>
#include <libtrx/game/creature.h>
#include <libtrx/game/effects/types.h>
#include <libtrx/game/game_flow/enum.h>
#include <libtrx/game/game_flow/types.h>
#include <libtrx/game/items.h>
#include <libtrx/game/lara/types.h>
#include <libtrx/game/lot.h>
#include <libtrx/game/math.h>
#include <libtrx/game/matrix.h>
#include <libtrx/game/objects/common.h>
#include <libtrx/game/output.h>
#include <libtrx/game/rooms.h>
#include <libtrx/game/sound/enum.h>
#include <libtrx/game/sound/ids.h>
#include <libtrx/game/text.h>
#include <libtrx/game/types.h>

#include <stdint.h>

typedef int16_t PHD_ANGLE;

typedef enum {
    SAMPLE_FLAG_NO_PAN = 1 << 12,
    SAMPLE_FLAG_PITCH_WIBBLE = 1 << 13,
    SAMPLE_FLAG_VOLUME_WIBBLE = 1 << 14,
} SAMPLE_FLAG;

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
    SHAPE_SPRITE = 1,
    SHAPE_LINE = 2,
    SHAPE_BOX = 3,
    SHAPE_FBOX = 4
} SHAPE;

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
    BT_LARA_HEALTH = 0,
    BT_LARA_MAX_AIR = 1,
    BT_ENEMY_HEALTH = 2,
    BT_PROGRESS = 3,
} BAR_TYPE;

typedef enum {
    GBF_NGPLUS = 1 << 0,
    GBF_JAPANESE = 1 << 1,
} GAME_BONUS_FLAG;

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
    SECTOR *sector;
    SECTOR old_sector;
    int16_t block;
} DOORPOS_DATA;

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
    LEVEL_STATS stats;
} RESUME_INFO;

typedef struct {
    RESUME_INFO *current;
    int32_t death_count;

    uint8_t bonus_flag;
    bool bonus_level_unlock;
    int16_t save_initial_version;
    PASSPORT_MODE passport_selection;
    int32_t select_save_slot;
    int32_t select_level_num;
    bool remove_guns;
    bool remove_scions;
    bool remove_ammo;
    bool remove_medipacks;

    // TODO: remove these from here and make it a responsibility of overlay.c
    bool inv_ring_shown;
    bool showing_demo;
    bool inv_showing_medpack;
    bool inv_ring_above;

    bool ask_for_save;
} GAME_INFO;

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
    int16_t line_offset;
    int16_t line_old_offset;
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
