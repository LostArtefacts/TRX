#include "harness.h"

#include <trx/core/bson.h>
#include <trx/core/json.h>
#include <trx/core/memory.h>

#define M_UNREAD (-1)

static int64_t M_RoundTrip(const int64_t value)
{
    JSON_OBJECT *const obj = JSON_ObjectNew();
    JSON_ObjectAppendInt64(obj, "value", value);
    JSON_VALUE *const root = JSON_ValueFromObject(obj);

    size_t size = 0;
    void *const blob = BSON_Write(root, &size);
    JSON_ValueFree(root);
    if (blob == nullptr) {
        return M_UNREAD;
    }

    JSON_VALUE *const parsed = BSON_Parse(blob, size);
    Memory_Free(blob);
    if (parsed == nullptr) {
        return M_UNREAD;
    }

    const int64_t result =
        JSON_ObjectGetInt64(JSON_ValueAsObject(parsed), "value", M_UNREAD);
    JSON_ValueFree(parsed);
    return result;
}

TEST(a_number_within_int32_survives_the_round_trip)
{
    CHECK_EQ_INT(M_RoundTrip(0), 0);
    CHECK_EQ_INT(M_RoundTrip(1), 1);
    CHECK_EQ_INT(M_RoundTrip(-1), -1);
    CHECK_EQ_INT(M_RoundTrip(INT32_MAX), INT32_MAX);
    CHECK_EQ_INT(M_RoundTrip(INT32_MIN), INT32_MIN);
}

TEST(a_number_past_int32_survives_the_round_trip)
{
    CHECK_EQ_INT(M_RoundTrip((int64_t)INT32_MAX + 1), (int64_t)INT32_MAX + 1);
    CHECK_EQ_INT(M_RoundTrip((int64_t)INT32_MIN - 1), (int64_t)INT32_MIN - 1);
    CHECK_EQ_INT(M_RoundTrip(INT64_MAX), INT64_MAX);
    CHECK_EQ_INT(M_RoundTrip(INT64_MIN), INT64_MIN);
}

TEST(a_mask_with_its_top_bit_set_keeps_its_bits)
{
    // A uint64_t past INT64_MAX travels as a negative literal, so what has to
    // hold is the bit pattern rather than the value.
    const uint64_t mask = 0x8000000000000001ull;
    CHECK((uint64_t)M_RoundTrip((int64_t)mask) == mask);

    const uint64_t every_bit = ~0ull;
    CHECK((uint64_t)M_RoundTrip((int64_t)every_bit) == every_bit);
}

TEST(a_document_mixing_widths_writes_the_size_it_measured)
{
    // Both passes classify each number independently, so a document holding
    // more than one width is where they would disagree.
    JSON_OBJECT *const obj = JSON_ObjectNew();
    JSON_ObjectAppendInt(obj, "small", 7);
    JSON_ObjectAppendInt64(obj, "large", (int64_t)1 << 40);
    JSON_ObjectAppendDouble(obj, "fraction", 0.5);
    JSON_ObjectAppendInt(obj, "small_again", -9);
    JSON_VALUE *const root = JSON_ValueFromObject(obj);

    size_t size = 0;
    void *const blob = BSON_Write(root, &size);
    JSON_ValueFree(root);
    CHECK(blob != nullptr);

    JSON_VALUE *const parsed = BSON_Parse(blob, size);
    Memory_Free(blob);
    CHECK(parsed != nullptr);

    const JSON_OBJECT *const read = JSON_ValueAsObject(parsed);
    CHECK_EQ_INT(JSON_ObjectGetInt(read, "small", -1), 7);
    CHECK_EQ_INT(JSON_ObjectGetInt64(read, "large", -1), (int64_t)1 << 40);
    CHECK(JSON_ObjectGetDouble(read, "fraction", -1.0) == 0.5);
    CHECK_EQ_INT(JSON_ObjectGetInt(read, "small_again", 0), -9);
    JSON_ValueFree(parsed);
}
