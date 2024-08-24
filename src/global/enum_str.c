#include "global/enum_str.h"

#undef DECLARE_ENUM_STRING_MAP
#define DECLARE_ENUM_STRING_MAP(type, ...)                                     \
    const ENUM_STRING_MAP g_EnumStr_##type[] = { __VA_ARGS__ { NULL, -1 } };

#include "global/enum_str.def"
