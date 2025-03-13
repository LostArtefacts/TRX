#pragma once

#include "../music/ids.h"
#include "../objects/types.h"
#include "./enum.h"

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
    float display_time;
    float fade_in_time;
    float fade_out_time;
} GF_DISPLAY_PICTURE_DATA;

typedef struct {
    struct {
        bool set;
        int32_t value;
    } x, y, z;
} GF_SET_CAMERA_POS_DATA;

#if TR_VERSION == 2
typedef enum {
    GF_INV_REGULAR,
    GF_INV_SECRET,
} GF_INV_TYPE;
#endif

typedef struct {
    GAME_OBJECT_ID object_id;
#if TR_VERSION == 2
    GF_INV_TYPE inv_type;
#endif
    int32_t quantity;
} GF_ADD_ITEM_DATA;

#if TR_VERSION == 1
typedef struct {
    GAME_OBJECT_ID object1_id;
    GAME_OBJECT_ID object2_id;
    int32_t mesh_num;
} GF_MESH_SWAP_DATA;
#endif

// ----------------------------------------------------------------------------
// Game flow level structures
// ----------------------------------------------------------------------------

typedef struct {
    int32_t count;
    char **data_paths;
} INJECTION_DATA;

typedef struct {
    const char *path;
} GF_FMV;

typedef struct {
#if TR_VERSION == 1
    RGB_F water_color;
    float draw_distance_fade;
    float draw_distance_max;
#elif TR_VERSION == 2
    char *sfx_path;
#endif
} GF_LEVEL_SETTINGS;

#if TR_VERSION == 1
typedef struct {
    int32_t enemy_num;
    int32_t count;
    int16_t *object_ids;
} GF_DROP_ITEM_DATA;
#endif

typedef struct {
    int32_t num;
    GF_LEVEL_TYPE type;
    char *path;
    char *title;

    MUSIC_TRACK_ID music_track;
    GF_SEQUENCE sequence;
    INJECTION_DATA injections;

    GF_LEVEL_SETTINGS settings;

#if TR_VERSION == 1
    struct {
        uint32_t pickups;
        uint32_t kills;
        uint32_t secrets;
    } unobtainable;

    struct {
        int count;
        GF_DROP_ITEM_DATA *data;
    } item_drops;

    GAME_OBJECT_ID lara_type;
#endif
} GF_LEVEL;

typedef struct {
    int32_t count;
    GF_LEVEL *levels;
} GF_LEVEL_TABLE;

// ----------------------------------------------------------------------------
// Game flow structures
// ----------------------------------------------------------------------------

typedef struct {
    GF_LEVEL *title_level;
    GF_LEVEL_TABLE level_tables[GFLT_NUMBER_OF];

    // FMVs
    struct {
        int32_t fmv_count;
        GF_FMV *fmvs;
    };

#if TR_VERSION == 1
    // savegame settings
    struct {
        char *savegame_fmt_legacy;
        char *savegame_fmt_bson;
    };

    // global settings
    struct {
        float demo_delay;
        char *main_menu_background_path;
        bool enable_tr2_item_drops;
        bool convert_dropped_guns;
        bool enable_killer_pushblocks;
    };
#elif TR_VERSION == 2
    // flow commands
    struct {
        GF_COMMAND cmd_init;
        GF_COMMAND cmd_title;
        GF_COMMAND cmd_death_demo_mode;
        GF_COMMAND cmd_death_in_game;
        GF_COMMAND cmd_demo_interrupt;
        GF_COMMAND cmd_demo_end;
    };

    // global settings
    struct {
        float demo_delay;
        bool is_demo_version;
        bool play_any_level;
        bool gym_enabled;
        bool lockout_option_ring;
        bool cheat_keys;
        bool load_save_disabled;
        int32_t single_level;
    };

    // music
    struct {
        MUSIC_TRACK_ID secret_track;
    };
#endif

    // other data
    GF_LEVEL_SETTINGS settings;
    INJECTION_DATA injections;
} GAME_FLOW;

typedef GF_COMMAND (*GF_SEQUENCE_EVENT_HANDLER)(
    const GF_LEVEL *, const GF_SEQUENCE_EVENT *, GF_SEQUENCE_CONTEXT, void *);
