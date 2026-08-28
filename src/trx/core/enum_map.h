#include <trx/core/vector.h>

#define ENUM_MAP(enum_type_name, enum_value, str_value)                        \
    EnumMap_Define(                                                            \
        ENUM_MAP_NAME(enum_type_name), #enum_value, enum_value, str_value)

// Defines a value from its C spelling without the prefix; the prefix includes
// its trailing underscore, as in
// ENUM_MAP_PREFIXED(LARA_GUN_TYPE, LGT_, SHOTGUN).
#define ENUM_MAP_PREFIXED(enum_type_name, prefix, suffix)                      \
    EnumMap_DefinePrefixed(                                                    \
        ENUM_MAP_NAME(enum_type_name), #prefix #suffix, prefix##suffix,        \
        #suffix)

#define ENUM_MAP_GET(enum_type_name, str_value, default_value)                 \
    EnumMap_Get(ENUM_MAP_NAME(enum_type_name), str_value, default_value)

#define ENUM_MAP_TO_STRING(enum_type_name, enum_value)                         \
    EnumMap_ToString(ENUM_MAP_NAME(enum_type_name), enum_value)

#define ENUM_MAP_NAME(enum_type_name) #enum_type_name
#define ENUM_MAP_MAX_NAME_SIZE 256

// Associate an integer enum value, such as WEATHER_SNOW, with a string
// representation such as "snow".
// @param enum_type_name    Name of the enum type, such as "WEATHER".
// @param enum_name         Name of the enum, such as "WEATHER_SNOW".
// @param enum_value        Value of the enum, such as 1.
// @param str_value         String representation of the enum, such as "snow".
void EnumMap_Define(
    const char *enum_type_name, const char *enum_name, int32_t enum_value,
    const char *str_value);

// Associates an integer enum value with its lower-case suffix for use by
// ENUM_MAP_PREFIXED.
// @param enum_type_name Specifies the enum type name, such as "LARA_GUN_TYPE".
// @param enum_name Specifies the enum constant name, such as "LGT_SHOTGUN".
// @param enum_value Specifies the enum value, such as 4.
// @param suffix Specifies the enum name without its prefix, such as "SHOTGUN".
void EnumMap_DefinePrefixed(
    const char *enum_type_name, const char *enum_name, int32_t enum_value,
    const char *suffix);

// Converts `-` and `:` to `_`; names of ENUM_MAP_MAX_NAME_SIZE characters or
// more are truncated.
const char *EnumMap_NormalizeName(const char *name);

// Retrieve an integer enum value from a string representation.
// @param enum_type_name    Name of the enum type, such as "WEATHER".
// @param str_value         String representation of the enum, such as "snow".
// @param default_value     Value to return in case the mapping fails.
// @return                  Value of the enum, such as 1.
int32_t EnumMap_Get(
    const char *enum_type_name, const char *str_value, int32_t default_value);

// Retrieve an enum integer canonical name as a string.
// @param enum_type_name    Name of the enum type, such as "WEATHER".
// @param enum_value        Value of the enum, such as 1.
// @return                  Name of the enum, such as "WEATHER_SNOW".
const char *EnumMap_GetName(const char *enum_type_name, int32_t enum_value);

// Retrieve a string representation, such as "snow", based on an integer enum
// value such as WEATHER_SNOW.
// @param enum_type_name    Name of the enum type, such as "WEATHER".
// @param enum_value        Value of the enum, such as 1.
// @return                  String representation of the enum, such as "snow".
const char *EnumMap_ToString(const char *enum_type_name, int32_t enum_value);

// Retrieve a localized label for the given enum value.
// @param enum_type_name    Name of the enum type, such as "WEATHER".
// @param enum_value        Value of the enum, such as 1.
// @return                  Localized label or nullptr if missing.
const char *EnumMap_GetLabel(const char *enum_type_name, int32_t enum_value);

// How many distinct values the given enum type has.
// @param enum_type_name    Name of the enum type, such as "WEATHER".
int32_t EnumMap_GetValueCount(const char *enum_type_name);

// Retrieve an enum value by position, in the order the values were defined in.
// @param enum_type_name    Name of the enum type, such as "WEATHER".
// @param index             Position of the value.
// @return                  Value of the enum, or -1 if there is none there.
int32_t EnumMap_GetValueAt(const char *enum_type_name, int32_t index);

// Returns a vector of valid string values for the given enum_type_name.
//
// The returned vector must be freed via Vector_Free(). The string pointers
// within the vector are owned by the enum map and should not be freed by the
// caller. Returns nullptr if the enum_type_name is not valid.
VECTOR *EnumMap_ListValues(const char *enum_type_name);
