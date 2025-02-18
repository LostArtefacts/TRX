#pragma once

#include "../math.h"

typedef struct {
    int32_t left;
    int32_t right;
    int32_t top;
    int32_t bottom;
    int16_t height;
    int16_t overlap_index;
} BOX_INFO;

typedef struct {
    int16_t exit_box;
    uint16_t search_num;
    int16_t next_expansion;
    int16_t box_num;
} BOX_NODE;

typedef struct {
    BOX_NODE *node;
    int16_t head;
    int16_t tail;
    uint16_t search_num;
    uint16_t block_mask;
    int16_t step;
    int16_t drop;
    int16_t fly;
    int16_t zone_count;
    int16_t target_box;
    int16_t required_box;
    XYZ_32 target;
} LOT_INFO;
