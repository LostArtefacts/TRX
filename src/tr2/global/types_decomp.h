#pragma once

#include <libtrx/game/camera/enum.h>
#include <libtrx/game/camera/types.h>
#include <libtrx/game/gun/types.h>
#include <libtrx/game/items.h>
#include <libtrx/game/math.h>
#include <libtrx/game/rooms/types.h>
#include <libtrx/game/types.h>

#include <stdint.h>

// clang-format off
#pragma pack(push, 1)

typedef struct {
    XYZ_32 pos;
    XYZ_16 rot;
} PHD_3DPOS;

typedef struct {
    float xv;
    float yv;
    float zv;
    float rhw;
    float xs;
    float ys;
    int16_t clip;
    int16_t g;
    struct {
        int16_t u;
        int16_t v;
        float z;
        float w;
    } tex;
} PHD_VBUF;

typedef enum {
    SHAPE_SPRITE = 1,
    SHAPE_LINE   = 2,
    SHAPE_BOX    = 3,
    SHAPE_FBOX   = 4,
} SHAPE;

typedef enum {
    SPRF_RGB       = 0x00FFFFFF,
    SPRF_ABS       = 0x01000000,
    SPRF_SEMITRANS = 0x02000000,
    SPRF_SCALE     = 0x04000000,
    SPRF_SHADE     = 0x08000000,
} SPRITE_FLAG;

typedef struct {
    float xv;
    float yv;
    float zv;
    float rhw;
    float xs;
    float ys;
    struct {
        float u;
        float v;
        float z;
        float w;
    } tex;
    float g;
} POINT_INFO;

typedef struct {
    void *_0;
    int32_t _1;
} SORT_ITEM;

typedef enum {
    GFE_PICTURE          = 0,
    GFE_LIST_START       = 1,
    GFE_LIST_END         = 2,
    GFE_PLAY_FMV         = 3,
    GFE_START_LEVEL      = 4,
    GFE_CUTSCENE         = 5,
    GFE_LEVEL_COMPLETE   = 6,
    GFE_DEMO_PLAY        = 7,
    GFE_JUMP_TO_SEQ      = 8,
    GFE_END_SEQ          = 9,
    GFE_SET_TRACK        = 10,
    GFE_SUNSET           = 11,
    GFE_LOADING_PIC      = 12,
    GFE_DEADLY_WATER     = 13,
    GFE_REMOVE_WEAPONS   = 14,
    GFE_GAME_COMPLETE    = 15,
    GFE_CUT_ANGLE        = 16,
    GFE_NO_FLOOR         = 17,
    GFE_ADD_TO_INV       = 18,
    GFE_START_ANIM       = 19,
    GFE_NUM_SECRETS      = 20,
    GFE_KILL_TO_COMPLETE = 21,
    GFE_REMOVE_AMMO      = 22,
} GF_EVENTS;

typedef struct {
    uint16_t key[14]; // INPUT_ROLE_NUMBER_OF
} CONTROL_LAYOUT;

typedef struct {
    int32_t boat_turn;
    int32_t left_fallspeed;
    int32_t right_fallspeed;
    int16_t tilt_angle;
    int16_t extra_rotation;
    int32_t water;
    int32_t pitch;
} BOAT_INFO;

typedef struct {
    int16_t track_mesh;
    int32_t skidoo_turn;
    int32_t left_fallspeed;
    int32_t right_fallspeed;
    int16_t momentum_angle;
    int16_t extra_rotation;
    int32_t pitch;
} SKIDOO_INFO;

typedef struct {
    int32_t xv;
    int32_t yv;
    int32_t zv;
} PORTAL_VBUF;

typedef struct {
    int32_t table[32]; // WIBBLE_SIZE
} ROOM_LIGHT_TABLE;

#pragma pack(pop)

// clang-format on
