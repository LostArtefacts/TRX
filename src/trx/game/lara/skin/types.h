#pragma once

#include <trx/game/gun/const.h>
#include <trx/game/lara/skin/enum.h>
#include <trx/game/lara/skin/seam.h>
#include <trx/game/lara/types.h>
#include <trx/game/sound/ids.h>

typedef struct {
    int32_t right;
    int32_t left;
} MESH_PAIR;

typedef struct {
    MESH_PAIR hand;
    MESH_PAIR thigh;
    int32_t torso;
} LARA_SKIN_MESH_MAP;

typedef struct {
    LARA_SKIN_MESH_MAP mesh_offsets[MAX_WEAPONS];
} LARA_SKIN_GUN_MAP;

// The braid top ring welds onto these head vertices so it meets the scalp
// instead of hanging off it. vertex_a is a braid segment-0 vertex, vertex_b
// the head vertex it pins to. Authored per outfit: the two rings sit a gap
// apart at rest, so no position match finds them, and the vertices differ
// between outfits and between the two pigtails.
typedef struct {
    int32_t count;
    SEAM_VERTEX_PAIR pairs[SEAM_MAX_VERTEX_PAIRS];
} LARA_SKIN_BRAID_HEAD_SEAM;

typedef struct {
    LARA_SKIN_BRAID_MODE mode;
    bool auto_enabled;
    struct {
        int32_t mesh_offset;
        XYZ_32 position;
        LARA_SKIN_BRAID_HEAD_SEAM head_seam;
    } setup[2];
    int32_t count;
} LARA_SKIN_BRAID;

typedef struct {
    bool is_defined;
    OBJECT_ID mesh_obj_id;
    OBJECT_ID joints_obj_id;
    OBJECT_ID extra_obj_id;
    OBJECT_ID guns_obj_id;
    OBJECT_ID legs_obj_id;
    LARA_SKIN_GUN_MAP *gun_map;
    LARA_SKIN_BRAID braid;
    bool is_selectable;
    // Set on the gilded form of an outfit, never read from the config.
    bool is_gold;
    RGB_888 gold_color;
    bool supports_sunglasses;
    bool is_barefoot;
    int32_t combat_face_offset;
    int32_t speech_face_offset;
    MESH_PAIR no_holster_offsets;
    int32_t extra_outfits[LS_EXTRA_NUMBER_OF];
    XYZ_16 extra_mesh_positions[NUM_EXTRA_MESHES];
} LARA_SKIN_OUTFIT;

typedef struct {
    LARA_SKIN_EQUIPMENT_TYPE type;
    int32_t data;
    bool visible;
    const OBJECT_MESH *mesh;
    XYZ_16 offset;
} LARA_SKIN_EQUIPMENT;
