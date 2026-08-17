#include <harness/harness.h>

#include <trx/core/math/const.h>
#include <trx/core/math/geom.h>
#include <trx/core/utils.h>
#include <trx/game/matrix.h>

// The rotation helpers stand in for roughly a hundred hand-rolled
// Math_Sin/Math_Cos pairs, so the thing worth pinning is that they agree with
// the matrix the renderer builds from the same angles - a sign slip reads as a
// vehicle drifting sideways and nothing else.

// Fixed point at 14 bits, taken three axes deep, drifts a unit or two.
#define M_TOLERANCE 3

static const XYZ_16 m_Rotations[] = {
    { .x = 0, .y = 0, .z = 0 },
    { .x = 0, .y = DEG_90, .z = 0 },
    { .x = 0, .y = -DEG_45, .z = 0 },
    { .x = DEG_45, .y = 0, .z = 0 },
    { .x = 0, .y = 0, .z = DEG_90 },
    { .x = DEG_45, .y = DEG_90, .z = -DEG_45 },
    { .x = -DEG_90, .y = -DEG_180, .z = DEG_45 },
    { .x = 4000, .y = -12000, .z = 7000 },
};

static void M_CheckNear(const XYZ_32 actual, const XYZ_32 expected)
{
    const XYZ_32 delta = XYZ_32_Subtract(actual, expected);
    if (ABS(delta.x) > M_TOLERANCE || ABS(delta.y) > M_TOLERANCE
        || ABS(delta.z) > M_TOLERANCE) {
        TEST_FAIL(
            "expected (%d, %d, %d), got (%d, %d, %d)", expected.x, expected.y,
            expected.z, actual.x, actual.y, actual.z);
    }
}

// What the renderer does with the same angles, for the helpers to match.
static XYZ_32 M_ByMatrix(const XYZ_32 vec, const XYZ_16 rot)
{
    MATRIX m = g_IDMatrix;
    Matrix_RotY_M(&m, rot.y);
    Matrix_RotX_M(&m, rot.x);
    Matrix_RotZ_M(&m, rot.z);
    return Matrix_MulVec32_M(&m, vec);
}

TEST(rotate_yaw_turns_forward_onto_the_axes)
{
    const XYZ_32 forward = { .x = 0, .y = 0, .z = 1024 };
    M_CheckNear(
        XYZ_32_RotateYaw(forward, 0), (XYZ_32) { .x = 0, .y = 0, .z = 1024 });
    M_CheckNear(
        XYZ_32_RotateYaw(forward, DEG_90),
        (XYZ_32) { .x = 1024, .y = 0, .z = 0 });
    M_CheckNear(
        XYZ_32_RotateYaw(forward, -DEG_180),
        (XYZ_32) { .x = 0, .y = 0, .z = -1024 });
    M_CheckNear(
        XYZ_32_RotateYaw(forward, -DEG_90),
        (XYZ_32) { .x = -1024, .y = 0, .z = 0 });
}

TEST(offset_rotated_puts_the_turned_vector_back_on_its_origin)
{
    const XYZ_32 origin = { .x = 1000, .y = 100, .z = -2000 };
    const XYZ_32 offset = { .x = 256, .y = -64, .z = 512 };
    const XYZ_16 rot = { .x = DEG_45, .y = DEG_90, .z = -DEG_45 };
    M_CheckNear(
        XYZ_32_OffsetLocalYaw(origin, offset, rot.y),
        XYZ_32_Add(origin, XYZ_32_RotateYaw(offset, rot.y)));
    M_CheckNear(
        XYZ_32_OffsetLocal(origin, offset, rot),
        XYZ_32_Add(origin, XYZ_32_Rotate(offset, rot)));
    // An offset straight ahead is what XYZ_32_OffsetYaw already did.
    M_CheckNear(
        XYZ_32_OffsetLocalYaw(origin, (XYZ_32) { .z = 1024 }, 5000),
        XYZ_32_OffsetYaw(origin, 5000, 1024));
}

TEST(rotate_yaw_leaves_height_alone)
{
    const XYZ_32 vec = { .x = 300, .y = -512, .z = 700 };
    CHECK_EQ_INT(XYZ_32_RotateYaw(vec, 5000).y, -512);
}

TEST(rotate_yaw_matches_the_full_rotation_with_no_pitch_or_roll)
{
    const XYZ_32 vec = { .x = 256, .y = -128, .z = 1024 };
    for (int32_t yaw = -DEG_180 / 2; yaw < DEG_180 / 2; yaw += 1024) {
        M_CheckNear(
            XYZ_32_RotateYaw(vec, yaw),
            XYZ_32_Rotate(vec, (XYZ_16) { .x = 0, .y = yaw, .z = 0 }));
    }
}

TEST(unrotate_yaw_reads_a_world_vector_along_an_items_axes)
{
    // An item turned a quarter to the right faces along world x, so a world
    // vector 256 along x lies 256 straight ahead of it.
    const XYZ_32 delta = { .x = 256, .y = 0, .z = -512 };
    M_CheckNear(
        XYZ_32_UnrotateYaw(delta, DEG_90),
        (XYZ_32) { .x = 512, .y = 0, .z = 256 });
    M_CheckNear(XYZ_32_UnrotateYaw(XYZ_32_RotateYaw(delta, 5000), 5000), delta);
}

TEST(rotate_matches_the_matrix_the_renderer_builds)
{
    const XYZ_32 vec = { .x = 341, .y = -777, .z = 1024 };
    for (int32_t i = 0; i < (int32_t)(sizeof(m_Rotations) / sizeof(XYZ_16));
         i++) {
        M_CheckNear(
            XYZ_32_Rotate(vec, m_Rotations[i]),
            M_ByMatrix(vec, m_Rotations[i]));
    }
}

TEST(unrotate_undoes_rotate)
{
    const XYZ_32 vec = { .x = 341, .y = -777, .z = 1024 };
    for (int32_t i = 0; i < (int32_t)(sizeof(m_Rotations) / sizeof(XYZ_16));
         i++) {
        M_CheckNear(
            XYZ_32_Unrotate(XYZ_32_Rotate(vec, m_Rotations[i]), m_Rotations[i]),
            vec);
    }
}

TEST(rotate_holds_at_offsets_that_would_overflow_32_bits)
{
    // 100000 * 16384 already runs past int32, and every hand-rolled site that
    // multiplies before shifting is wrong there.
    const XYZ_32 vec = { .x = 0, .y = 0, .z = 100000 };
    M_CheckNear(
        XYZ_32_RotateYaw(vec, DEG_90),
        (XYZ_32) { .x = 100000, .y = 0, .z = 0 });
}

TEST(add_and_subtract_are_componentwise)
{
    const XYZ_32 a = { .x = 1, .y = 2, .z = 3 };
    const XYZ_32 b = { .x = 10, .y = -20, .z = 30 };
    const XYZ_32 sum = XYZ_32_Add(a, b);
    CHECK_EQ_INT(sum.x, 11);
    CHECK_EQ_INT(sum.y, -18);
    CHECK_EQ_INT(sum.z, 33);
    M_CheckNear(XYZ_32_Subtract(sum, b), a);
}
