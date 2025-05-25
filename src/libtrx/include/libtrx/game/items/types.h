#pragma once

#include "../math.h"
#include "../objects/ids.h"
#include "../output/types.h"
#include "./enum.h"

typedef struct CARRIED_ITEM {
    GAME_OBJECT_ID object_id;
    int16_t spawn_num;
    XYZ_32 pos;
    XYZ_16 rot;
    int16_t room_num;
    int16_t fall_speed;
    DROP_STATUS status;
    struct CARRIED_ITEM *next_item;
} CARRIED_ITEM;

typedef struct {
    int32_t floor;
    uint32_t touch_bits;
    uint32_t mesh_bits;
    GAME_OBJECT_ID object_id;
    int16_t current_anim_state;
    int16_t goal_anim_state;
    int16_t required_anim_state;
    int16_t anim_num;
    int16_t frame_num;
    int16_t room_num;
    int16_t next_item;
    int16_t next_active;
    int16_t speed;
    int16_t fall_speed;
    int16_t hit_points;
    int16_t box_num;
    int16_t timer;
    uint16_t flags;

    SHADE shade;
    void *data;
    void *priv;
    CARRIED_ITEM *carried_item;

    XYZ_32 pos;
    XYZ_16 rot;

    ITEM_STATUS status;
    bool enable_interpolation;
    bool enable_shadow;
    bool active;
    bool gravity;
    bool hit_status;
    bool collidable;
    uint16_t pad_1 : 1; // 0x0001
    uint16_t pad_2 : 2; // 0x0002…0x0004
    uint16_t pad_3 : 1; // 0x0008
    uint16_t pad_4 : 1; // 0x0010
    uint16_t pad_5 : 1; // 0x0020
    uint16_t looked_at : 1; // 0x0040
#if TR_VERSION == 1
    uint16_t pad : 9; // 0x0080…0x8000
#elif TR_VERSION == 2
    uint16_t dynamic_light : 1; // 0x0080
    uint16_t killed : 1; // 0x0100
    uint16_t pad : 7; // 0x0200…0x8000
#endif

    struct {
        struct {
            XYZ_32 pos;
            XYZ_16 rot;
        } result, prev;
    } interp;
} ITEM;
