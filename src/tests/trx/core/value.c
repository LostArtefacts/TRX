#include <harness/harness.h>

#include <trx/core/dynamic_enum.h>
#include <trx/core/enum_map.h>
#include <trx/core/value.h>

TEST(type_sizes_and_names_are_stable)
{
    CHECK_EQ_INT(Value_TypeSize(TVT_BOOL), sizeof(bool));
    CHECK_EQ_INT(Value_TypeSize(TVT_S16), 2);
    CHECK_EQ_INT(Value_TypeSize(TVT_U32), 4);
    CHECK_EQ_INT(Value_TypeSize(TVT_ENUM), sizeof(int32_t));
    CHECK_EQ_INT(Value_TypeSize(TVT_RGB_888), sizeof(RGB_888));
    CHECK_EQ_INT(Value_TypeSize(TVT_XYZ_16), sizeof(XYZ_16));
    CHECK_EQ_INT(Value_TypeSize(TVT_DYNAMIC_ENUM), sizeof(char *));

    CHECK_EQ_STR(Value_TypeName(TVT_S16), "S16");
    CHECK_EQ_STR(Value_TypeName(TVT_XYZ_32), "XYZ_32");
    CHECK_EQ_STR(Value_TypeName(TVT_DYNAMIC_ENUM), "DYNAMIC_ENUM");
}

TEST(type_of_resolves_from_the_member)
{
    struct {
        bool b;
        int8_t i8;
        uint16_t u16;
        int32_t i32;
        float f;
        XYZ_16 v16;
        RGB_888 rgb;
        const char *s;
    } sample;
    CHECK_EQ_INT(Value_TypeOf(sample.b), TVT_BOOL);
    CHECK_EQ_INT(Value_TypeOf(sample.i8), TVT_S8);
    CHECK_EQ_INT(Value_TypeOf(sample.u16), TVT_U16);
    CHECK_EQ_INT(Value_TypeOf(sample.i32), TVT_S32);
    CHECK_EQ_INT(Value_TypeOf(sample.f), TVT_FLOAT);
    CHECK_EQ_INT(Value_TypeOf(sample.v16), TVT_XYZ_16);
    CHECK_EQ_INT(Value_TypeOf(sample.rgb), TVT_RGB_888);
    CHECK_EQ_INT(Value_TypeOf(sample.s), TVT_STRING);
}

TEST(read_write_round_trips_every_scalar)
{
    uint16_t u16 = 0;
    const TRX_VALUE in_u16 = { .type = TVT_U16, .as_int = 40000 };
    CHECK_NULL(Value_WritePtr(TVT_U16, &u16, &in_u16));
    CHECK_EQ_INT(u16, 40000);
    TRX_VALUE out;
    Value_ReadPtr(TVT_U16, &u16, &out);
    CHECK_EQ_INT(out.as_int, 40000);

    RGB_888 rgb = {};
    const TRX_VALUE in_rgb = { .type = TVT_RGB_888,
                               .as_rgb = { 0x11, 0x22, 0x33 } };
    CHECK_NULL(Value_WritePtr(TVT_RGB_888, &rgb, &in_rgb));
    CHECK_EQ_INT(rgb.g, 0x22);
    Value_ReadPtr(TVT_RGB_888, &rgb, &out);
    CHECK_EQ_INT(out.as_rgb.b, 0x33);
}

// XYZ_16 rides in as_xyz and narrows to a 16-bit member on store.
TEST(xyz16_narrows_on_store)
{
    XYZ_16 v = {};
    const TRX_VALUE in = { .type = TVT_XYZ_16,
                           .as_xyz = { .x = -100, .y = 200, .z = 300 } };
    CHECK_NULL(Value_WritePtr(TVT_XYZ_16, &v, &in));
    CHECK_EQ_INT(v.x, -100);
    CHECK_EQ_INT(v.z, 300);
    TRX_VALUE out;
    Value_ReadPtr(TVT_XYZ_16, &v, &out);
    CHECK_EQ_INT(out.as_xyz.y, 200);
}

TEST(range_check_rejects_overflow)
{
    int16_t i16 = 0;
    const TRX_VALUE too_big = { .type = TVT_S16, .as_int = 99999 };
    CHECK_NOT_NULL(Value_WritePtr(TVT_S16, &i16, &too_big));
    CHECK_EQ_INT(i16, 0); // rejected, left untouched

    const TRX_VALUE neg = { .type = TVT_U8, .as_int = -1 };
    CHECK_NOT_NULL(Value_CheckRange(TVT_U8, &neg));

    const TRX_VALUE ok = { .type = TVT_S16, .as_int = -30000 };
    CHECK_NULL(Value_CheckRange(TVT_S16, &ok));

    const TRX_VALUE bad_vec = { .type = TVT_XYZ_16,
                                .as_xyz = { .x = 0, .y = 70000, .z = 0 } };
    CHECK_NOT_NULL(Value_CheckRange(TVT_XYZ_16, &bad_vec));
}

