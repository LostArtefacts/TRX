#pragma once

// clang-format off
typedef enum {
    // TODO: reset to 1
    INJ_VERSION_1  = 1,
#if TR_VERSION == 1
    INJ_VERSION_2  = 2,
    INJ_VERSION_3  = 3,
    INJ_VERSION_4  = 4,
    INJ_VERSION_5  = 5,
    INJ_VERSION_6  = 6,
    INJ_VERSION_7  = 7,
    INJ_VERSION_8  = 8,
    INJ_VERSION_9  = 9,
    INJ_VERSION_10 = 10,
    INJ_VERSION_11 = 11,
#endif
} INJECTION_VERSION;

typedef enum {
    IFT_GENERAL             = 0,
    IFT_BRAID               = 1,
    IFT_TEXTURE_FIX         = 2,
    IFT_UZI_SFX             = 3,
    IFT_FLOOR_DATA          = 4,
    IFT_LARA_ANIMS          = 5,
    IFT_LARA_JUMPS          = 6, // TODO: eliminate in new format
    IFT_ITEM_POSITION       = 7,
    IFT_PS1_ENEMY           = 8,
    IFT_DISABLE_ANIM_SPRITE = 9,
    IFT_SKYBOX              = 10,
    IFT_PS1_CRYSTAL         = 11,
    IFT_NUMBER_OF           = 12,
} INJECTION_FILE_TYPE;

typedef enum {
    // TODO: expand in new format
    IDT_FLOOR_EDITS = 0,
    IDT_ITEM_EDITS  = 1,
    IDT_NUMBER_OF   = 2,
} INJECTION_DATA_TYPE;

typedef enum {
    FT_TEXTURED_QUAD     = 0,
    FT_TEXTURED_TRIANGLE = 1,
    FT_COLOURED_QUAD     = 2,
    FT_COLOURED_TRIANGLE = 3,
} FACE_TYPE;

typedef enum {
    FET_TRIGGER_PARAM   = 0,
    FET_MUSIC_ONESHOT   = 1,
    FET_FD_INSERT       = 2,
    FET_ROOM_SHIFT      = 3,
    FET_TRIGGER_ITEM    = 4,
    FET_ROOM_PROPERTIES = 5,
    FET_TRIGGER_TYPE    = 6,
} FLOOR_EDIT_TYPE;

typedef enum {
    RMET_TEXTURE_FACE = 0,
    RMET_MOVE_FACE    = 1,
    RMET_ALTER_VERTEX = 2,
    RMET_ROTATE_FACE  = 3,
    RMET_ADD_FACE     = 4,
    RMET_ADD_VERTEX   = 5,
    RMET_ADD_SPRITE   = 6,
} ROOM_MESH_EDIT_TYPE;
// clang-format on
