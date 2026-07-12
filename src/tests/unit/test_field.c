#include "harness.h"

#include <trx/core/field.h>

// A synthetic struct. field.c has no engine dependency, so the reflection layer
// can be exercised in full without loading a level, an item, or anything else.
typedef struct {
    bool flag;
    int8_t i8;
    uint8_t u8;
    int16_t i16;
    uint16_t u16;
    int32_t i32;
    uint32_t u32;
    float f32;
    double f64;
    XYZ_16 small_vec;
    XYZ_32 big_vec;
    const char *text;
    int16_t locked;
    int32_t guarded;
    int16_t narrow;
    struct {
        bool nested_flag;
    } group;
} SAMPLE;

static bool M_GetDoubled(const void *const self, FIELD_VALUE *const out)
{
    const SAMPLE *const s = self;
    *out = (FIELD_VALUE) { .type = FT_INT32, .as_int = s->i32 * 2 };
    return true;
}

static const char *M_SetRejecting(void *const self, const FIELD_VALUE *const in)
{
    if (in->as_int < 0) {
        return "must not be negative";
    }
    ((SAMPLE *)self)->guarded = in->as_int;
    return nullptr;
}

// A setter that assigns the widened carrier straight into a narrow member, as
// the engine's own validating setters do.
static const char *M_SetNarrow(void *const self, const FIELD_VALUE *const in)
{
    ((SAMPLE *)self)->narrow = in->as_int;
    return nullptr;
}

// clang-format off
static const FIELD_DESC M_SAMPLE_FIELDS[] = {
    FIELD    (SAMPLE, flag),
    FIELD    (SAMPLE, i8),
    FIELD    (SAMPLE, u8),
    FIELD    (SAMPLE, i16),
    FIELD    (SAMPLE, u16),
    FIELD    (SAMPLE, i32),
    FIELD    (SAMPLE, u32),
    FIELD    (SAMPLE, f32),
    FIELD    (SAMPLE, f64),
    FIELD    (SAMPLE, small_vec),
    FIELD    (SAMPLE, big_vec),
    FIELD    (SAMPLE, text),
    FIELD    (SAMPLE, group.nested_flag),
    FIELD_RO (SAMPLE, locked),
    FIELD_FN ("doubled", FT_INT32, M_GetDoubled, nullptr),
    FIELD_SET(SAMPLE, guarded, M_SetRejecting),
    FIELD_SET(SAMPLE, narrow, M_SetNarrow),
};
// clang-format on

TYPE_DEFINE(SAMPLE, M_SAMPLE_FIELDS)

static const FIELD_DESC *F(const char *const name)
{
    return Field_Find(&TYPE_SAMPLE, name);
}

TEST(field_find_locates_members_and_misses_absent_ones)
{
    CHECK_NOT_NULL(F("i16"));
    CHECK_EQ_STR(F("i16")->name, "i16");
    CHECK_NULL(F("no_such_member"));
}

TEST(field_get_reads_every_scalar_type)
{
    const SAMPLE s = {
        .flag = true,
        .i8 = -8,
        .u8 = 200,
        .i16 = -1234,
        .u16 = 60000,
        .i32 = -100000,
        .u32 = 4000000000u,
        .f32 = 1.5f,
        .f64 = 2.25,
        .text = "hello",
    };
    FIELD_VALUE v;

    CHECK(Field_Get(F("flag"), &s, &v));
    CHECK(v.as_bool);
    CHECK(Field_Get(F("i8"), &s, &v));
    CHECK_EQ_INT(v.as_int, -8);
    CHECK(Field_Get(F("u8"), &s, &v));
    CHECK_EQ_INT(v.as_int, 200);
    CHECK(Field_Get(F("i16"), &s, &v));
    CHECK_EQ_INT(v.as_int, -1234);
    CHECK(Field_Get(F("u16"), &s, &v));
    CHECK_EQ_INT(v.as_int, 60000);
    CHECK(Field_Get(F("i32"), &s, &v));
    CHECK_EQ_INT(v.as_int, -100000);
    CHECK(Field_Get(F("u32"), &s, &v));
    CHECK_EQ_INT(v.as_int, 4000000000LL);
    CHECK(Field_Get(F("f32"), &s, &v));
    CHECK(v.as_num == 1.5);
    CHECK(Field_Get(F("f64"), &s, &v));
    CHECK(v.as_num == 2.25);
    CHECK(Field_Get(F("text"), &s, &v));
    CHECK_EQ_STR(v.as_str, "hello");
}

