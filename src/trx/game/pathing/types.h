#pragma once

#include <trx/core/math.h>

typedef struct {
    int32_t left;
    int32_t right;
    int32_t top;
    int32_t bottom;
    int16_t height;
    uint32_t overlap_index;
} BOX_INFO;

typedef struct {
    int16_t exit_box;
    uint16_t search_num;
    int16_t next_expansion;
    int16_t box_num;
} BOX_NODE;

typedef enum {
    LOT_SETUP_DEFAULT,
    LOT_SETUP_BEAST,
    LOT_SETUP_QUADRUPED,
    LOT_SETUP_JUMPER,
    LOT_SETUP_CLIMBER,
    LOT_SETUP_ACROBAT,
    LOT_SETUP_FLYER,
} LOT_SETUP_TYPE;

typedef enum {
    LOT_ZONE_SKELETON,
    LOT_ZONE_BASIC,
    LOT_ZONE_CROCODILE,
    LOT_ZONE_HUMAN,
    LOT_ZONE_FLYER,
} LOT_ZONE;

typedef struct {
    int16_t step;
    int16_t drop;
    int16_t fly;
    uint32_t block_mask;
    // Selects the zone in TR4, which stores the zones by creature kind.
    // TR1-3 store them by step height and select the zone from the step.
    LOT_ZONE zone;
    // TR4 marks the overlaps that take a jump or the monkey bars to cross,
    // and only a creature that can do either is pathed over them.
    bool can_jump;
    bool can_monkey;
} LOT_SETUP;

typedef struct {
    LOT_SETUP setup;
    // TR4's guides leave the floor along their path: they jump the gaps and
    // swing the monkey bars. Set while that is what they are doing.
    bool is_jumping;
    bool is_monkeying;
    BOX_NODE *node;
    int16_t head;
    int16_t tail;
    uint16_t search_num;
    int16_t zone_count;
    int16_t target_box;
    int16_t required_box;
    XYZ_32 target;
} LOT_INFO;

typedef enum {
    TARGET_NONE = 0,
    TARGET_PRIMARY = 1,
    TARGET_SECONDARY = 2,
} TARGET_TYPE;
