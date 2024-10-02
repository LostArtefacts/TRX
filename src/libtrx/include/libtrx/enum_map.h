#include <stdint.h>

#define ENUM_MAP_DEFINE(enum_name, enum_value, str_value)                      \
    EnumMap_Define(ENUM_MAP_NAME(enum_name), enum_value, str_value);
#define ENUM_MAP_DEFINE_SELF(enum_name, enum_value)                            \
    EnumMap_Define(ENUM_MAP_NAME(enum_name), enum_value, #enum_value);

#define ENUM_MAP_GET(enum_name, str_value, default_value)                      \
    EnumMap_Get(ENUM_MAP_NAME(enum_name), str_value, default_value)

#define ENUM_MAP_TO_STRING(enum_name, enum_value)                              \
    EnumMap_ToString(ENUM_MAP_NAME(enum_name), enum_value)

#define ENUM_MAP_NAME(enum_name) #enum_name

// The function to put the EnumMap_Define calls in
extern void EnumMap_Init(void);

void EnumMap_Shutdown(void);

void EnumMap_Define(
    const char *enum_name, int32_t enum_value, const char *str_value);
int32_t EnumMap_Get(
    const char *enum_name, const char *str_value, int32_t default_value);
const char *EnumMap_ToString(const char *enum_name, int32_t enum_value);
