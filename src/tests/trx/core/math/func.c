#include <harness/harness.h>

#include <trx/core/math/func.h>

// Pure integer maths, no engine behind it. These are the routines the geometry
// code leans on, and the sign and boundary cases are exactly what nobody checks
// by playing the game.

TEST(floor_div_rounds_toward_negative_infinity)
{
    // C division truncates toward zero: -1 / 4 == 0. Floor division must give
    // -1. The distinction is the whole reason this function exists - a position
    // one unit left of a sector boundary belongs to the sector on the left, and
    // plain division would put it on the right.
    CHECK_EQ_INT(Math_FloorDiv(-1, 4), -1);
    CHECK_EQ_INT(-1 / 4, 0); // what it must not do

    CHECK_EQ_INT(Math_FloorDiv(-4, 4), -1);
    CHECK_EQ_INT(Math_FloorDiv(-5, 4), -2);
    CHECK_EQ_INT(Math_FloorDiv(-8, 4), -2);
}

TEST(floor_div_is_plain_division_for_non_negative_operands)
{
    CHECK_EQ_INT(Math_FloorDiv(0, 4), 0);
    CHECK_EQ_INT(Math_FloorDiv(3, 4), 0);
    CHECK_EQ_INT(Math_FloorDiv(4, 4), 1);
    CHECK_EQ_INT(Math_FloorDiv(7, 4), 1);
}

TEST(integer_sqrt_is_exact_on_perfect_squares)
{
    CHECK_EQ_INT(Math_Sqrt(0), 0);
    CHECK_EQ_INT(Math_Sqrt(1), 1);
    CHECK_EQ_INT(Math_Sqrt(4), 2);
    CHECK_EQ_INT(Math_Sqrt(100), 10);
    CHECK_EQ_INT(Math_Sqrt(65536), 256);
    CHECK_EQ_INT(Math_Sqrt(1000000), 1000);
}

TEST(integer_sqrt_truncates_between_squares)
{
    // Not rounded: 8 lies between 2*2 and 3*3, and the answer is 2.
    CHECK_EQ_INT(Math_Sqrt(2), 1);
    CHECK_EQ_INT(Math_Sqrt(3), 1);
    CHECK_EQ_INT(Math_Sqrt(8), 2);
    CHECK_EQ_INT(Math_Sqrt(99), 9);
    CHECK_EQ_INT(Math_Sqrt(101), 10);
}

TEST(sqrt64_agrees_with_sqrt_and_reaches_past_32_bits)
{
    CHECK_EQ_INT(Math_Sqrt64(0), 0);
    CHECK_EQ_INT(Math_Sqrt64(144), 12);
    CHECK_EQ_INT(Math_Sqrt64(1000000), 1000);
    // A distance squared can overflow 32 bits, which is why this exists.
    CHECK_EQ_INT(Math_Sqrt64(1ULL << 40), 1ULL << 20);
}

TEST(gcd_reduces_and_handles_zero_from_either_side)
{
    CHECK_EQ_INT(Math_GCD(12, 18), 6);
    CHECK_EQ_INT(Math_GCD(18, 12), 6);
    CHECK_EQ_INT(Math_GCD(7, 13), 1); // coprime
    CHECK_EQ_INT(Math_GCD(9, 9), 9);

    // Zero is the identity - and the loop must not divide by it.
    CHECK_EQ_INT(Math_GCD(5, 0), 5);
    CHECK_EQ_INT(Math_GCD(0, 5), 5);
}
