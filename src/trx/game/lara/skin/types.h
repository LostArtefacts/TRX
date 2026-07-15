#pragma once

#include <trx/game/lara/skin/enum.h>
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
    LARA_SKIN_MESH_MAP mesh_offsets[NUM_WEAPONS];
} LARA_SKIN_GUN_MAP;

typedef struct {
    LARA_SKIN_BRAID_MODE mode;
    bool enabled;
    int32_t mesh_offset;
    int32_t gold_offset;
    XYZ_32 positions[2];
    int32_t count;
} LARA_SKIN_BRAID;

typedef struct {
    bool is_defined;
    OBJECT_ID mesh_obj_id;
    OBJECT_ID joints_obj_id;
    LARA_SKIN_GUN_MAP *gun_map;
    LARA_SKIN_BRAID braid;
    bool is_selectable;
    bool is_reflective;
    bool supports_sunglasses;
    bool is_barefoot;
    int32_t combat_face_offset;
    int32_t speech_face_offset;
    MESH_PAIR no_holster_offsets;
    int32_t extra_outfits[LS_EXTRA_NUMBER_OF];
} LARA_SKIN_OUTFIT;

typedef struct {
    LARA_SKIN_EQUIPMENT_TYPE type;
    int32_t data;
    bool visible;
    const OBJECT_MESH *mesh;
} LARA_SKIN_EQUIPMENT;
