#pragma once

#include <trx/game/anims/types.h>
#include <trx/game/objects/types.h>

// Walks an object's bones, producing the transform for each joint so drawing,
// sampling, and bounds all use the same pose. Supports blending between poses.

typedef struct {
    XYZ_16 offset_a;
    XYZ_16 offset_b;
    const XYZ_16 *rots_a;
    const XYZ_16 *rots_b;
    int32_t frac;
    int32_t rate;
} ANIM_POSE;

ANIM_POSE Anim_Pose_FromFrames(
    const ANIM_FRAME *frame_a, const ANIM_FRAME *frame_b, int32_t frac,
    int32_t rate);
ANIM_POSE Anim_Pose_FromFrame(const ANIM_FRAME *frame);
ANIM_POSE Anim_Pose_FromRots(const XYZ_16 *rots, XYZ_16 offset);

typedef struct {
    const OBJECT *obj;
    ANIM_POSE pose;
    // Extra bone rotations, in bone order. Null for none.
    const int16_t *extra_rotations;
    // Applies the root joint's extra rotation when drawing, but not when
    // sampling.
    bool applies_base_rot;
} ANIM_WALK_DESC;

typedef struct {
    ANIM_WALK_DESC desc;
    const ANIM_BONE *bones;
    const int16_t *extra_rotation;
    int32_t joint;
    int32_t last_joint;
    int32_t depth;
    bool interpolated;
    bool ended;
} ANIM_WALK;

// Starts from the caller's current matrix without pushing a new one. Every
// walk must end with Anim_Walk_End, on all exit paths.
void Anim_Walk_Begin(ANIM_WALK *walk, const ANIM_WALK_DESC *desc);

// Starts a walk that stops after last_joint. The caller clamps the value to
// the object's mesh count; a negative value visits no joint at all.
void Anim_Walk_BeginToJoint(
    ANIM_WALK *walk, const ANIM_WALK_DESC *desc, int32_t last_joint);

// Moves the matrix to the next joint. Returns false past the last joint, and
// leaves the matrix stack as it stands.
bool Anim_Walk_Next(ANIM_WALK *walk);

// Gets a point's position relative to where the walk started. Blending
// continues for the remaining joints. Call this before Anim_Walk_End, which
// discards the matrices the walk built.
XYZ_32 Anim_Walk_GetPos(ANIM_WALK *walk, XYZ_32 local);

// Ends the walk and pops every matrix it pushed. Fatal if called twice, or
// after the walk has already ended.
void Anim_Walk_End(ANIM_WALK *walk);
