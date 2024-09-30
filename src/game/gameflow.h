#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    const char *key;
    const char *value;
} GAMEFLOW_STRING_ENTRY;

typedef struct {
    GAMEFLOW_SEQUENCE_TYPE type;
    void *data;
} GAMEFLOW_SEQUENCE;

typedef struct {
    int32_t enemy_num;
    int32_t count;
    int16_t *object_ids;
} GAMEFLOW_DROP_ITEM_DATA;

typedef struct {
    GAMEFLOW_LEVEL_TYPE level_type;
    int16_t music;
    char *level_title;
    char *level_file;
    GAMEFLOW_STRING_ENTRY *object_strings;
    int8_t demo;
    GAMEFLOW_SEQUENCE *sequence;
    struct {
        bool override;
        RGB_F value;
    } water_color;
    struct {
        bool override;
        float value;
    } draw_distance_fade;
    struct {
        bool override;
        float value;
    } draw_distance_max;
    struct {
        uint32_t pickups;
        uint32_t kills;
        uint32_t secrets;
    } unobtainable;
    struct {
        int length;
        char **data_paths;
    } injections;
    struct {
        int count;
        GAMEFLOW_DROP_ITEM_DATA *data;
    } item_drops;
    GAME_OBJECT_ID lara_type;
} GAMEFLOW_LEVEL;

typedef struct {
    char *main_menu_background_path;
    int32_t gym_level_num;
    int32_t first_level_num;
    int32_t last_level_num;
    int32_t title_level_num;
    int32_t level_count;
    char *savegame_fmt_legacy;
    char *savegame_fmt_bson;
    int8_t has_demo;
    double demo_delay;
    TRISTATE_BOOL force_game_modes;
    TRISTATE_BOOL force_save_crystals;
    GAMEFLOW_LEVEL *levels;
    RGB_F water_color;
    float draw_distance_fade;
    float draw_distance_max;
    struct {
        int length;
        char **data_paths;
    } injections;
    bool convert_dropped_guns;
} GAMEFLOW;

extern GAMEFLOW g_GameFlow;

GAMEFLOW_COMMAND
GameFlow_InterpretSequence(int32_t level_num, GAMEFLOW_LEVEL_TYPE level_type);
GAMEFLOW_COMMAND
GameFlow_StorySoFar(int32_t level_num, int32_t savegame_level);
GAMEFLOW_COMMAND GameFlow_PlayAvailableStory(int32_t slot_num);

bool GameFlow_LoadFromFile(const char *file_name);
void GameFlow_Shutdown(void);

void GameFlow_LoadStrings(int32_t level_num);
