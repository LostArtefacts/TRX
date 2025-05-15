#pragma once

#include "../types.h"
#include "./enum.h"

typedef struct {
    GAME_VECTOR pos;
    GAME_VECTOR target;
    CAMERA_TYPE type;

    int32_t shift;
    CAMERA_FLAGS flags;
    bool fixed_camera;
    int32_t bounce;
    bool underwater;
    int32_t target_distance;
    int32_t target_square;
    int16_t target_angle;
#if TR_VERSION == 2
    int16_t actual_angle;
#endif
    int16_t target_elevation;
    int16_t num;
    int16_t last;
    int16_t timer;
    int16_t speed;
    int16_t roll;
    ITEM *item;
    ITEM *last_item;

    int32_t debuff;

    // used for the manual camera control
    int16_t additional_angle;
    int16_t additional_elevation;
#if TR_VERSION == 2
    XYZ_32 mic_pos;
#endif

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
    struct {
        XYZ_16 shift;
    } target, camera;
    int16_t fov;
    int16_t roll;
} CINE_FRAME;

typedef struct {
    int16_t frame_idx;
    int16_t frame_count;
    struct {
        XYZ_32 pos;
        XYZ_16 rot;
#if TR_VERSION == 2
        int16_t target_angle;
#endif
    } position;
} CINE_DATA;
