#pragma once

#include <trx/game/fx/weather.h>
#include <trx/game/game_flow/enum.h>
#include <trx/game/music/ids.h>
#include <trx/game/objects/types.h>

#include <stdint.h>

typedef struct GF_COMMAND {
    GF_ACTION action;
    int32_t param;
} GF_COMMAND;

// ----------------------------------------------------------------------------
// Sequencer structures
// ----------------------------------------------------------------------------

typedef struct {
    GF_SEQUENCE_EVENT_TYPE type;
    void *data;
} GF_SEQUENCE_EVENT;

typedef struct {
    int32_t length;
    GF_SEQUENCE_EVENT *events;
} GF_SEQUENCE;

// Concrete events data

typedef struct {
    char *path;
    bool is_legal;
    bool is_credit;
    float display_time;
    float fade_in_time;
    float fade_out_time;
} GF_DISPLAY_PICTURE_DATA;

typedef struct {
    char *image_path;
} GF_GLOBE_SELECT_DATA;

typedef enum {
    GF_INV_REGULAR,
    GF_INV_SECRET,
} GF_INV_TYPE;

typedef struct {
    OBJECT_ID object_id;
    GF_INV_TYPE inv_type;
    int32_t quantity;
} GF_ADD_ITEM_DATA;

typedef struct {
    int32_t layer;
    RGB_888 color;
    int32_t speed;
    bool color_add;
    bool fog_gradient;
} GF_SETUP_HORIZON_DATA;

typedef struct {
    XYZ_32 pos;
    RGB_888 color;
} GF_SETUP_LENS_FLARE_DATA;

// ----------------------------------------------------------------------------
// Game flow level structures
// ----------------------------------------------------------------------------

typedef struct {
    int32_t count;
    char **data_paths;
} INJECTION_DATA;

typedef struct {
    const char *path;
    bool is_legal;
    bool is_credit;
    bool is_intro;
} GF_FMV;

typedef struct {
    bool is_present;
    int32_t count;
    MUSIC_SLOT *ids;
} GF_AMBIENT_DATA;

// Stores declared level settings. Each field generated from level/settings.def
// has a presence flag and the declared value.

// clang-format off
typedef struct {
#define M_SETTING(field_, type_)                                               \
    struct {                                                                   \
        bool is_present;                                                       \
        type_ value;                                                           \
    } field_;
#define X_SETTING(name_, field_, type_, config_) M_SETTING(field_, type_)
#define X_SETTING_ENUM(name_, field_, enum_, fallback_) M_SETTING(field_, int32_t)
#include <trx/game/level/settings.def>
#undef X_SETTING
#undef X_SETTING_ENUM
#undef M_SETTING
    char *sfx_path;
} GF_LEVEL_SETTINGS;
// clang-format on

typedef struct {
    int32_t enemy_num;
    int32_t count;
    int16_t *object_ids;
} GF_DROP_ITEM_DATA;

typedef struct {
    int32_t num;
    GF_LEVEL_TYPE type;
    char *path;
    // The stem of path, lower-cased; nullptr for a level that loads no file.
    char *key;
    char *title;
    // Path to the Lua script executed when this level loads: scripts/<key>.lua
    // where the game ships one, nullptr where it does not.
    char *script_path;

    MUSIC_SLOT music_track;
    char *lara_outfit;
    GF_SEQUENCE sequence;
    INJECTION_DATA injections;

    GF_LEVEL_SETTINGS settings;
    WEATHER_TYPE weather_type;
    bool water_particles;

    struct {
        uint32_t pickups;
        uint32_t kills;
        uint32_t ally_kills;
        uint32_t secrets;
    } unobtainable;

    struct {
        int32_t count;
        GF_DROP_ITEM_DATA *data;
    } item_drops;
} GF_LEVEL;

typedef struct {
    int32_t count;
    GF_LEVEL *levels;
} GF_LEVEL_TABLE;

typedef struct {
    XYZ_16 rot;
    int32_t start_level_ordinal;
    int32_t completion_level_ordinal;
    uint32_t prereq_mask;
    uint8_t mesh_idx;
} GF_GLOBE_ENTRY;

// ----------------------------------------------------------------------------
// Mod metadata
// ----------------------------------------------------------------------------

typedef struct {
    char *name;
    int32_t engine;
    char *extends;
} GF_MOD_META;

// ----------------------------------------------------------------------------
// Game flow structures
// ----------------------------------------------------------------------------

typedef struct {
    char *path;

    GF_MOD_META meta;

    GF_LEVEL *title_level;
    GF_LEVEL_TABLE level_tables[GFLT_NUMBER_OF];

    // FMVs
    struct {
        int32_t fmv_count;
        GF_FMV *fmvs;
    };

    // savegame settings
    struct {
        char *savegame_file_fmt;
    };

    // global settings
    struct {
        // The title screen's picture, or nullptr when the gameflow named none
        // or the file it named is missing.
        char *main_menu_background_path;
        // Whether the title runs its level live behind the menu instead of
        // showing a picture. Set from the gameflow naming no picture at all,
        // which a picture it named and could not find is not.
        bool main_menu_use_live_scene;
        bool enable_tr2_item_drops;
        bool convert_dropped_guns;
        GF_AMBIENT_DATA ambient_tracks;
    };

    // other data
    GF_LEVEL_SETTINGS settings;
    INJECTION_DATA injections;

    // Globe select entries
    struct {
        int32_t count;
        GF_GLOBE_ENTRY *entries;
    } globe;
} GAME_FLOW;
