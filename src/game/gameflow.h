#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct GAMEFLOW_DEFUALT_STRING {
    GAME_STRING_ID key;
    char *string;
} GAMEFLOW_DEFAULT_STRING;

typedef struct GAMEFLOW_SEQUENCE {
    GAMEFLOW_SEQUENCE_TYPE type;
    void *data;
} GAMEFLOW_SEQUENCE;

typedef struct GAMEFLOW_LEVEL {
    GAMEFLOW_LEVEL_TYPE level_type;
    int16_t music;
    char *level_title;
    char *level_file;
    char *key1;
    char *key2;
    char *key3;
    char *key4;
    char *pickup1;
    char *pickup2;
    char *puzzle1;
    char *puzzle2;
    char *puzzle3;
    char *puzzle4;
    int8_t demo;
    GAMEFLOW_SEQUENCE *sequence;
    struct {
        bool override;
        RGBF value;
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
    } unobtainable;
    struct {
        int length;
        char **data_paths;
    } injections;
    GAME_OBJECT_ID lara_type;
} GAMEFLOW_LEVEL;

typedef struct GAMEFLOW {
    char *main_menu_background_path;
    int32_t gym_level_num;
    int32_t first_level_num;
    int32_t last_level_num;
    int32_t title_level_num;
    int32_t level_count;
    char *savegame_fmt_legacy;
    char *savegame_fmt_bson;
    int8_t has_demo;
    int32_t demo_delay;
    bool force_disable_game_modes;
    bool force_enable_save_crystals;
    GAMEFLOW_LEVEL *levels;
    char *strings[GS_NUMBER_OF];
    RGBF water_color;
    float draw_distance_fade;
    float draw_distance_max;
    struct {
        int length;
        char **data_paths;
    } injections;
} GAMEFLOW;

extern GAMEFLOW g_GameFlow;
extern GAMEFLOW_DEFAULT_STRING g_GameFlowDefaultStrings[];

GAMEFLOW_OPTION
GameFlow_InterpretSequence(int32_t level_num, GAMEFLOW_LEVEL_TYPE level_type);
GAMEFLOW_OPTION
GameFlow_StorySoFar(int32_t level_num, int32_t savegame_level);
bool GameFlow_LoadFromFile(const char *file_name);
void GameFlow_Shutdown(void);
