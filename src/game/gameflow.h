#pragma once

#include "global/types.h"

#include <stdint.h>

typedef struct GAMEFLOW_SEQUENCE {
    GAMEFLOW_SEQUENCE_TYPE type;
    void *data;
} GAMEFLOW_SEQUENCE;

typedef struct GAMEFLOW_LEVEL {
    GAMEFLOW_LEVEL_TYPE level_type;
    int16_t music;
    const char *level_title;
    const char *level_file;
    const char *key1;
    const char *key2;
    const char *key3;
    const char *key4;
    const char *pickup1;
    const char *pickup2;
    const char *puzzle1;
    const char *puzzle2;
    const char *puzzle3;
    const char *puzzle4;
    int8_t demo;
    int16_t secrets;
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
} GAMEFLOW_LEVEL;

typedef struct GAMEFLOW {
    const char *main_menu_background_path;
    int32_t gym_level_num;
    int32_t first_level_num;
    int32_t last_level_num;
    int32_t title_level_num;
    int32_t level_count;
    const char *save_game_fmt;
    int8_t has_demo;
    int32_t demo_delay;
    int8_t enable_game_modes;
    int8_t enable_save_crystals;
    GAMEFLOW_LEVEL *levels;
    char *strings[GS_NUMBER_OF];
    RGBF water_color;
    float draw_distance_fade;
    float draw_distance_max;
} GAMEFLOW;

extern GAMEFLOW g_GameFlow;

GAMEFLOW_OPTION
GameFlow_InterpretSequence(int32_t level_num, GAMEFLOW_LEVEL_TYPE level_type);
bool GameFlow_LoadFromFile(const char *file_name);