// XYZ_16 is widened into the same carrier as XYZ_32, so a caller never has to
// care which one it is reading.
TEST(field_get_widens_xyz16_into_the_common_carrier)
{
    const SAMPLE s = {
        .small_vec = { .x = -1, .y = 2, .z = -3 },
        .big_vec = { .x = 100000, .y = -200000, .z = 300000 },
    };
    FIELD_VALUE v;

    CHECK(Field_Get(F("small_vec"), &s, &v));
    CHECK_EQ_INT(v.as_xyz.x, -1);
    CHECK_EQ_INT(v.as_xyz.y, 2);
    CHECK_EQ_INT(v.as_xyz.z, -3);

    CHECK(Field_Get(F("big_vec"), &s, &v));
    CHECK_EQ_INT(v.as_xyz.x, 100000);
    CHECK_EQ_INT(v.as_xyz.z, 300000);
}

TEST(field_set_narrows_on_store)
{
    SAMPLE s = {};

    CHECK_NULL(Field_Set(
        F("i16"), &s, &(FIELD_VALUE) { .type = FT_INT16, .as_int = -4321 }));
    CHECK_EQ_INT(s.i16, -4321);

    CHECK_NULL(Field_Set(
        F("small_vec"), &s,
        &(FIELD_VALUE) {
            .type = FT_XYZ_16,
            .as_xyz = { .x = 7, .y = -8, .z = 9 },
        }));
    CHECK_EQ_INT(s.small_vec.x, 7);
    CHECK_EQ_INT(s.small_vec.z, 9);

    CHECK_NULL(Field_Set(
        F("flag"), &s, &(FIELD_VALUE) { .type = FT_BOOL, .as_bool = true }));
    CHECK(s.flag);
}

// offsetof reaches through a nested struct of plain members, which is what lets
// ROOM.flags.underwater be a reflected field.
TEST(field_reaches_through_a_nested_struct)
{
    SAMPLE s = {};
    FIELD_VALUE v;

    CHECK_NULL(Field_Set(
        F("group.nested_flag"), &s,
        &(FIELD_VALUE) { .type = FT_BOOL, .as_bool = true }));
    CHECK(s.group.nested_flag);

    CHECK(Field_Get(F("group.nested_flag"), &s, &v));
    CHECK(v.as_bool);
}

TEST(field_set_refuses_a_readonly_member)
{
    SAMPLE s = { .locked = 5 };
    const char *const err = Field_Set(
        F("locked"), &s, &(FIELD_VALUE) { .type = FT_INT16, .as_int = 99 });
    CHECK_NOT_NULL(err);
    CHECK_EQ_INT(s.locked, 5); // unchanged
}

TEST(field_set_rejects_an_out_of_range_value)
{
    SAMPLE s = { .i16 = 5 };

    // 99999 does not fit an int16 - it must be rejected, not truncated to a
    // wrapped value.
    const char *const err = Field_Set(
        F("i16"), &s, &(FIELD_VALUE) { .type = FT_INT16, .as_int = 99999 });
    CHECK_NOT_NULL(err);
    CHECK_EQ_INT(s.i16, 5); // unchanged

    // The boundary value fits and is stored.
    CHECK_NULL(Field_Set(
        F("i16"), &s, &(FIELD_VALUE) { .type = FT_INT16, .as_int = 32767 }));
    CHECK_EQ_INT(s.i16, 32767);

    // A negative value for an unsigned member is out of range too.
    CHECK_NOT_NULL(Field_Set(
        F("u8"), &s, &(FIELD_VALUE) { .type = FT_UINT8, .as_int = -1 }));

    // A vector component past int16 is rejected without a partial store.
    SAMPLE v = {};
    CHECK_NOT_NULL(Field_Set(
        F("small_vec"), &v,
        &(FIELD_VALUE) {
            .type = FT_XYZ_16,
            .as_xyz = { .x = 1, .y = 99999, .z = 3 },
        }));
    CHECK_EQ_INT(v.small_vec.x, 0); // unchanged
}

TEST(computed_member_uses_its_accessor_and_is_readonly)
{
    SAMPLE s = { .i32 = 21 };
    FIELD_VALUE v;

    CHECK(Field_Get(F("doubled"), &s, &v));
    CHECK_EQ_INT(v.as_int, 42);

    // FIELD_FN with no setter is read-only by construction.
    CHECK_NOT_NULL(Field_Set(
        F("doubled"), &s, &(FIELD_VALUE) { .type = FT_INT32, .as_int = 1 }));
}

