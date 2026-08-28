#pragma once

#include <trx/core/colors.h>
#include <trx/game/lara/enum.h>
#include <trx/game/types.h>

#include <stdint.h>

typedef struct {
    bool shade;
    bool room_shade;
    bool wibble;
} WATER_EFFECTS;

typedef struct {
    RGBA_F tint;
    // Tints only the part of the mesh below the water surface; without a
    // surface, tints the whole mesh.
    bool has_surface;
    int32_t surface;
    // Shifts the ambient light below the water surface by this much, in
    // place of tinting that part. The shift applies on top of the ambient
    // light the light list produced, which the shadow lights darken.
    bool has_submerged_ambient;
    RGB_F submerged_ambient_delta;
} WATER_LARA_MESH;

// Identifies a Lara mesh outside her body, such as a gun flash.
#define WATER_LARA_MESH_OTHER LM_NUMBER_OF

// Sets the water effects for subsequently drawn geometry according to the
// water states of the room and camera.
void Output_Water_SetupAboveWater(bool is_camera_underwater);
void Output_Water_SetupBelowWater(bool is_camera_underwater);

// Reports whether objects receive the water colour.
bool Output_Water_IsShadeEnabled(void);
// Reports whether room geometry receives the water colour.
bool Output_Water_IsRoomShadeEnabled(void);
// Reports whether room geometry distorts: TR1 to TR3 distort water viewed from
// above and dry rooms viewed from below, while TR4 distorts every room when
// the camera is below the surface.
bool Output_Water_IsWibbleEnabled(void);
// Reports whether objects and static meshes distort.
bool Output_Water_IsObjectWibbleEnabled(void);
// Reports whether subsequently drawn geometry is in water, controlling
// caustics and water-surface movement.
bool Output_Water_IsSubmerged(void);

// Records Lara's water state once per frame in which she is drawn.
void Output_Water_ObserveLaraFrame(void);

// Records the water state of one Lara mesh from the position it is drawn at.
void Output_Water_ObserveLaraMesh(LARA_MESH mesh, GAME_VECTOR pos);

// Sets the water effects for the next Lara mesh draw; each push is paired
// with Output_Water_PopLaraMesh.
void Output_Water_PushLaraMesh(LARA_MESH mesh, GAME_VECTOR pos, int32_t radius);
// Sets the water effects for a joint bridging two Lara meshes and
// interpolates their lighting across the joint.
void Output_Water_PushLaraJoint(
    LARA_MESH parent, LARA_MESH child, GAME_VECTOR pos, int32_t radius);
void Output_Water_PopLaraMesh(void);

// Returns the ambient light for the last mesh pushed, or nullptr when Lara is
// lit entirely from one room.
const RGB_888 *Output_Water_GetLaraMeshAmbient(void);

// Reports whether the last mesh pushed spans two rooms and returns the first
// room's ambient light; joint vertices encode their position towards the
// second room, whose ambient light the mesh already carries.
bool Output_Water_GetLaraAmbientSpan(RGB_888 *out_from);
