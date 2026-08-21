#pragma once

#include <trx/core/colors.h>
#include <trx/core/math.h>
#include <trx/game/items/const.h>
#include <trx/game/rooms/enum.h>
#include <trx/game/types.h>

#define ROOM_DRAWSET_WORDS (MAX_ITEMS / 64)

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
    int16_t sequence_num;
    bool one_shot;
} TRIGGER_FLYBY_DATA;

typedef struct {
    bool enabled;
    TRIGGER_TYPE type;
    int8_t timer;
    int16_t mask;
    bool one_shot;
    int16_t item_index;
    TRIGGER_CMD *command;
} TRIGGER;

typedef struct {
    ITEM *camera_item;
    bool switch_off;
    // The flip group a trigger asked to move, or -1 for none.
    int32_t flip_group;
    bool flip_available;
    int32_t new_effect;
    bool is_heavy;
    int32_t heavy_mask;
} TRIGGER_STATUS;

// A flip-map slot's accumulated trigger state. The mask is the 0..31 editor
// mask; only the savegame packs it back into the released save word.
typedef struct {
    uint8_t mask;
    bool is_one_shot;
} FLIP_SLOT;

// The lean description a flip-slot trigger acts on, mapped from floordata at
// the trigger-handler boundary, as with ITEM_TRIGGER.
typedef struct {
    bool from_switch;
    uint16_t mask;
    bool one_shot;
} FLIP_TRIGGER;

typedef struct {
    int16_t room_num;
    XYZ_16 normal;
    XYZ_16 vertex[4];
    BOUNDS_32 bounds;
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
    int16_t tilts[4];
    int32_t h1;
    int32_t h2;
} SPLIT;

typedef struct {
    SURFACE_TYPE type;
    int32_t height;
    bool is_split;
    union {
        XZ_16 tilt;
        SPLIT split;
    };
} SURFACE;

typedef struct {
    uint16_t idx;
    int16_t box;
    bool is_death_sector;
    LADDER_DIRECTION ladder;
    MINE_CART_TYPE mine_cart_type;
    TRIGGER *trigger;
    WALKABLE *walkable;
    struct {
        int16_t pit;
        int16_t sky;
        int16_t wall;
    } portal_room;
    SURFACE floor;
    SURFACE ceiling;
    uint8_t fx;
    bool stopper;
} SECTOR;

typedef struct {
    SHADE shade;
    FALLOFF falloff;
    XYZ_16 dir;
} LIGHT_LEGACY_DATA;

typedef struct {
    int32_t intensity;
    float inner_radius;
    float outer_radius;
    float length;
    float cutoff;
    XYZ_F dir;
} LIGHT_TR4_DATA;

typedef enum {
    LIGHT_LAYOUT_LEGACY,
    LIGHT_LAYOUT_TR4,
} LIGHT_LAYOUT;

typedef enum {
    LIGHT_TYPE_SUN = 0,
    LIGHT_TYPE_POINT = 1,
    LIGHT_TYPE_SPOT = 2,
    LIGHT_TYPE_SHADOW = 3,
    LIGHT_TYPE_FOG_BULB = 4,
} LIGHT_TYPE;

typedef struct {
    XYZ_32 pos;
    RGB_888 color;
    LIGHT_LAYOUT layout;
    LIGHT_TYPE type;
    union {
        LIGHT_LEGACY_DATA legacy;
        LIGHT_TR4_DATA tr4;
    } u;
} LIGHT;

typedef struct {
    XYZ_16 pos;
    RGBA_8888 color;
    int16_t light_base;
    uint8_t light_table_value;
    struct {
        bool disable_wibble;
        bool move;
        bool glow;
    } flags;
} ROOM_VERTEX;

typedef struct {
    uint16_t texture;
    uint16_t vertex;
} ROOM_SPRITE;

typedef struct {
    int16_t num_vertices;
    struct {
        int16_t count;
        FACE *data;
    } all_faces, face4s, face3s;
    struct {
        int16_t count;
        ROOM_SPRITE *data;
    } sprites;
    ROOM_VERTEX *vertices;
} ROOM_MESH;

typedef struct {
    XYZ_32 pos;
    struct {
        int16_t y;
    } rot;
    RGBA_8888 color;
    SHADE shade;
    int16_t static_num;
    int16_t draw_num;
    // The room that holds the mesh. A mesh that reaches through a portal is
    // lent to the room on the other side, so the room that draws it is not
    // always the room it stands in.
    int16_t room_num;
} STATIC_MESH;

typedef struct {
    uint64_t bits[ROOM_DRAWSET_WORDS];
    uint16_t count;
} ROOM_DRAWSET;

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
    RGB_888 ambient_rgb; // TR4 room ambient; grayscale of `ambient` otherwise
    ROOM_LIGHT_MODE light_mode;
    int16_t num_lights;
    int16_t num_static_meshes;
    int16_t item_num;
    int16_t effect_num;
    int16_t flipped_room;
    ROOM_FLIP_STATUS flip_status;
    struct {
        bool underwater;
        bool outside;
        bool wind;
        bool inside;
        bool dynamic_lit;
        bool swamp;
        bool damaging;
        bool cold;
        bool no_lens_flare;
    } flags;

    ROOM_DRAWSET drawn_items;
    uint8_t water_scheme;
    uint8_t reverb_info;
    uint8_t alternate_group;
} ROOM;
