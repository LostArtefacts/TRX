#pragma once

#include "../anims/types.h"
#include "../collision.h"
#include "../items/types.h"
#include "../types.h"

#include <stdint.h>

typedef struct {
    const GAME_OBJECT_ID key_id;
    const GAME_OBJECT_ID value_id;
} GAME_OBJECT_PAIR;

typedef struct {
    void *priv;
    XYZ_16 center;
    int32_t radius;
    int16_t num_lights;
    int16_t num_vertices;
    int16_t num_tex_face4s;
    int16_t num_tex_face3s;
    int16_t num_flat_face4s;
    int16_t num_flat_face3s;
    union {
        XYZ_16 *normals;
        int16_t *lights;
    } lighting;
    XYZ_16 *vertices;
    FACE4 *tex_face4s;
    FACE3 *tex_face3s;
    FACE4 *flat_face4s;
    FACE3 *flat_face3s;
    bool enable_reflections;
} OBJECT_MESH;

#if TR_VERSION == 1
typedef struct {
    struct {
        XYZ_16 min;
        XYZ_16 max;
    } shift, rot;
} OBJECT_BOUNDS;
#endif

typedef struct OBJECT {
    int16_t mesh_count;
    int16_t mesh_idx;
    int32_t bone_idx;
    uint32_t frame_ofs;
    ANIM_FRAME *frame_base;

    void (*setup_func)(struct OBJECT *obj);
    void (*initialise_func)(int16_t item_num);
    void (*control_func)(int16_t item_num);
    void (*draw_func)(const ITEM *item);
    void (*collision_func)(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
    int16_t (*floor_height_func)(
        const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
    int16_t (*ceiling_height_func)(
        const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
#if TR_VERSION == 1
    const OBJECT_BOUNDS *(*bounds_func)(void);
    bool (*is_usable_func)(int16_t item_num);
#endif

    int16_t anim_idx;
    int16_t hit_points;
    int16_t pivot_length;
    int16_t radius;
    int16_t shadow_size;
    int16_t smartness;

    union {
        uint16_t flags;
        // clang-format off
        struct {
            uint16_t loaded:           1; // 0x01 1
            uint16_t intelligent:      1; // 0x02 2
            uint16_t save_position:    1; // 0x04 4
            uint16_t save_hitpoints:   1; // 0x08 8
            uint16_t save_flags:       1; // 0x10 16
            uint16_t save_anim:        1; // 0x20 32
            uint16_t semi_transparent: 1; // 0x40 64
            uint16_t pad:              9;
        };
        // clang-format on
    };
} OBJECT;

typedef struct {
    bool loaded;
    int16_t mesh_idx;
    bool collidable;
    bool visible;
    BOUNDS_16 draw_bounds;
    BOUNDS_16 collision_bounds;
} STATIC_OBJECT_3D;

typedef struct {
    bool loaded;
    int16_t frame_count;
    int16_t texture_idx;
} STATIC_OBJECT_2D;
