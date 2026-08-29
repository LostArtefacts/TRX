#include <harness/harness.h>

#include <trx/core/enum_map.h>

// enum_map.c is a leaf module: a name<->value registry with no engine state, so
// it can be exercised in full against a synthetic enum.
//
// The Lua enum bridge reflects the public scripting API out of this registry
// (see trx/game/lua/enum.c), so ListValues and Get are load-bearing for the
// docs: a constant that does not round-trip here is a constant that silently
// drops out of trx.items.Status and out of the reference.

typedef enum {
    M_WIDGET_OFF = 0,
    M_WIDGET_ON = 1,
    M_WIDGET_BROKEN = 7,
} M_WIDGET_STATE;

static __attribute__((constructor)) void M_Init(void)
{
    ENUM_MAP(M_WIDGET_STATE, M_WIDGET_OFF, "off");
    ENUM_MAP(M_WIDGET_STATE, M_WIDGET_ON, "on");
    ENUM_MAP(M_WIDGET_STATE, M_WIDGET_BROKEN, "broken");
    ENUM_MAP(M_WIDGET_STATE, M_WIDGET_BROKEN, "bust");
}

TEST(get_maps_string_to_value)
{
    CHECK_EQ_INT(ENUM_MAP_GET(M_WIDGET_STATE, "off", -1), M_WIDGET_OFF);
    CHECK_EQ_INT(ENUM_MAP_GET(M_WIDGET_STATE, "on", -1), M_WIDGET_ON);
    CHECK_EQ_INT(ENUM_MAP_GET(M_WIDGET_STATE, "broken", -1), M_WIDGET_BROKEN);
}

TEST(get_returns_the_default_for_an_unknown_value)
{
    CHECK_EQ_INT(ENUM_MAP_GET(M_WIDGET_STATE, "nonexistent", -1), -1);
    // Values must not leak across enum types.
    CHECK_EQ_INT(EnumMap_Get("NO_SUCH_ENUM", "on", -1), -1);
}

TEST(to_string_maps_value_to_string)
{
    CHECK_EQ_STR(ENUM_MAP_TO_STRING(M_WIDGET_STATE, M_WIDGET_BROKEN), "broken");
    CHECK_NULL(EnumMap_ToString("M_WIDGET_STATE", 999));
}

// Resolve duplicate values under either name and serialise them using the
// first name, so reordering definitions changes persisted files.
TEST(to_string_answers_with_the_first_name_defined)
{
    CHECK_EQ_INT(ENUM_MAP_GET(M_WIDGET_STATE, "bust", -1), M_WIDGET_BROKEN);
    CHECK_EQ_STR(ENUM_MAP_TO_STRING(M_WIDGET_STATE, M_WIDGET_BROKEN), "broken");
}

// Treat `-`, `:` and `_` as the same separator, so alternate spellings
// identify the same name.
TEST(separators_are_interchangeable)
{
    CHECK_EQ_INT(ENUM_MAP_GET(M_WIDGET_STATE, "off", -1), M_WIDGET_OFF);
    CHECK_EQ_STR(EnumMap_NormalizeName("half-lit:up"), "half_lit_up");
}

TEST(get_name_reports_the_c_identifier)
{
    CHECK_EQ_STR(EnumMap_GetName("M_WIDGET_STATE", M_WIDGET_ON), "M_WIDGET_ON");
}

// This is what trxc.enum.values() reflects. Every constant must come back,
// however it is numbered - M_WIDGET_BROKEN is 7, not 2, so nothing here may
// assume the values are contiguous.
TEST(list_values_returns_every_constant)
{
    VECTOR *const values = EnumMap_ListValues("M_WIDGET_STATE");
    CHECK_NOT_NULL(values);
    CHECK_EQ_INT(values->count, 3);

    bool seen_off = false;
    bool seen_on = false;
    bool seen_broken = false;
    for (int32_t i = 0; i < values->count; i++) {
        const char *const str = *(const char **)Vector_Get(values, i);
        seen_off |= strcmp(str, "off") == 0;
        seen_on |= strcmp(str, "on") == 0;
        seen_broken |= strcmp(str, "broken") == 0;
        // Every reflected name maps back to its value. The bridge builds the
        // Lua table out of exactly this round-trip.
        CHECK(EnumMap_Get("M_WIDGET_STATE", str, -1) != -1);
    }
    CHECK(seen_off);
    CHECK(seen_on);
    CHECK(seen_broken);

    Vector_Free(values);
}

// The bridge reads an empty result as a misspelled type name rather than as an
// empty enum, and raises. It can only do that if this stays empty.
TEST(list_values_of_an_unknown_enum_is_empty)
{
    VECTOR *const values = EnumMap_ListValues("NO_SUCH_ENUM");
    CHECK_NOT_NULL(values);
    CHECK_EQ_INT(values->count, 0);
    Vector_Free(values);
}
