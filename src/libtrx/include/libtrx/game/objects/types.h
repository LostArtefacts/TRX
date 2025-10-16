#pragma once

#include "../anims/types.h"
#include "../collision.h"
#include "../items/types.h"
#include "../pathing/types.h"
#include "../rooms/enum.h"
#include "../savegame/enum.h"
#include "../types.h"

#include <stdint.h>

typedef struct {
    const OBJECT_ID key_id;
    const OBJECT_ID value_id;
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

    float depth_adjustment;
    bool enable_reflections;
    bool disable_transparency_sort;
} OBJECT_MESH;

typedef struct {
    struct {
        XYZ_16 min;
        XYZ_16 max;
    } shift, rot;
    bool ignore_rot;
} OBJECT_BOUNDS;

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
    void (*activate_func)(ITEM *item);
    void (*handle_flip_func)(ITEM *item, ROOM_FLIP_STATUS flip_status);
    void (*handle_save_func)(ITEM *item, SAVEGAME_STAGE stage);
    const OBJECT_BOUNDS *(*bounds_func)(void);
    bool (*is_usable_func)(int16_t item_num);
    void (*add_walkable_func)(int16_t item_num);
    bool (*can_interpolate_func)(
        const ITEM *item, int32_t frame_a, int32_t frame_b);

    int16_t anim_idx;
    int16_t hit_points;
    int16_t pivot_length;
    int16_t radius;
    int16_t shadow_size;
    int16_t smartness;
    XYZ_BOOL base_rot;
    LOT_SETUP lot_setup;

    bool enable_interpolation;
    bool loaded;
    bool intelligent;
    bool save_position;
    bool save_hitpoints;
    bool save_flags;
    bool save_anim;
    bool semi_transparent;
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

typedef enum {
    TRAP_SET = 0,
    TRAP_ACTIVATE = 1,
    TRAP_WORKING = 2,
    TRAP_FINISHED = 3,
} TRAP_ANIM;
