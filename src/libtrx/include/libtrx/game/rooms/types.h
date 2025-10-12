#pragma once

#include "../math.h"
#include "../types.h"
#include "./enum.h"

typedef struct TRIGGER_CMD {
    TRIGGER_OBJECT type;
    void *parameter;
    struct TRIGGER_CMD *next_cmd;
} TRIGGER_CMD;

typedef struct {
    int16_t camera_num;
    uint8_t timer;
    uint8_t glide;
    bool one_shot;
} TRIGGER_CAMERA_DATA;

typedef struct {
    TRIGGER_TYPE type;
    int8_t timer;
    int16_t mask;
    bool one_shot;
    int16_t item_index;
    TRIGGER_CMD *command;
} TRIGGER;

typedef struct {
    int16_t room_num;
    XYZ_16 normal;
    XYZ_16 vertex[4];
} PORTAL;

typedef struct {
    uint16_t count;
    PORTAL portal[];
} PORTALS;

typedef struct WALKABLE {
    int16_t item_num;
    XYZ_32 pos;
    struct WALKABLE *next;
} WALKABLE;

typedef struct {
    SPLIT_TYPE type;
    uint8_t tilts[4];
    int8_t h1;
    int8_t h2;
} SPLIT;

typedef struct {
    SURFACE_TYPE type;
    int16_t height;
    bool is_split;
    union {
        int16_t tilt;
        SPLIT split;
    };
} SURFACE;

typedef struct {
    uint16_t idx;
    int16_t box;
    bool is_death_sector;
    LADDER_DIRECTION ladder;
    TRIGGER *trigger;
    WALKABLE *walkable;
    struct {
        int16_t pit;
        int16_t sky;
        int16_t wall;
    } portal_room;
    SURFACE floor;
    SURFACE ceiling;
} SECTOR;

typedef struct {
    XYZ_32 pos;
    SHADE shade;
    FALLOFF falloff;
} LIGHT;

typedef struct {
    XYZ_16 pos;
    int16_t light_base;
    int16_t light_adder;
    uint16_t flags;
    uint8_t light_table_value;
} ROOM_VERTEX;

typedef struct {
    uint16_t texture;
    uint16_t vertex;
} ROOM_SPRITE;

typedef struct {
    int16_t num_vertices;
    int16_t num_face4s;
    int16_t num_face3s;
    int16_t num_sprites;
    ROOM_VERTEX *vertices;
    FACE4 *face4s;
    FACE3 *face3s;
    ROOM_SPRITE *sprites;
} ROOM_MESH;

typedef struct {
    XYZ_32 pos;
    struct {
        int16_t y;
    } rot;
    SHADE shade;
    int16_t static_num;
} STATIC_MESH;

typedef struct {
    ROOM_MESH mesh;
    PORTALS *portals;
    SECTOR *sectors;
    LIGHT *lights;
    STATIC_MESH *static_meshes;
    XYZ_32 pos;
    int32_t min_floor;
    int32_t max_ceiling;
    struct {
        int16_t z;
        int16_t x;
    } size;
    int16_t ambient;
    ROOM_LIGHT_MODE light_mode;
    int16_t num_lights;
    int16_t num_static_meshes;
    int16_t bound_left;
    int16_t bound_right;
    int16_t bound_top;
    int16_t bound_bottom;
    uint16_t bound_active;
#if TR_VERSION == 2
    int16_t test_left;
    int16_t test_right;
    int16_t test_top;
    int16_t test_bottom;
#endif
    int16_t item_num;
    int16_t effect_num;
    int16_t flipped_room;
    ROOM_FLIP_STATUS flip_status;
    uint16_t flags;
} ROOM;
