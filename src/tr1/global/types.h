#pragma once

#include <libtrx/game/anims.h>
#include <libtrx/game/camera/enum.h>
#include <libtrx/game/camera/types.h>
#include <libtrx/game/collision.h>
#include <libtrx/game/creature.h>
#include <libtrx/game/effects/types.h>
#include <libtrx/game/game_flow/enum.h>
#include <libtrx/game/game_flow/types.h>
#include <libtrx/game/gun/types.h>
#include <libtrx/game/items.h>
#include <libtrx/game/lara/types.h>
#include <libtrx/game/math.h>
#include <libtrx/game/matrix.h>
#include <libtrx/game/objects/common.h>
#include <libtrx/game/output.h>
#include <libtrx/game/pathing.h>
#include <libtrx/game/rooms.h>
#include <libtrx/game/sound/enum.h>
#include <libtrx/game/sound/ids.h>
#include <libtrx/game/types.h>

#include <stdint.h>

typedef enum {
    D_TRANS1 = 1,
    D_TRANS2 = 2,
    D_TRANS3 = 3,
    D_TRANS4 = 4,
    D_NEXT = 1 << 3,
} D_FLAGS;

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
    union {
        struct {
            float u, v, z, w;
        };
        float tex_coord[4];
    };
} PHD_VBUF;

typedef struct {
    PASSPORT_MODE passport_selection;
    int32_t select_save_slot;
    int32_t select_level_num;
    bool ask_for_save;
} GAME_INFO;

typedef struct {
    int32_t xv;
    int32_t yv;
    int32_t zv;
} DOOR_VBUF;
