#pragma once

#include <trx/core/result.h>
#include <trx/core/vector.h>
#include <trx/game/game_flow/enum.h>

#include <stdint.h>

typedef struct {
    const char *key;
    const char *value;
} GS_GAME_STRING_ENTRY;

typedef struct {
    GS_GAME_STRING_ENTRY *game_strings;
} GS_TABLE;

typedef struct {
    const char *title;
    GS_TABLE table;
} GS_LEVEL;

typedef struct {
    int32_t count;
    GS_LEVEL *entries;
} GS_LEVEL_TABLE;

typedef struct {
    char *path;
    GS_TABLE global;
    GS_LEVEL_TABLE level_tables[GFLT_NUMBER_OF];
} GS_FILE;

void GS_Table_Free(GS_TABLE *gs_table);

RESULT GS_File_CreateFromPath(
    const char *path, bool load_levels, GS_FILE **out_gs_file);
void GS_File_Free(GS_FILE *gs_file);
