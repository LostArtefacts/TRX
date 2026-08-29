#pragma once

// The item lifecycle as the released save format stores it. Not a runtime
// concept - the engine carries is_visible, is_finished and is_simulated; the
// savegame path is the only code that packs them into this value and back
// (M_PackItemStatus in file_write.c).
typedef enum {
    // clang-format off
    IS_INACTIVE    = 0,
    IS_ACTIVE      = 1,
    IS_DEACTIVATED = 2,
    IS_INVISIBLE   = 3,
    // clang-format on
} ITEM_STATUS;

// The flip-map slot word as the released save format stores it. Not a runtime
// concept: the slot carries a 0..31 mask, and this pack/unpack is the only code
// that shifts it into FSF_CODE_BITS and back.
typedef enum {
    // clang-format off
    FSF_ONE_SHOT  = 0x0100,
    FSF_CODE_BITS = 0x3E00,
    // clang-format on
} FLIP_SLOT_FLAG;

// A music track's packed trigger word as the released save format stores it;
// the low byte carries the TR2 delay counter. Not a runtime concept: the track
// carries a 0..31 mask, and this pack/unpack is the only code that shifts it
// into MTF_CODE_BITS and back.
typedef enum {
    // clang-format off
    MTF_ONE_SHOT  = 0x0100,
    MTF_CODE_BITS = 0x3E00,
    // clang-format on
} MUSIC_TRACK_FLAG;

typedef enum {
    SAVEGAME_STAGE_BEFORE_LOAD,
    SAVEGAME_STAGE_AFTER_LOAD,
    SAVEGAME_STAGE_BEFORE_SAVE,
} SAVEGAME_STAGE;

typedef enum {
    SG_VERSION_LEGACY = -1,
    SG_VERSION_1 = 1,

    // Before TRX 1.0
    SG_VERSION_13 = 13,

    // Separated Magnums and Automatic Pistols.
    SG_VERSION_14 = 14,

    // Replaced Lara mesh pointers with outfits
    SG_VERSION_15 = 15,

    // Music save format switched to stream list with play modes.
    SG_VERSION_16 = 16,

    // Carried-item drops are persisted with truthful statuses/positions.
    SG_VERSION_17 = 17,

    // Crystal statistics are persisted in savegames.
    SG_VERSION_18 = 18,

    // Animations are persisted as an object and an index relative to it.
    SG_VERSION_19 = 19,

    // Persists identities and music track flags by key.
    SG_VERSION_20 = 20,

    SG_MIN_SUPPORTED_VERSION = SG_VERSION_13,
    SG_CURRENT_VERSION = SG_VERSION_20,
} SAVEGAME_VERSION;
