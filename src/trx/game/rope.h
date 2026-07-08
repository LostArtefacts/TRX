#pragma once

#include <trx/core/math/types.h>
#include <trx/game/items/types.h>

#define ROPE_SEGMENTS 24
#define MAX_ROPES 5
#define NO_ROPE (-1)

// Node positions are stored in fixed point, shifted by (W2V_SHIFT + 2)
// relative to the rope's world anchor position.
typedef struct {
    XYZ_32 segments[ROPE_SEGMENTS];
    XYZ_32 velocities[ROPE_SEGMENTS];
    XYZ_32 normalised_segments[ROPE_SEGMENTS];
    // Node positions rebuilt at constant segment length; this is what gets
    // drawn and collided against.
    XYZ_32 mesh_segments[ROPE_SEGMENTS];
    // Previous frame's mesh segments, for render interpolation only.
    XYZ_32 prev_mesh_segments[ROPE_SEGMENTS];
    XYZ_32 pos;
    int32_t segment_length;
    bool active;
} ROPE;

// The pendulum is the point on the rope that Lara's weight acts on.
typedef struct {
    XYZ_32 pos;
    XYZ_32 vel;
    int32_t node;
    ROPE *rope;
} ROPE_PENDULUM;

void Rope_Reset(void);
void Rope_Create(int16_t item_num);
ROPE *Rope_Get(int32_t rope_num);
int32_t Rope_GetIndexByItem(int16_t item_num);
ROPE_PENDULUM *Rope_GetPendulum(void);

void Rope_Calculate(ROPE *rope);
void Rope_DrawAll(void);
void Rope_GetPos(const ROPE *rope, int32_t rel_pos, XYZ_32 *out_pos);
int32_t Rope_NodeCollision(const ROPE *rope, XYZ_32 pos, int32_t radius);
void Rope_SetPendulumVelocity(int32_t x, int32_t y, int32_t z);
void Rope_AlignLara(ITEM *item);
