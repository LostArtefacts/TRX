#pragma once

typedef struct {
    const char *text;
    int value;
} ENUM_STRING_MAP;

#define ENUM_STRING_MAP(type) g_EnumStr_##type

#define DECLARE_ENUM_STRING_MAP(type, ...)                                     \
    extern const ENUM_STRING_MAP g_EnumStr_##type[];

#include "global/enum_str.def"
#include "global/types.h"
