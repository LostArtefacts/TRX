#pragma once

#include <trx/game/camera/enum.h>
#include <trx/game/types.h>

typedef struct {
    int16_t head_turn;
    int16_t max_head_rotation;
    int16_t min_head_rotation;
    int16_t max_head_tilt;
    int16_t min_head_tilt;
    float torso_head_rot_y;
    float torso_head_rot_x;
} CAMERA_LOOK_SETTINGS;

typedef struct {
    int16_t (*get_chase_speed_func)(void);
    const CAMERA_LOOK_SETTINGS *(*get_look_settings_func)(bool on_surface);
    void (*clamp_result_func)(void);
    void (*reset_func)(void);
    void (*update_func)(const ITEM *item, bool fixed_camera, int32_t target_y);
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

typedef struct {
    uint8_t sequence;
    uint8_t index;
    XYZ_32 pos;
    XYZ_32 target;
    int32_t room_num;
    int16_t fov;
    int16_t roll;
    int16_t timer;
    int16_t speed;
    struct {
        bool snap_from_game;
        bool target_item;
        bool loop;
        bool track_path;
        bool focus_lara;
        bool target_lara;
        bool snap_to_game;
        bool jump_to_camera;
        bool hold;
        bool no_break;
        bool lara_control_off;
        bool lara_control_on;
        bool fade_in_screen;
        bool fade_out_screen;
        bool test_triggers;
        bool one_shot;
    } flags;
} FLYBY_CAMERA;
