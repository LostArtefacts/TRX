#pragma once

#include "../../virtual_file.h"
#include "../math.h"
#include "../objects/ids.h"
#include "./enum.h"

typedef struct {
    VFILE *fp;
    INJECTION_VERSION version;
    INJECTION_FILE_TYPE type;
    bool relevant;
} INJECTION;

typedef struct {
    const INJECTION *injection;
    INJECTION_CHUNK_TYPE type;
    int32_t num_blocks;
    int32_t total_size;
} INJECTION_CHUNK;

typedef struct {
    int16_t room_index;
    int16_t num_vertices;
    int16_t num_quads;
    int16_t num_triangles;
    int16_t num_sprites;
} INJECTION_MESH_META;
