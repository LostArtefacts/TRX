#pragma once

#include <trx/game/output/water.h>

// Defines each water family's strategy: TR1 to TR3 multiply submerged geometry
// by the water colour, while TR4 uses water-room lighting and reserves the
// water colour for underwater views.
typedef struct {
    WATER_EFFECTS (*get_effects)(
        bool is_below_water, bool is_camera_underwater);
    bool (*is_object_wibble_enabled)(void);
    void (*observe_lara_frame)(void);
    void (*observe_lara_mesh)(LARA_MESH mesh, GAME_VECTOR pos);
    WATER_LARA_MESH (*decide_lara_mesh)(
        LARA_MESH mesh, GAME_VECTOR pos, int32_t radius);
    const RGB_888 *(*get_lara_ambient)(LARA_MESH parent, LARA_MESH child);
    bool (*get_lara_ambient_span)(
        LARA_MESH parent, LARA_MESH child, RGB_888 *out_from);
} WATER_MODEL;

extern const WATER_MODEL g_WaterModelTR123;
extern const WATER_MODEL g_WaterModelTR4;
