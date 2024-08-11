#pragma once

typedef struct {
    const char *text;
    int value;
} ENUM_STRING_MAP;

extern const ENUM_STRING_MAP g_EnumStr_UI_STYLE[];
extern const ENUM_STRING_MAP g_EnumStr_BAR_SHOW_MODE[];
extern const ENUM_STRING_MAP g_EnumStr_BAR_LOCATION[];
extern const ENUM_STRING_MAP g_EnumStr_BAR_COLOR[];
extern const ENUM_STRING_MAP g_EnumStr_TARGET_LOCK_MODE[];
extern const ENUM_STRING_MAP g_EnumStr_SCREENSHOT_FORMAT[];
extern const ENUM_STRING_MAP g_EnumStr_UNDERWATER_MUSIC_MODE[];

#define ENUM_STR_MAP(type) g_EnumStr_##type
