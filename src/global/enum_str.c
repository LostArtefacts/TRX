#include "global/enum_str.h"

#include "global/types.h"

const ENUM_STRING_MAP g_EnumStr_UI_STYLE[] = {
    { "ps1", UI_STYLE_PS1 },
    { "pc", UI_STYLE_PC },
    { NULL, -1 },
};

const ENUM_STRING_MAP g_EnumStr_BAR_SHOW_MODE[] = {
    { "default", BSM_DEFAULT },
    { "flashing-or-default", BSM_FLASHING_OR_DEFAULT },
    { "flashing-only", BSM_FLASHING_ONLY },
    { "always", BSM_ALWAYS },
    { "never", BSM_NEVER },
    { "ps1", BSM_PS1 },
    { "boss-only", BSM_BOSS_ONLY },
    { NULL, -1 },
};

const ENUM_STRING_MAP g_EnumStr_BAR_LOCATION[] = {
    { "top-left", BL_TOP_LEFT },
    { "top-center", BL_TOP_CENTER },
    { "top-right", BL_TOP_RIGHT },
    { "bottom-left", BL_BOTTOM_LEFT },
    { "bottom-center", BL_BOTTOM_CENTER },
    { "bottom-right", BL_BOTTOM_RIGHT },
    { NULL, -1 },
};

const ENUM_STRING_MAP g_EnumStr_BAR_COLOR[] = {
    { "gold", BC_GOLD },   { "blue", BC_BLUE },     { "grey", BC_GREY },
    { "red", BC_RED },     { "silver", BC_SILVER }, { "green", BC_GREEN },
    { "gold2", BC_GOLD2 }, { "blue2", BC_BLUE2 },   { "pink", BC_PINK },
    { NULL, -1 },
};

const ENUM_STRING_MAP g_EnumStr_TARGET_LOCK_MODE[] = {
    { "full-lock", TLM_FULL },
    { "semi-lock", TLM_SEMI },
    { "no-lock", TLM_NONE },
    { NULL, -1 },
};

const ENUM_STRING_MAP g_EnumStr_SCREENSHOT_FORMAT[] = {
    { "jpg", SCREENSHOT_FORMAT_JPEG },
    { "jpeg", SCREENSHOT_FORMAT_JPEG },
    { "png", SCREENSHOT_FORMAT_PNG },
    { NULL, -1 },
};

const ENUM_STRING_MAP g_EnumStr_UNDERWATER_MUSIC_MODE[] = {
    { "full", UMM_FULL },
    { "quiet", UMM_QUIET },
    { "full_no_ambient", UMM_FULL_NO_AMBIENT },
    { "quiet_no_ambient", UMM_QUIET_NO_AMBIENT },
    { "none", UMM_NONE },
    { NULL, -1 },
};
