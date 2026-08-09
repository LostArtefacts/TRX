#include <harness/harness.h>

#include <trx/core/math/const.h>
#include <trx/game/matrix.h>

// Rotations reach the renderer as three angles, and the tempting way to meet
// two of them in the middle is to meet each angle in the middle. That is not
// the same rotation: the TR4 cutscene tracks quantize to ten bits per axis, and
// near a quarter turn on X the exporter switches between two triples half a
// turn apart on Y and Z that name the same pose. These pin the arc.

// The matrix is built from a 14-bit trig table and read back through a double,
// so a rotation only survives the round trip to about a thousandth.
#define M_TOLERANCE 0.002

// Where a rotation sends the three unit axes, which is the whole of what it
// does, and what the renderer sees. Comparing angles instead would call two
// spellings of one rotation different.
typedef struct {
    double v[3][3];
} M_AXES;

static M_AXES M_Apply(const XYZ_16 rot)
{
    M_AXES out;
    const double scale = 1 << W2V_SHIFT;
    const XYZ_32 axes[3] = {
        { .x = 1, .y = 0, .z = 0 },
        { .x = 0, .y = 1, .z = 0 },
        { .x = 0, .y = 0, .z = 1 },
    };
    for (int32_t i = 0; i < 3; i++) {
        Matrix_ResetStack();
        *g_MatrixPtr = g_IDMatrix;
        *g_WMatrixPtr = g_IDMatrix;
        Matrix_Rot16(rot);
        Matrix_TranslateRel32(axes[i]);
        out.v[i][0] = g_MatrixPtr->_03 / scale;
        out.v[i][1] = g_MatrixPtr->_13 / scale;
        out.v[i][2] = g_MatrixPtr->_23 / scale;
    }
    return out;
}

// How far apart two rotations point, in units of the axis length.
static double M_Distance(const M_AXES a, const M_AXES b)
{
    double worst = 0.0;
    for (int32_t i = 0; i < 3; i++) {
        for (int32_t j = 0; j < 3; j++) {
            const double d = a.v[i][j] - b.v[i][j];
            worst = worst > d ? worst : d;
            worst = worst > -d ? worst : -d;
        }
    }
    return worst;
}

#define M_CHECK_NEAR(rot_1_, rot_2_, tolerance_)                               \
    do {                                                                       \
        const XYZ_16 r1_ = (rot_1_);                                           \
        const XYZ_16 r2_ = (rot_2_);                                           \
        const double d_ = M_Distance(M_Apply(r1_), M_Apply(r2_));              \
        if (!(d_ <= (tolerance_))) {                                           \
            TEST_FAIL(                                                         \
                "(%d,%d,%d) and (%d,%d,%d) are %.4f apart, over %.4f", r1_.x,  \
                r1_.y, r1_.z, r2_.x, r2_.y, r2_.z, d_, (double)(tolerance_));  \
        }                                                                      \
    } while (0)

TEST(test_slerp_rot16_keeps_the_ends)
{
    const XYZ_16 from = { .x = 1000, .y = -20000, .z = 3000 };
    const XYZ_16 to = { .x = -8000, .y = 25000, .z = -4000 };
    M_CHECK_NEAR(Matrix_SlerpRot16(from, to, 0.0), from, M_TOLERANCE);
    M_CHECK_NEAR(Matrix_SlerpRot16(from, to, 1.0), to, M_TOLERANCE);
}

TEST(test_slerp_rot16_halves_a_turn)
{
    const XYZ_16 from = { .y = 0 };
    const XYZ_16 to = { .y = DEG_90 };
    M_CHECK_NEAR(
        Matrix_SlerpRot16(from, to, 0.5), (XYZ_16) { .y = DEG_45 },
        M_TOLERANCE);
}

TEST(test_slerp_rot16_takes_the_short_way_around)
{
    // Two degrees apart across the wrap, not 358 the other way.
    const XYZ_16 from = { .y = DEG_1 * -1 };
    const XYZ_16 to = { .y = DEG_1 * 1 };
    M_CHECK_NEAR(Matrix_SlerpRot16(from, to, 0.5), (XYZ_16) {}, M_TOLERANCE);
}

TEST(test_slerp_rot16_holds_still_between_two_spellings)
{
    // What the TR4 backpack cutscene hands the left forearm: X just short of a
    // quarter turn, and Y and Z both half a turn on one side and zero on the
    // other. Both name the same rotation, so the middle is that rotation too -
    // interpolating the angles apart from each other points it backwards.
    const XYZ_16 from = { .x = 15616, .y = 0, .z = 0 };
    const XYZ_16 to = { .x = 15936, .y = 32704, .z = -32768 };
    M_CHECK_NEAR(from, to, 0.15);
    for (int32_t i = 0; i <= 10; i++) {
        M_CHECK_NEAR(Matrix_SlerpRot16(from, to, i / 10.0), from, 0.15);
    }
}

TEST(test_slerp_rot16_reads_back_a_locked_rotation)
{
    // At exactly a quarter turn on X, Y and Z describe one turn between them
    // and cannot be told apart. Any spelling of it has to survive the trip.
    const XYZ_16 rot = { .x = DEG_90, .y = DEG_45, .z = DEG_45 };
    M_CHECK_NEAR(Matrix_SlerpRot16(rot, rot, 0.5), rot, M_TOLERANCE);
}
