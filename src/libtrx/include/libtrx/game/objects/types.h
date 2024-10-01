#pragma once

#include "../types.h"

#include <stdint.h>

#if TR_VERSION == 1
typedef struct __PACKING {
    struct __PACKING {
        XYZ_16 min;
        XYZ_16 max;
    } shift, rot;
} OBJECT_BOUNDS;

typedef struct __PACKING {
    int16_t nmeshes;
    int16_t mesh_idx;
    int32_t bone_idx;
    FRAME_INFO *frame_base;
    void (*initialise)(int16_t item_num);
    void (*control)(int16_t item_num);
    int16_t (*floor_height_func)(
        const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
    int16_t (*ceiling_height_func)(
        const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
    void (*draw_routine)(ITEM *item);
    void (*collision)(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
    const OBJECT_BOUNDS *(*bounds)(void);
    int16_t anim_idx;
    int16_t hit_points;
    int16_t pivot_length;
    int16_t radius;
    int16_t smartness;
    int16_t shadow_size;
    uint16_t loaded : 1;
    uint16_t intelligent : 1;
    uint16_t save_position : 1;
    uint16_t save_hitpoints : 1;
    uint16_t save_flags : 1;
    uint16_t save_anim : 1;
} OBJECT;

#elif TR_VERSION == 2
typedef struct __PACKING {
    int16_t mesh_count;
    int16_t mesh_idx;
    int32_t bone_idx;
    int16_t *frame_base; // TODO: make me FRAME_INFO

    void (*initialise)(int16_t item_num);
    void (*control)(int16_t item_num);
    void (*floor)(
        const ITEM *item, int32_t x, int32_t y, int32_t z, int32_t *height);
    void (*ceiling)(
        const ITEM *item, int32_t x, int32_t y, int32_t z, int32_t *height);
    void (*draw_routine)(const ITEM *item);
    void (*collision)(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);

    int16_t anim_idx;
    int16_t hit_points;
    int16_t pivot_length;
    int16_t radius;
    int16_t shadow_size;

    union {
        uint16_t flags;
        // clang-format off
        struct __PACKING {
            uint16_t loaded:           1; // 0x01 1
            uint16_t intelligent:      1; // 0x02 2
            uint16_t save_position:    1; // 0x04 4
            uint16_t save_hitpoints:   1; // 0x08 8
            uint16_t save_flags:       1; // 0x10 16
            uint16_t save_anim:        1; // 0x20 32
            uint16_t semi_transparent: 1; // 0x40 64
            uint16_t water_creature:   1; // 0x80 128
            uint16_t pad:              8;
        };
        // clang-format on
    };
} OBJECT;
#endif