TEST(a_validating_setter_can_reject_a_value)
{
    SAMPLE s = { .guarded = 7 };

    const char *const err = Field_Set(
        F("guarded"), &s, &(FIELD_VALUE) { .type = FT_INT32, .as_int = -1 });
    CHECK_NOT_NULL(err);
    CHECK_EQ_STR(err, "must not be negative");
    CHECK_EQ_INT(s.guarded, 7); // rejected, so unchanged

    CHECK_NULL(Field_Set(
        F("guarded"), &s, &(FIELD_VALUE) { .type = FT_INT32, .as_int = 11 }));
    CHECK_EQ_INT(s.guarded, 11);
}

// Whether a value fits is a property of the member's storage, so a custom
// setter does not get to skip the check: it has no width of its own to check
// against, and would assign the widened carrier straight into a narrow member.
TEST(a_value_too_wide_for_the_member_is_rejected_even_with_a_setter)
{
    SAMPLE s = { .narrow = 7 };

    const char *const err = Field_Set(
        F("narrow"), &s, &(FIELD_VALUE) { .type = FT_INT16, .as_int = 99999 });
    CHECK_NOT_NULL(err);
    CHECK_EQ_INT(s.narrow, 7); // rejected, so not truncated

    CHECK_NULL(Field_Set(
        F("narrow"), &s, &(FIELD_VALUE) { .type = FT_INT16, .as_int = 1234 }));
    CHECK_EQ_INT(s.narrow, 1234);
}

// Found while writing these tests: Field_Find returns the first match, so a
// name declared twice silently shadows every later entry - a validating setter
// declared after a plain FIELD would simply never run.
TEST(duplicate_field_names_are_detected)
{
    CHECK_NULL(Field_FindDuplicateName(&TYPE_SAMPLE));

    static const FIELD_DESC BAD_FIELDS[] = {
        FIELD(SAMPLE, i16),
        FIELD(SAMPLE, i32),
        FIELD_SET(SAMPLE, i16, nullptr), // shadowed by the first
    };
    static const TYPE_DESC BAD_TYPE = {
        .name = "BAD",
        .fields = BAD_FIELDS,
        .field_count = 3,
    };
    CHECK_EQ_STR(Field_FindDuplicateName(&BAD_TYPE), "i16");
}

TEST(type_sizes_and_names_are_stable)
{
    CHECK_EQ_INT(Field_GetTypeSize(FT_BOOL), sizeof(bool));
    CHECK_EQ_INT(Field_GetTypeSize(FT_INT16), 2);
    CHECK_EQ_INT(Field_GetTypeSize(FT_UINT32), 4);
    CHECK_EQ_INT(Field_GetTypeSize(FT_XYZ_16), sizeof(XYZ_16));
    CHECK_EQ_INT(Field_GetTypeSize(FT_XYZ_32), sizeof(XYZ_32));
    CHECK_EQ_INT(Field_GetTypeSize(FT_STRING), sizeof(char *));

    CHECK_EQ_STR(Field_GetTypeName(FT_INT16), "INT16");
    CHECK_EQ_STR(Field_GetTypeName(FT_XYZ_32), "XYZ_32");
}

// Every declared member's sizeof must match its declared FIELD_TYPE. This is
// the check that caught a real bug: OBJECT_ID and ITEM_STATUS are 4-byte enums,
// and tagging either as FT_INT16 would have silently read half the member.
TEST(field_validate_type_accepts_a_consistent_table)
{
    for (int32_t i = 0; i < TYPE_SAMPLE.field_count; i++) {
        const FIELD_DESC *const f = &TYPE_SAMPLE.fields[i];
        if (f->get != nullptr && f->set != nullptr) {
            continue; // fully computed; no backing member to size-check
        }
        if (f->get != nullptr && (f->flags & FF_READONLY)) {
            continue; // read-only computed; never addresses memory
        }
        CHECK_EQ_INT(f->size, Field_GetTypeSize(f->type));
    }
    Field_ValidateType(&TYPE_SAMPLE); // asserts on mismatch
}

TEST(types_self_register_and_are_findable_by_name)
{
    const TYPE_DESC *const t = Type_GetByName("SAMPLE");
    CHECK_NOT_NULL(t);
    CHECK(t == &TYPE_SAMPLE);
    CHECK_NULL(Type_GetByName("NO_SUCH_TYPE"));
    CHECK(Type_GetCount() >= 1);
}
