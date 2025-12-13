#pragma once

#include <trx/game/camera/enum.h>
#include <trx/game/types.h>

typedef struct {
    int16_t (*get_chase_speed_func)(void);
    void (*chase_func)(const ITEM *item);
    void (*combat_func)(const ITEM *item);
    void (*look_func)(const ITEM *item);
    void (*fixed_func)(void);
    void (*clamp_result_func)(void);
    void (*reset_func)(void);
} CAMERA_STRATEGY;

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
    int16_t actual_angle;
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
    GAME_VECTOR mic_pos;

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
        int16_t target_angle;
    } position;
} CINE_DATA;
