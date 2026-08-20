#include <trx/game/anims/walk.h>

#include <trx/core/utils.h>
#include <trx/game/matrix.h>
#include <trx/game/objects.h>
#include <trx/game/objects/draw.h>

static void M_Push(const bool interpolated)
{
    if (interpolated) {
        Matrix_Push_I();
    } else {
        Matrix_Push();
    }
}

static void M_Pop(const bool interpolated)
{
    if (interpolated) {
        Matrix_Pop_I();
    } else {
        Matrix_Pop();
    }
}

static void M_TranslateRel32(const XYZ_32 offset, const bool interpolated)
{
    if (interpolated) {
        Matrix_TranslateRel32_I(offset);
    } else {
        Matrix_TranslateRel32(offset);
    }
}

static void M_Rot16(
    const XYZ_16 rot_a, const XYZ_16 rot_b, const bool interpolated)
{
    if (interpolated) {
        Matrix_Rot16_ID(rot_a, rot_b);
    } else {
        Matrix_Rot16(rot_a);
    }
}

static void M_EnterRoot(ANIM_WALK *const walk)
{
    const ANIM_POSE *const pose = &walk->desc.pose;
    if (walk->interpolated) {
        Matrix_InitInterpolate(pose->frac, pose->rate);
        Matrix_TranslateRel16_ID(pose->offset_a, pose->offset_b);
    } else {
        Matrix_TranslateRel16(pose->offset_a);
    }
    if (pose->rots_a != nullptr) {
        M_Rot16(pose->rots_a[0], pose->rots_b[0], walk->interpolated);
    }
    if (walk->desc.applies_base_rot) {
        Object_ApplyExtraRotation(
            &walk->extra_rotation, walk->desc.obj->base_rot,
            walk->interpolated);
    }
}

static void M_EnterJoint(ANIM_WALK *const walk)
{
    const ANIM_POSE *const pose = &walk->desc.pose;
    const ANIM_BONE *const bone = &walk->bones[walk->joint - 1];
    if (bone->matrix_pop) {
        walk->depth--;
        M_Pop(walk->interpolated);
    }
    if (bone->matrix_push) {
        walk->depth++;
        M_Push(walk->interpolated);
    }

    M_TranslateRel32(bone->pos, walk->interpolated);
    if (pose->rots_a != nullptr) {
        M_Rot16(
            pose->rots_a[walk->joint], pose->rots_b[walk->joint],
            walk->interpolated);
    }
    Object_ApplyExtraRotation(
        &walk->extra_rotation, bone->rot, walk->interpolated);
}

ANIM_POSE Anim_Pose_FromFrame(const ANIM_FRAME *const frame)
{
    if (frame == nullptr) {
        return (ANIM_POSE) {};
    }
    return (ANIM_POSE) {
        .offset_a = frame->offset,
        .rots_a = frame->mesh_rots,
    };
}

ANIM_POSE Anim_Pose_FromFrames(
    const ANIM_FRAME *const frame_a, const ANIM_FRAME *const frame_b,
    const int32_t frac, const int32_t rate)
{
    if (frame_a == nullptr || frame_b == nullptr) {
        return Anim_Pose_FromFrame(frame_a);
    }
    return (ANIM_POSE) {
        .offset_a = frame_a->offset,
        .offset_b = frame_b->offset,
        .rots_a = frame_a->mesh_rots,
        .rots_b = frame_b->mesh_rots,
        .frac = frac,
        .rate = rate,
    };
}

ANIM_POSE Anim_Pose_FromRots(const XYZ_16 *const rots, const XYZ_16 offset)
{
    return (ANIM_POSE) { .offset_a = offset, .rots_a = rots };
}

void Anim_Walk_BeginToJoint(
    ANIM_WALK *const walk, const ANIM_WALK_DESC *const desc,
    const int32_t joint)
{
    *walk = (ANIM_WALK) {
        .desc = *desc,
        .bones = Object_TryGetBone(desc->obj, 0),
        .extra_rotation = desc->extra_rotations,
        .joint = -1,
        .interpolated = desc->pose.rots_b != nullptr && desc->pose.frac != 0
            && desc->pose.rate != 0,
    };
    walk->last_joint = MIN(joint, desc->obj->mesh_count - 1);
    if (walk->desc.pose.rots_b == nullptr) {
        walk->desc.pose.rots_b = walk->desc.pose.rots_a;
    }
}

void Anim_Walk_Begin(ANIM_WALK *const walk, const ANIM_WALK_DESC *const desc)
{
    Anim_Walk_BeginToJoint(walk, desc, desc->obj->mesh_count - 1);
}

bool Anim_Walk_Next(ANIM_WALK *const walk)
{
    walk->joint++;
    if (walk->joint > walk->last_joint) {
        while (walk->depth > 0) {
            walk->depth--;
            M_Pop(walk->interpolated);
        }
        return false;
    }

    if (walk->joint == 0) {
        M_EnterRoot(walk);
    } else {
        M_EnterJoint(walk);
    }
    return true;
}

XYZ_32 Anim_Walk_GetPos(ANIM_WALK *const walk, const XYZ_32 local)
{
    M_Push(walk->interpolated);
    M_TranslateRel32(local, walk->interpolated);
    if (walk->interpolated) {
        Matrix_Interpolate();
    }
    const XYZ_32 result = {
        .x = g_MatrixPtr->_03 >> W2V_SHIFT,
        .y = g_MatrixPtr->_13 >> W2V_SHIFT,
        .z = g_MatrixPtr->_23 >> W2V_SHIFT,
    };
    M_Pop(walk->interpolated);
    return result;
}
