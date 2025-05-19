#pragma once

typedef enum {
    SAVEGAME_STAGE_BEFORE_LOAD,
    SAVEGAME_STAGE_AFTER_LOAD,
    SAVEGAME_STAGE_BEFORE_SAVE,
} SAVEGAME_STAGE;

typedef enum {
    SAVEGAME_FORMAT_INVALID = 0,
    SAVEGAME_FORMAT_LEGACY = 1,
    SAVEGAME_FORMAT_BSON = 2,
} SAVEGAME_FORMAT;

typedef enum {
    VERSION_LEGACY = -1,
    VERSION_0 = 0,
    VERSION_1 = 1,
    VERSION_2 = 2,
    VERSION_3 = 3,
    VERSION_4 = 4,
    VERSION_5 = 5,
    VERSION_6 = 6,

    // Added extra footer after the compressed BSON structure for quicker
    // access to essential data, such as the level counter and the level title,
    // without the need to parse the entire BSON document.
    // Added TR2+ stats ammo hits/used, health packs used, distance travelled.
    VERSION_7 = 7,

    // Added the current TRX version string.
    VERSION_8 = 8,

    // Added the current ambient track to allow triggers to change it at
    // different stages of the level.
    VERSION_9 = 9,
} SAVEGAME_VERSION;