TEST(parse_and_format_round_trip)
{
    TRX_VALUE v;

    CHECK(Value_Parse(TVT_BOOL, nullptr, "on", &v));
    CHECK(v.as_bool);
    CHECK(Value_Parse(TVT_BOOL, nullptr, "0", &v));
    CHECK(!v.as_bool);
    CHECK(!Value_Parse(TVT_BOOL, nullptr, "maybe", &v));

    CHECK(Value_Parse(TVT_S32, nullptr, "-1234", &v));
    CHECK_EQ_INT(v.as_int, -1234);
    CHECK_EQ_STR(Value_Format(TVT_S32, nullptr, &v, false), "-1234");

    CHECK(Value_Parse(TVT_U32, nullptr, "4000000000", &v));
    CHECK_EQ_INT(v.as_int, 4000000000LL);

    CHECK(Value_Parse(TVT_RGB_888, nullptr, "#AABBCC", &v));
    CHECK_EQ_INT(v.as_rgb.r, 0xAA);
    CHECK_EQ_STR(Value_Format(TVT_RGB_888, nullptr, &v, false), "aabbcc");

    CHECK(Value_Parse(TVT_DOUBLE, nullptr, "1.50", &v));
    CHECK_EQ_STR(Value_Format(TVT_DOUBLE, nullptr, &v, false), "1.50");
}

TEST(coerce_between_numeric_types)
{
    TRX_VALUE out;

    const TRX_VALUE from_double = { .type = TVT_DOUBLE, .as_num = 3.9 };
    CHECK(Value_Coerce(TVT_S32, &from_double, &out));
    CHECK_EQ_INT(out.type, TVT_S32);
    CHECK_EQ_INT(out.as_int, 3);

    const TRX_VALUE from_int = { .type = TVT_S32, .as_int = 7 };
    CHECK(Value_Coerce(TVT_FLOAT, &from_int, &out));
    CHECK(out.as_num == 7.0);

    const TRX_VALUE a_bool = { .type = TVT_BOOL, .as_bool = true };
    CHECK(!Value_Coerce(TVT_S32, &a_bool, &out));
}

TEST(equal_ptr_compares_by_type)
{
    const int32_t a = 5;
    const int32_t b = 5;
    const int32_t c = 6;
    CHECK(Value_EqualPtr(TVT_S32, &a, &b));
    CHECK(!Value_EqualPtr(TVT_S32, &a, &c));

    const char *s1 = "hi";
    const char *s2 = "hi";
    const char *s3 = "bye";
    CHECK(Value_EqualPtr(TVT_STRING, &s1, &s2));
    CHECK(!Value_EqualPtr(TVT_STRING, &s1, &s3));
}

TEST(copy_ptr_moves_scalars_and_owns_strings)
{
    int32_t src_i = 77;
    int32_t dst_i = 0;
    Value_CopyPtr(TVT_S32, &dst_i, &src_i);
    CHECK_EQ_INT(dst_i, 77);

    // A string copy duplicates onto a fresh pointer and frees the old one.
    const char *src_s = "hello";
    char *dst_s = nullptr;
    Value_CopyPtr(TVT_STRING, &dst_s, &src_s);
    CHECK_NOT_NULL(dst_s);
    CHECK(dst_s != src_s);
    CHECK_EQ_STR(dst_s, "hello");
    Value_CopyPtr(TVT_STRING, &dst_s, &(const char *) { nullptr });
    CHECK_NULL(dst_s);
}

TEST(enum_resolves_end_to_end)
{
    EnumMap_Define("TESTCOLOR", "TESTCOLOR_RED", 10, "red");
    EnumMap_Define("TESTCOLOR", "TESTCOLOR_BLUE", 20, "blue");

    TRX_VALUE v;
    CHECK(Value_Parse(TVT_ENUM, "TESTCOLOR", "blue", &v));
    CHECK_EQ_INT(v.as_int, 20);
    CHECK_EQ_STR(Value_Format(TVT_ENUM, "TESTCOLOR", &v, false), "blue");
    CHECK(!Value_Parse(TVT_ENUM, "TESTCOLOR", "green", &v));

    int32_t stored = 0;
    CHECK_NULL(Value_WritePtr(TVT_ENUM, &stored, &v));
    CHECK_EQ_INT(stored, 20);
}

TEST(dynamic_enum_resolves_end_to_end)
{
    static const char m_Token; // a stable address to key the registry on
    DynamicEnum_ResetValues(&m_Token);
    DynamicEnum_AddValue(&m_Token, "alpha", nullptr);
    DynamicEnum_AddValue(&m_Token, "beta", nullptr);

    TRX_VALUE v;
    CHECK(Value_Parse(TVT_DYNAMIC_ENUM, &m_Token, "beta", &v));
    CHECK_EQ_STR(v.as_str, "beta");
    CHECK_EQ_STR(Value_Format(TVT_DYNAMIC_ENUM, &m_Token, &v, false), "beta");
    CHECK(!Value_Parse(TVT_DYNAMIC_ENUM, &m_Token, "gamma", &v));
}
