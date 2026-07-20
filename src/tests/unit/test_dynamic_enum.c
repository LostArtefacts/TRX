#include "harness.h"

#include <trx/core/dynamic_enum.h>

// Two unrelated tokens, so the registry is shown to key values per token rather
// than pooling them.
static const char m_TokenA;
static const char m_TokenB;

TEST(add_and_query_values)
{
    DynamicEnum_ResetValues(&m_TokenA);
    DynamicEnum_AddValue(&m_TokenA, "alpha", nullptr);
    DynamicEnum_AddValue(&m_TokenA, "beta", nullptr);

    CHECK_EQ_INT(DynamicEnum_GetValueCount(&m_TokenA), 2);
    CHECK_EQ_STR(DynamicEnum_GetValueAt(&m_TokenA, 0), "alpha");
    CHECK_EQ_STR(DynamicEnum_GetValueAt(&m_TokenA, 1), "beta");
    CHECK(DynamicEnum_IsValidValue(&m_TokenA, "beta"));
    CHECK(!DynamicEnum_IsValidValue(&m_TokenA, "gamma"));
}

TEST(reset_clears_prior_values)
{
    DynamicEnum_ResetValues(&m_TokenA);
    DynamicEnum_AddValue(&m_TokenA, "one", nullptr);
    DynamicEnum_ResetValues(&m_TokenA);
    CHECK_EQ_INT(DynamicEnum_GetValueCount(&m_TokenA), 0);
    CHECK(!DynamicEnum_IsValidValue(&m_TokenA, "one"));
}

TEST(tokens_are_independent)
{
    DynamicEnum_ResetValues(&m_TokenA);
    DynamicEnum_ResetValues(&m_TokenB);
    DynamicEnum_AddValue(&m_TokenA, "a-only", nullptr);
    DynamicEnum_AddValue(&m_TokenB, "b-only", nullptr);

    CHECK(DynamicEnum_IsValidValue(&m_TokenA, "a-only"));
    CHECK(!DynamicEnum_IsValidValue(&m_TokenA, "b-only"));
    CHECK(DynamicEnum_IsValidValue(&m_TokenB, "b-only"));
}

// A value carrying a label reports the label (the stub GameString_Get echoes
// its key); a value the registry never saw is echoed back as its own label.
TEST(labels_resolve_or_echo_the_value)
{
    DynamicEnum_ResetValues(&m_TokenA);
    DynamicEnum_AddValue(&m_TokenA, "coded", "labels/coded");

    CHECK_EQ_STR(
        DynamicEnum_GetLabelForValue(&m_TokenA, "coded"), "labels/coded");
    CHECK_EQ_STR(DynamicEnum_GetLabelForValue(&m_TokenA, "ghost"), "ghost");
}

TEST(cycling_walks_the_values)
{
    DynamicEnum_ResetValues(&m_TokenA);
    DynamicEnum_AddValue(&m_TokenA, "first", nullptr);
    DynamicEnum_AddValue(&m_TokenA, "second", nullptr);
    DynamicEnum_AddValue(&m_TokenA, "third", nullptr);

    CHECK(DynamicEnum_CanCycle(&m_TokenA, "first", 1));
    CHECK(!DynamicEnum_CanCycle(&m_TokenA, "third", 1));
    CHECK(!DynamicEnum_CanCycle(&m_TokenA, "first", -1));

    CHECK_EQ_STR(DynamicEnum_GetNext(&m_TokenA, "first", 1), "second");
    CHECK_EQ_STR(DynamicEnum_GetNext(&m_TokenA, "second", -1), "first");
    CHECK_NULL(DynamicEnum_GetNext(&m_TokenA, "third", 1));

    // An unknown current value cycles to the first entry.
    CHECK_EQ_STR(DynamicEnum_GetNext(&m_TokenA, "missing", 1), "first");
}
